## Dependencies

- Boost
- nlohmann/json
- bcrypt
- libpqxx
- PostgreSQL

─ xcode-select --install
brew install lld
brew install lcov
brew install llvm

### yaml-cpp

#### macOS

```bash
brew install yaml-cpp
```

#### Ubuntu

```bash
sudo apt-get install libyaml-cpp-dev
```

### bcrypt

#### macOS

```bash
brew install libbcrypt
```

#### Ubuntu

```bash
sudo apt-get install libbcrypt-dev
```

### libpqxx

#### macOS

```bash
brew install libpqxx
```

#### Ubuntu

```bash
sudo apt-get install libpqxx-dev
```

### PostgreSQL

#### macOS

```bash

sudo chown -R $(whoami) ~/Library/LaunchAgents

brew install postgresql@15
brew link --force postgresql@15

```

Start the service

```bash
brew services start postgresql@15
```

Connect to the database

```bash
psql -U [your_username] -W postgres
```

#### SQL

```sql
CREATE
DATABASE kudeta;

\c
kudeta
```

Create the users table

```sql
CREATE TABLE users
(
    id            BIGSERIAL PRIMARY KEY,
    username      VARCHAR(255) NOT NULL UNIQUE,
    email         VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    salt          VARCHAR(255) NOT NULL,
    is_active     BOOLEAN                  DEFAULT true,
    created_at    TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at    TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_users_username ON users (username);
CREATE INDEX idx_users_email ON users (email);
```

## Environment Setup

1. Copy the example environment file:
   ```bash
   cp .env.example .env
   ```

2. Edit .env and set your values:
   ```env
    DB_PASSWORD=your_password
    DB_USER=your_username
    DB_NAME=your_database_name
    DB_HOST=your_host
    DB_PORT=your_port
   ```

3. Run the application:
   ```bash
   ./scripts/run.sh
   ```

## Build

### Debug

```bash
./build.sh
```

### Release

```bash
./build.sh Release
```

### Clean

```bash
./build.sh clean
```

### Rebuild

```bash
./build.sh rebuild
```

### Rebuild Release

```bash
./build.sh rebuild Release
```

## Run

```bash
./bin/Debug/kudeta run config.json
```

## Test Client

For authentication:

```json
{
  "username": "user1"
}
```

For regular chat messages:

```json
{
  "type": "message",
  "content": "Hello everyone!"
}
```

For direct messages:

```json
{
  "type": "direct_message",
  "recipient": "user2",
  "content": "Hello user2!"
}
```

New message

```json
{
  "type": "new",
  "content": "Hello @user1, how are you?"
}
```

Edit message

```json
{
  "type": "edit",
  "id": "message-uuid",
  "content": "Updated message content @user2"
}
```

Delete message

```json
{
  "type": "delete",
  "id": "message-uuid"
}
```

## Testing

```bash
./bin/Debug/kudeta_local_tests --gtest_color=yes
```

```bash
# Build the tests
cmake --build build/

# Run all tests
ctest --test-dir build/

# Run with verbose output
ctest --test-dir build/ -V

# Run specific test
./build/user_service_test
```

### Running Specific Test Cases

You can use Google Test's built-in filters:

```bash
# Run all tests in UserServiceTest
./build/user_service_test --gtest_filter="UserServiceTest.*"

# Run a specific test
./build/user_service_test --gtest_filter="UserServiceTest.RegisterUser_ValidInput_Success"

# Run tests matching a pattern
./build/user_service_test --gtest_filter="*Register*"

# Exclude certain tests
./build/user_service_test --gtest_filter="-*ValidateEmail*"
```

Additional Useful Flags

```bash
# Run tests in random order
./build/user_service_test --gtest_shuffle

# Run tests multiple times
./build/user_service_test --gtest_repeat=10

# Generate XML report
./build/user_service_test --gtest_output="xml:test_report.xml"

# Show all test cases without running them
./build/user_service_test --gtest_list_tests

# Run tests in parallel
./build/user_service_test --gtest_parallel=4
```

Using VS Code
If you're using VS Code, you can use the "Test Explorer" extension:
Install the "C/C++ Test Adapter" extension
Configure .vscode/settings.json:

