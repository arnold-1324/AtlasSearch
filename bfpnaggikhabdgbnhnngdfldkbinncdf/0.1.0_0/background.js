// OpenClaw Copilot background service worker
let gatewayWs = null;
let gatewayPingTimer = null;
let gatewayStatus = "disconnected";
let gatewayConfig = { url: "ws://127.0.0.1:18789", token: "" };
let mainSessionKey = null;

let relayWs = null;
let relayConnecting = null;
let relayInitialized = false;
let relayBackoffMs = 1000;
let relayReconnectTimer = null;

let gatewayBackoffMs = 1000;
let gatewayReconnectTimer = null;

let activeTabId = null;
let sessionCounter = 1;
const tabMap = new Map(); // tabId -> { sessionId, targetId }
const sessionToTab = new Map(); // sessionId -> tabId

let pWriteAccess = false;
const WRITE_METHOD_PREFIXES = [
  "Input.",
  "Page.navigate",
  "Page.reload",
  "Runtime.evaluate",
  "Runtime.callFunctionOn",
  "DOM.set",
  "DOM.insert",
  "DOM.remove",
  "DOM.focus",
  "Target.createTarget",
  "Target.closeTarget",
  "Emulation.",
  "Network.set",
];

const KEEPALIVE_ALARM = "oc-keepalive";

// Antigravity-style keepalive
function antigravityKeepalive() {
  chrome.runtime.sendMessage(chrome.runtime.id, { type: "PING" }).catch(() => {});
  const t = setInterval(function () {}, 10000);
  setTimeout(function () {
    clearInterval(t);
  }, 60000);
}

// Run once on load (mirrors Antigravity's immediate keepalive)
antigravityKeepalive();

function sendToSidePanel(msg) {
  chrome.runtime.sendMessage(msg).catch(() => {});
}

function setStatus(status) {
  gatewayStatus = status;
  sendToSidePanel({ type: "STATUS_UPDATE", status });
}

function notifyWelcome() {
  try {
    chrome.notifications.create({
      type: "basic",
      iconUrl: "icons/icon128.png",
      title: "Elsa — OpenClaw",
      message: "I’m here. Open any site and I can help (read‑only by default).",
    });
  } catch (e) {}
  try {
    sendToSidePanel({
      type: "INCOMING_MESSAGE",
      content:
        "I’m here — tell me what you want to do. (Read‑only by default; enable write access in Settings when you’re ready.)",
    });
  } catch (e) {}
}

function scheduleGatewayReconnect(reason) {
  if (gatewayReconnectTimer) return;
  const wait = Math.min(gatewayBackoffMs, 30000);
  console.log("Scheduling gateway reconnect in", wait, "ms due to", reason);
  gatewayReconnectTimer = setTimeout(() => {
    gatewayReconnectTimer = null;
    gatewayBackoffMs = Math.min(gatewayBackoffMs * 1.6, 30000);
    connectGateway();
  }, wait);
}

function scheduleRelayReconnect(reason) {
  if (relayReconnectTimer) return;
  const wait = Math.min(relayBackoffMs, 30000);
  console.log("Scheduling relay reconnect in", wait, "ms due to", reason);
  relayReconnectTimer = setTimeout(() => {
    relayReconnectTimer = null;
    relayBackoffMs = Math.min(relayBackoffMs * 1.6, 30000);
    connectRelay().catch((e) => console.error("Relay auto-connect failed:", e));
  }, wait);
}

function resetBackoffs() {
  relayBackoffMs = 1000;
  gatewayBackoffMs = 1000;
}

function loadConfigAndConnect() {
  chrome.storage.local.get(["gatewayUrl", "gatewayToken", "writeAccess"], (res) => {
    pWriteAccess = !!res.writeAccess;
    gatewayConfig.url = res.gatewayUrl || "ws://127.0.0.1:18789";
    gatewayConfig.token = res.gatewayToken || "";
    connectGateway();
  });
}

