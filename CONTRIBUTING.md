# Contributing to AtlasSearch

Thank you for your interest in contributing to AtlasSearch! This document provides guidelines for contributing to the project.

## Code of Conduct

- Be respectful and inclusive
- Provide constructive feedback
- Focus on what is best for the community
- Show empathy towards other community members

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in Issues
2. If not, create a new issue with:
   - Clear title and description
   - Steps to reproduce
   - Expected vs actual behavior
   - Environment details (OS, compiler version, etc.)
   - Relevant logs or error messages

### Suggesting Enhancements

1. Check if the enhancement has been suggested
2. Create an issue describing:
   - The problem you're trying to solve
   - Your proposed solution
   - Alternative solutions considered
   - Any relevant examples or mockups

### Pull Requests

1. **Fork the repository**
2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow the coding style (see below)
   - Add tests for new functionality
   - Update documentation as needed

4. **Test your changes**
   ```bash
   mkdir build && cd build
   cmake .. && make
   ctest --output-on-failure
   ```

5. **Commit your changes**
   ```bash
   git commit -m "Add feature: brief description"
   ```
   - Use clear, descriptive commit messages
   - Reference issue numbers if applicable

6. **Push to your fork**
   ```bash
   git push origin feature/your-feature-name
   ```

7. **Create a Pull Request**
   - Provide a clear description of changes
   - Link to related issues
   - Ensure CI/CD passes

## Coding Style

### C++ Guidelines

1. **Modern C++17**
   - Use `auto` where type is obvious
   - Prefer `std::unique_ptr` and `std::shared_ptr`
   - Use structured bindings
   - Avoid raw pointers when possible

2. **Naming Conventions**
   - Classes: `PascalCase` (e.g., `SearchService`)
   - Functions: `camelCase` (e.g., `performSearch`)
   - Variables: `snake_case` (e.g., `max_results`)
   - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_RETRIES`)
   - Private members: trailing underscore (e.g., `data_`)

3. **Formatting**
   - Indentation: 4 spaces (no tabs)
   - Line length: 100 characters max
   - Braces: K&R style
   ```cpp
   if (condition) {
       // code
   } else {
       // code
   }
   ```

4. **Comments**
   - Use `//` for single-line comments
   - Use `/* */` for multi-line comments
   - Document complex algorithms
   - Explain "why", not "what"

5. **Error Handling**
   - Use exceptions for exceptional cases
   - Check return values
   - Provide meaningful error messages
   - Clean up resources (RAII)

### Example

```cpp
/*
 * SearchService - Provides search API with custom reranking
 * 
 * This class integrates with Elasticsearch and applies a custom
 * reranking algorithm to improve result relevance.
 */
class SearchService {
public:
    SearchService(const std::string& es_host, int es_port);
    ~SearchService();

    // Perform search with reranking
    SearchResponse search(const std::string& query, int size = 10);

private:
    std::unique_ptr<ElasticsearchClient> es_client_;
    
    // Calculate reranked score
    double calculateRerankedScore(double es_score, 
                                   const std::string& updated_at,
                                   const std::string& title,
                                   const std::string& query);
};
```

## Testing Guidelines

1. **Unit Tests**
   - Test each function/method independently
   - Use GoogleTest framework
   - Mock external dependencies
   - Test edge cases

2. **Integration Tests**
   - Test component interactions
   - Use real services (Docker Compose)
   - Verify end-to-end flows

3. **Test Naming**
   ```cpp
   TEST_F(SearchServiceTest, ReturnsEmptyResultsForEmptyQuery) {
       // Test implementation
   }
   ```

## Documentation

1. **Code Documentation**
   - Document all public APIs
   - Include complexity analysis
   - Provide usage examples

2. **README Updates**
   - Update relevant README files
   - Add new features to main README
   - Include usage examples

3. **Design Documents**
   - Update `docs/design.md` for architectural changes
   - Explain design decisions
   - Include diagrams if helpful

## Commit Message Guidelines

Format:
```
<type>: <subject>

<body>

<footer>
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Maintenance tasks

Example:
```
feat: Add Redis caching to search service

Implement Redis caching layer to reduce Elasticsearch load
and improve response times. Cache entries expire after 5 minutes.

Closes #42
```

## Review Process

1. **Automated Checks**
   - CI/CD must pass
   - All tests must pass
   - Code must compile without warnings

2. **Code Review**
   - At least one approval required
   - Address all review comments
   - Keep discussions focused and respectful

3. **Merge**
   - Squash commits if requested
   - Ensure commit message is clear
   - Delete branch after merge

## Development Setup

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install -y \
    build-essential \
    cmake \
    libcurl4-openssl-dev \
    libboost-all-dev \
    librdkafka-dev \
    libhiredis-dev \
    libyaml-cpp-dev

# macOS
brew install cmake curl boost librdkafka hiredis yaml-cpp
```

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Run Tests

```bash
ctest --output-on-failure --verbose
```

### Start Infrastructure

```bash
docker-compose up -d
```

## Adding New Features

### New Algorithm

1. Create file in `algorithms/cpp/`
2. Include problem description and complexity
3. Add `main()` with example usage
4. Update `algorithms/README.md`

### New Service

1. Create directory in `services/`
2. Add `CMakeLists.txt`
3. Add `README.md` with usage
4. Implement service with tests
5. Update root `CMakeLists.txt`
6. Update root `README.md`

## Questions?

- Open an issue for questions
- Check existing issues and PRs
- Review documentation in `docs/`

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

Thank you for contributing to AtlasSearch! 🚀