```json
{
  "cmake.testPreset": "default",
  "cmake.defaultTest": {
    "parallel": true,
    "output": "${workspaceFolder}/build/Testing/"
  }
}
```

Then you can:
Click the test explorer icon in the sidebar
Run/debug individual tests
See test results visually

Using CLion
If you're using CLion:

1. Right-click on the test file in the project explorer
   Select "Run 'user_service_test'"
   Use the test explorer window to run individual tests
   Remember to:
   Set up your test environment (database, configuration, etc.) before running tests
   Check the test output for failures and details
   Review test coverage if you have coverage tools enabled
   For debugging failed tests:

```bash
# Run with more detailed output
./build/user_service_test --gtest_filter="FailingTest" --gtest_break_on_failure

# Print timing for each test
./build/user_service_test --gtest_print_time=1
```

## Plugins

### Create task

```json
{
  "type": "plugin",
  "plugin": "TaskManagement",
  "action": "create",
  "data": {
    "title": "Implement login system",
    "description": "Add user authentication to the chat system",
    "assignee": "john_doe"
  }
}
```

### Update task status

```json
{
  "type": "plugin",
  "plugin": "TaskManagement",
  "action": "update",
  "data": {
    "id": "task-123",
    "status": "in_progress"
  }
}
```

### List tasks

```json
{
  "type": "plugin",
  "plugin": "TaskManagement",
  "action": "list",
  "data": {
    "filter": "assigned_to_me"
  }
}
```

## Markdown Editor

### Format text

```json
{
  "type": "plugin",
  "plugin": "MarkdownEditor",
  "action": "format",
  "data": {
    "text": "Hello World",
    "format": "bold"
  }
}
```

### Generate preview

```json
{
  "type": "plugin",
  "plugin": "MarkdownEditor",
  "action": "preview",
  "data": {
    "text": "# Hello World\n\nThis is a **markdown** _text_."
  }
}
```

## Register

```json
{
  "type": "register",
  "username": "user1",
  "email": "user1@example.com",
  "password": "password"
}
```

Response

```json     
{
  "type": "register_response",
  "success": true,
  "message": "Registration successful"
}
```

## Database

```bash
psql -d kudeta -U kudeta -f ./sql/schema.sql
``` 

Basic schema for users:

```sql
CREATE TABLE users
(
    id            BIGSERIAL PRIMARY KEY,
    username      VARCHAR(255) NOT NULL UNIQUE,
    email         VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    salt          VARCHAR(255) NOT NULL,
    is_active     BOOLEAN                  DEFAULT true,
    created_at    TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    updated_at    TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX idx_users_username ON users (username);
CREATE INDEX idx_users_email ON users (email);
```

## Development Setup

### Supported Platforms

- macOS (via Homebrew)
- Linux (Ubuntu/Debian via apt, Fedora via dnf)
- FreeBSD (via pkg)

### Prerequisites

- Git
- Bash shell
- Internet connection
- Sudo privileges (for Linux/FreeBSD)

### Quick Start

1. Clone the repository:

```bash
git clone https://github.com/yourusername/kudeta_local.git
cd kudeta_local
```

2. Run the setup script:

```bash
./scripts/setup.sh
```

### Platform-Specific Notes

#### FreeBSD

- Uses clang/LLVM as the default compiler
- Requires a restart of the shell for PATH changes to take effect
- Some tools might be in different locations compared to Linux/macOS

If you encounter permission issues on FreeBSD:

```bash
sudo chown -R $(whoami) /usr/local

# Make sure you're in the wheel group
su
pw groupmod wheel -m yourusername

#Then log out and log back in for the changes to take effect.

exit
```

The setup script will:

- Detect your operating system
- Install necessary package managers (Homebrew for macOS, apt/dnf for Linux)
- Install all required dependencies
- Setup the development environment
- Configure git hooks
- Perform initial build

### Manual Build

After setup, you can:

- Build the project: `./build.sh`
- Run tests: `./build.sh test`
- Generate coverage: `./build.sh coverage`

### Development Workflow

1. Make your changes
2. Tests will run automatically on commit (via pre-commit hook)
3. Build and test manually as needed
4. Submit your PR

### Troubleshooting

If you encounter any issues during setup:

1. Check the logs directory
2. Ensure all prerequisites are met
3. Try running the specific failed step manually
4. Open an issue if the problem persists