function connectGateway() {
  if (gatewayWs) {
    try {
      gatewayWs.close();
    } catch (e) {}
    gatewayWs = null;
  }
  if (gatewayPingTimer) clearInterval(gatewayPingTimer);

  setStatus("connecting");
  mainSessionKey = null;
  console.log("Connecting to Gateway:", gatewayConfig.url);

  try {
    gatewayWs = new WebSocket(gatewayConfig.url);

    gatewayWs.onopen = () => {
      console.log("Gateway WebSocket Connected");
      resetBackoffs();
      const handshake = {
        type: "req",
        id: "connect-1",
        method: "connect",
        params: {
          client: {
            id: "gateway-client",
            version: "0.1.0",
            platform: "browser",
            mode: "ui",
          },
          auth: gatewayConfig.token ? { token: gatewayConfig.token } : undefined,
          minProtocol: 3,
          maxProtocol: 3,
        },
      };
      console.log("Sending handshake:", JSON.stringify(handshake, null, 2));
      gatewayWs.send(JSON.stringify(handshake));

      gatewayPingTimer = setInterval(() => {
        if (!gatewayWs || gatewayWs.readyState !== WebSocket.OPEN) return;
        // keepalive no-op
      }, 30000);

      connectRelay().catch((e) => console.error("Relay auto-connect failed:", e));
    };

    gatewayWs.onmessage = (evt) => {
      try {
        const msg = JSON.parse(evt.data);
        if (msg.type === "res" && msg.id === "connect-1") {
          if (msg.ok && msg.payload && msg.payload.type === "hello-ok") {
            console.log("Handshake successful");
            setStatus("connected");
            const snapshot = msg.payload.snapshot;
            if (snapshot?.sessionDefaults?.mainSessionKey) {
              mainSessionKey = snapshot.sessionDefaults.mainSessionKey;
              console.log("Session Key acquired:", mainSessionKey);
              attachToActiveTab();
            } else {
              console.warn("No default session key found in snapshot.");
              sendToSidePanel({
                type: "ERROR_MESSAGE",
                content: "Connected, but no active session found.",
              });
            }
          } else {
            console.error("Handshake failed:", msg.error);
            sendToSidePanel({
              type: "ERROR_MESSAGE",
              content: `Handshake failed: ${msg.error ? msg.error.message : "Unknown error"}`,
            });
          }
          return;
        }

        if (msg.type === "event" && msg.event === "chat") {
          const payload = msg.payload;
          if (payload?.message?.content) {
            const content = payload.message.content;
            let text = "";
            if (Array.isArray(content)) text = content.map((x) => x.text || "").join("");
            else if (typeof content === "string") text = content;
            if (text && payload.state === "final") {
              sendToSidePanel({ type: "INCOMING_MESSAGE", content: text });
            }
          }
        }

        if (msg.type === "res" && msg.error) {
          console.error("Gateway Error:", msg.error);
          sendToSidePanel({
            type: "ERROR_MESSAGE",
            content: `Error: ${msg.error.message || "Unknown error"}`,
          });
        }
      } catch (e) {
        console.warn("Received non-JSON message:", evt.data);
      }
    };

    gatewayWs.onclose = (evt) => {
      console.log("Gateway WebSocket Closed", evt.code, evt.reason);
      setStatus("disconnected");
      if (gatewayPingTimer) clearInterval(gatewayPingTimer);
      gatewayWs = null;
      scheduleGatewayReconnect("close");
    };

    gatewayWs.onerror = (err) => {
      console.error("Gateway WebSocket Error", err);
      setStatus("disconnected");
      scheduleGatewayReconnect("error");
    };
  } catch (e) {
    console.error("Gateway Connection failed:", e);
    setStatus("disconnected");
    scheduleGatewayReconnect("exception");
  }
}

async function connectRelay() {
  if (relayWs && relayWs.readyState === WebSocket.OPEN) return;
  if (relayConnecting) return await relayConnecting;

  relayConnecting = (async () => {
    const relayPort = await (async function () {
      const res = await chrome.storage.local.get(["relayPort"]);
      const port = parseInt(res.relayPort || "18792", 10);
      return !Number.isFinite(port) || port <= 0 || port > 65535 ? 18792 : port;
    })();

    const relayUrl = `ws://127.0.0.1:${relayPort}/extension`;
    console.log("Connecting to Relay:", relayUrl);

    try {
      const ws = new WebSocket(relayUrl);
      relayWs = ws;

      await new Promise((resolve, reject) => {
        const timeout = setTimeout(() => reject(new Error("Relay connect timeout")), 5000);
        ws.onopen = () => {
          clearTimeout(timeout);
          console.log("Relay Connected");
          resolve();
        };
        ws.onerror = (e) => {
          clearTimeout(timeout);
          reject(e);
        };
        ws.onclose = () => clearTimeout(timeout);
      });

      relayWs.onmessage = (evt) => handleRelayMessage(String(evt.data || ""));
      relayWs.onclose = () => onRelayDisconnected("closed");
      relayWs.onerror = () => onRelayDisconnected("error");

      if (!relayInitialized) {
        relayInitialized = true;
        chrome.debugger.onEvent.addListener(onDebuggerEvent);
        chrome.debugger.onDetach.addListener(onDebuggerDetach);
      }

      resetBackoffs();
      relayBackoffMs = 1000;
    } catch (e) {
      console.error("Relay connection failed:", e);
      relayWs = null;
      scheduleRelayReconnect("connect-failed");
      throw e;
    }
  })();

  try {
    await relayConnecting;
  } finally {
    relayConnecting = null;
  }
}

function onRelayDisconnected(reason) {
  console.log("Relay disconnected:", reason);
  relayWs = null;
  tabMap.forEach((_, tabId) => {
    chrome.debugger.detach({ tabId }).catch(() => {});
  });
  tabMap.clear();
  sessionToTab.clear();
  scheduleRelayReconnect(reason);
}

async function attachToActiveTab() {
  await connectRelay();
  const [tab] = await chrome.tabs.query({ active: true, lastFocusedWindow: true });
  if (tab?.id) await attachToTab(tab.id);
}

async function attachToTab(tabId) {
  const target = { tabId };
  if (tabMap.has(tabId)) return tabMap.get(tabId);

  try {
    const tab = await chrome.tabs.get(tabId).catch(() => null);
    const url = tab?.url || "";
    const restricted =
      url.startsWith("chrome://") ||
      url.startsWith("edge://") ||
      (url.startsWith("about:") && url !== "about:blank") ||
      url.startsWith("chrome-extension://") ||
      url.startsWith("devtools://") ||
      url.includes("chromewebstore.google.com") ||
      url.includes("chrome.google.com/webstore");
    if (restricted) {
      console.log(`Cannot attach to restricted URL: tab ${tabId}`);
      return null;
    }

    await chrome.debugger.attach(target, "1.3");
    await chrome.debugger.sendCommand(target, "Page.enable").catch(() => {});
    const info = await chrome.debugger.sendCommand(target, "Target.getTargetInfo");
    const targetInfo = info?.targetInfo;
    const targetId = String(targetInfo?.targetId || "").trim();
    const sessionId = `cb-tab-${sessionCounter++}`;

    tabMap.set(tabId, { sessionId, targetId });
    console.log(`Debugger attached to tab ${tabId} (Session: ${sessionId})`);

    if (relayWs && relayWs.readyState === WebSocket.OPEN) {
      relayWs.send(
        JSON.stringify({
          method: "forwardCDPEvent",
          params: {
            method: "Target.attachedToTarget",
            params: {
              sessionId,
              targetInfo: { ...targetInfo, attached: true },
              waitingForDebugger: false,
            },
          },
        })
      );
    }

    return { sessionId, targetId };
  } catch (e) {
    if (e.message?.includes("Already attached")) {
      console.warn(`Tab ${tabId} is already attached but not in our map.`);
      return null;
    }
    if (e.message?.includes("Cannot access a chrome:// URL")) {
      console.log(`Cannot attach to restricted URL: tab ${tabId}`);
      return null;
    }
    console.error(`Failed to attach to tab ${tabId}:`, e);
    throw e;
  }
}

function onDebuggerDetach(source, reason) {
  console.log(`Debugger detached from ${source.tabId}: ${reason}`);
  const entry = tabMap.get(source.tabId);
  for (const [sid, tabId] of sessionToTab.entries()) {
    if (tabId === source.tabId) sessionToTab.delete(sid);
  }
  if (entry && relayWs && relayWs.readyState === WebSocket.OPEN) {
    relayWs.send(
      JSON.stringify({
        method: "forwardCDPEvent",
        params: {
          sessionId: entry.sessionId,
          targetId: entry.targetId,
          reason,
          method: "Target.detachedFromTarget",
        },
      })
    );
  }
  tabMap.delete(source.tabId);
}

function safeRelaySend(payload) {
  try {
    if (relayWs && relayWs.readyState === WebSocket.OPEN) {
      relayWs.send(JSON.stringify(payload));
      return true;
    }
  } catch {}
  return false;
}

function onDebuggerEvent(source, method, params) {
  if (!relayWs || relayWs.readyState !== WebSocket.OPEN) return;
  const entry = tabMap.get(source.tabId);
  if (!entry?.sessionId) return;

  if (method === "Target.attachedToTarget" && params?.sessionId) {
    sessionToTab.set(String(params.sessionId), source.tabId);
  }
  if (method === "Target.detachedFromTarget" && params?.sessionId) {
    sessionToTab.delete(String(params.sessionId));
  }

  const payload = {
    method: "forwardCDPEvent",
    params: {
      sessionId: source.sessionId || entry.sessionId,
      method,
      params,
    },
  };

  if (method.startsWith("Target.") || method.startsWith("Page.load")) {
    console.log(`[CDP Event] ${method} (Session: ${payload.params.sessionId})`);
  }

  safeRelaySend(payload);
}

async function handleRelayMessage(raw) {
  let msg;
  try {
    msg = JSON.parse(raw);
  } catch {
    return;
  }

  if (msg.method === "ping") {
    if (relayWs) relayWs.send(JSON.stringify({ method: "pong" }));
    return;
  }

  if (msg.method === "forwardCDPCommand") {
    try {
      const result = await forwardCDPCommand(msg);
      relayWs?.send(JSON.stringify({ id: msg.id, result }));
    } catch (e) {
      relayWs?.send(JSON.stringify({ id: msg.id, error: e.message }));
    }
    return;
  }

  if (msg.method === "Browser.attach") {
    attachToActiveTab()
      .then(() => relayWs?.send(JSON.stringify({ id: msg.id, result: { attached: true } })))
      .catch((e) => relayWs?.send(JSON.stringify({ id: msg.id, error: e.message })));
    return;
  }

  if (msg.id && msg.method) {
    let tabId = null;
    if (tabMap.size > 0) tabId = activeTabId && tabMap.has(activeTabId) ? activeTabId : tabMap.keys().next().value;
    if (!tabId) return relayWs?.send(JSON.stringify({ id: msg.id, error: "No target tab attached" }));

    try {
      const result = await chrome.debugger.sendCommand({ tabId }, msg.method, msg.params);
      relayWs?.send(JSON.stringify({ id: msg.id, result }));
    } catch (e) {
      relayWs?.send(JSON.stringify({ id: msg.id, error: e.message }));
    }
  }
}

async function forwardCDPCommand(cmd) {
  const method = String(cmd?.params?.method || "").trim();
  const params = cmd?.params?.params || undefined;
  const sessionId = typeof cmd?.params?.sessionId === "string" ? cmd.params.sessionId : undefined;

  if (!pWriteAccess && WRITE_METHOD_PREFIXES.some((p) => method.startsWith(p))) {
    throw new Error("Write actions are disabled (read-only mode). Enable write access in Settings.");
  }

  let tabId = null;
  if (sessionId) {
    for (const [tid, info] of tabMap.entries()) {
      if (info.sessionId === sessionId) {
        tabId = tid;
        break;
      }
    }
    if (!tabId) tabId = sessionToTab.get(sessionId);
  }

  if (!tabId && params?.targetId) {
    for (const [tid, info] of tabMap.entries()) {
      if (info.targetId === params.targetId) {
        tabId = tid;
        break;
      }
    }
  }

  if (!tabId && tabMap.size > 0) tabId = activeTabId && tabMap.has(activeTabId) ? activeTabId : tabMap.keys().next().value;
  if (!tabId && method !== "Target.createTarget") throw new Error(`No attached tab for method ${method}`);

  const debugTarget = { tabId };

  if (method === "Runtime.enable") {
    try {
      await chrome.debugger.sendCommand(debugTarget, "Runtime.disable");
      await new Promise((r) => setTimeout(r, 50));
    } catch {}
    return await chrome.debugger.sendCommand(debugTarget, "Runtime.enable", params);
  }

  if (method === "Target.createTarget") {
    const url = typeof params?.url === "string" ? params.url : "about:blank";
    const newTab = await chrome.tabs.create({ url, active: false });
    if (!newTab.id) throw new Error("Failed to create tab");
    await new Promise((r) => setTimeout(r, 100));
    await attachToTab(newTab.id);
    const info = tabMap.get(newTab.id);
    return { targetId: info?.targetId || "" };
  }

  if (method === "Target.closeTarget") {
    const targetId = typeof params?.targetId === "string" ? params.targetId : "";
    let closeId = null;
    if (targetId) {
      for (const [tid, info] of tabMap.entries()) {
        if (info.targetId === targetId) {
          closeId = tid;
          break;
        }
      }
    } else {
      closeId = tabId;
    }
    if (!closeId) return { success: false };
    try {
      await chrome.tabs.remove(closeId);
      return { success: true };
    } catch {
      return { success: false };
    }
  }

  if (method === "Target.activateTarget") {
    const targetId = typeof params?.targetId === "string" ? params.targetId : "";
    let activateId = null;
    if (targetId) {
      for (const [tid, info] of tabMap.entries()) {
        if (info.targetId === targetId) {
          activateId = tid;
          break;
        }
      }
    } else {
      activateId = tabId;
    }
    if (!activateId) return {};
    const t = await chrome.tabs.get(activateId).catch(() => null);
    if (t) {
      if (t.windowId) await chrome.windows.update(t.windowId, { focused: true }).catch(() => {});
      await chrome.tabs.update(activateId, { active: true }).catch(() => {});
    }
    return {};
  }

  return await chrome.debugger.sendCommand(debugTarget, method, params);
}

async function broadcastTabStatus() {
  try {
    let [tab] = await chrome.tabs.query({ active: true, lastFocusedWindow: true });
    if (!tab) {
      const [fallback] = await chrome.tabs.query({ active: true, currentWindow: true });
      tab = fallback;
    }
    if (!tab) {
      return sendToSidePanel({ type: "TAB_STATUS_UPDATE", status: "disconnected", title: "No active tab" });
    }
    const restricted =
      tab.url &&
      (tab.url.startsWith("chrome://") ||
        tab.url.startsWith("edge://") ||
        (tab.url.startsWith("about:") && tab.url !== "about:blank") ||
        tab.url.startsWith("chrome-extension://") ||
        tab.url.startsWith("devtools://"));

    let status = "disconnected";
    if (tabMap.has(tab.id)) status = "connected";
    else if (restricted) status = "restricted";

    sendToSidePanel({
      type: "TAB_STATUS_UPDATE",
      status,
      title: tab.title || "Unknown Tab",
      url: tab.url,
    });
  } catch (e) {
    console.error("Error broadcasting tab status:", e);
  }
}

function startKeepalive() {
  try {
    chrome.alarms.clear(KEEPALIVE_ALARM, () => {
      chrome.alarms.create(KEEPALIVE_ALARM, { periodInMinutes: 1 });
    });
  } catch (e) {}
}

chrome.alarms.onAlarm.addListener((alarm) => {
  if (alarm.name !== KEEPALIVE_ALARM) return;
  antigravityKeepalive();
  if (!gatewayWs || gatewayWs.readyState !== WebSocket.OPEN) {
    connectGateway();
  }
  if (!relayWs || relayWs.readyState !== WebSocket.OPEN) {
    connectRelay().catch(() => {});
  }
});

chrome.runtime.onInstalled.addListener(() => {
  console.log("OpenClaw Copilot installed.");
  chrome.sidePanel?.setPanelBehavior?.({ openPanelOnActionClick: true }).catch((e) =>
    console.error("Failed to set panel behavior:", e)
  );
  antigravityKeepalive();
  loadConfigAndConnect();
  startKeepalive();
  notifyWelcome();
});

chrome.runtime.onStartup.addListener(() => {
  antigravityKeepalive();
  loadConfigAndConnect();
  startKeepalive();
  notifyWelcome();
});

chrome.tabs.onActivated.addListener(async (info) => {
  activeTabId = info.tabId;
  if (relayWs && relayWs.readyState === WebSocket.OPEN) await attachToTab(info.tabId);
  broadcastTabStatus();
});

chrome.tabs.onUpdated.addListener(async (tabId, changeInfo, tab) => {
  if (changeInfo.status === "complete" && tab.active) {
    if (relayWs && relayWs.readyState === WebSocket.OPEN) await attachToTab(tabId);
    broadcastTabStatus();
  }
});

chrome.tabs.onRemoved.addListener((tabId) => {
  if (tabMap.has(tabId)) onDebuggerDetach({ tabId }, "tab_removed");
  setTimeout(broadcastTabStatus, 200);
});

chrome.runtime.onMessage.addListener((msg, sender, reply) => {
  if (msg.type === "CONFIG_UPDATED") {
    loadConfigAndConnect();
  } else if (msg.type === "SEND_MESSAGE") {
    if (!gatewayWs || gatewayWs.readyState !== WebSocket.OPEN) {
      return sendToSidePanel({ type: "ERROR_MESSAGE", content: "Not connected." });
    }
    if (!mainSessionKey) {
      return sendToSidePanel({ type: "ERROR_MESSAGE", content: "No active session. Please reconnect." });
    }
    const id = `msg-${Date.now()}`;
    const payload = {
      type: "req",
      id,
      method: "chat.send",
      params: { sessionKey: mainSessionKey, message: msg.content, idempotencyKey: id },
    };
    gatewayWs.send(JSON.stringify(payload));
  } else if (msg.type === "GET_STATUS") {
    reply({ status: gatewayStatus });
  } else if (msg.type === "GET_TAB_STATUS") {
    broadcastTabStatus();
  } else if (msg.type === "CONNECT_CURRENT_TAB") {
    attachToActiveTab()
      .then(() => reply({ success: true }))
      .catch((e) => reply({ success: false, error: e.message }));
    return true;
  }
});
