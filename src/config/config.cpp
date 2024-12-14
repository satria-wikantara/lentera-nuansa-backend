//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/utils/pch.h"

#include "nuansa/config/config.h"
#include "nuansa/utils/pattern/circuit_breaker.h"
#include "nuansa/utils/validation.h"


namespace nuansa::config {
    // Returns a reference to the singleton Config instance
    // This implementation ensures:
    // 1. Only one instance is created (lazy initialization)
    // 2. Thread-safe initialization (C++11 magic statics)
    // 3. Automatic cleanup when program exits
    Config &Config::GetInstance() {
        // Static local instance is initialized only once on first call
        // Subsequent calls will return the same instance
        static Config instance;
        return instance;
    }

    void Config::LoadFromFile(const std::string &configPath) {
        if (configPath.empty()) {
            std::cerr << "No configuration file provided." << std::endl;
            return;
        }

        try {
            const YAML::Node config_ = YAML::LoadFile(configPath);

            LoadServerConfig(config_);
            LoadDatabaseConfig(config_);


            LOG_INFO << "Database configuration loaded successfully";
        } catch (const std::exception &e) {
            LOG_ERROR << "Error loading configuration: " << e.what();
            throw;
        }
    }

    void Config::LoadServerConfig(const YAML::Node &config) {
        try {
            if (!config["server"]) {
                throw std::runtime_error("Server configuration section is missing");
            }

            const auto &serverConfig = config["server"];

            // Create a ServerConfig instance
            ServerConfig cfg;

            // Load and validate host
            if (serverConfig["host"]) {
                cfg.host = serverConfig["host"].as<std::string>();
                if (cfg.host.empty()) {
                    throw std::runtime_error("Server host cannot be empty");
                }
            } else {
                cfg.host = "localhost"; // default value
            }

            // Load and validate port
            if (serverConfig["port"]) {
                cfg.port = serverConfig["port"].as<uint16_t>();
                if (cfg.port < 1024 || cfg.port > 65535) {
                    throw std::runtime_error("Server port must be between 1024 and 65535");
                }
            } else {
                cfg.port = 8080; // default value
            }

            // Load and validate log level
            if (serverConfig["log_level"]) {
                cfg.logLevel = serverConfig["log_level"].as<std::string>();
                if (const std::vector<std::string> validLevels = {"debug", "info", "warn", "error"};
                    std::ranges::find(validLevels, cfg.logLevel) == validLevels.end()) {
                    throw std::runtime_error("Invalid log level. Must be one of: debug, info, warn, error");
                }
            } else {
                cfg.logLevel = "info"; // default value
            }

            // Load and validate log path
            if (serverConfig["log_path"]) {
                cfg.logPath = serverConfig["log_path"].as<std::string>();
            } else {
                cfg.logPath = "logs/lentera.log"; // default value
            }

            if (serverConfig["github"]) {
                const auto& githubConfig = serverConfig["github"];
                cfg.githubClientId = githubConfig["client_id"].as<std::string>();
                cfg.githubClientSecret = githubConfig["client_secret"].as<std::string>();
                cfg.githubRedirectUri = githubConfig["redirect_uri"].as<std::string>();
                cfg.githubApiUrl = githubConfig["api_url"].as<std::string>();
                cfg.githubTokenValidationUrl = githubConfig["token_validation_url"].as<std::string>();
                cfg.githubUserApiUrl = githubConfig["user_api_url"].as<std::string>();
                cfg.githubUserEmailsUrl = githubConfig["user_emails_url"].as<std::string>();
            }

            if (serverConfig["google"]) {
                const auto& googleConfig = serverConfig["google"];
                cfg.googleClientId = googleConfig["client_id"].as<std::string>();
                cfg.googleClientSecret = googleConfig["client_secret"].as<std::string>();
                cfg.googleRedirectUri = googleConfig["redirect_uri"].as<std::string>();
                cfg.googleTokenInfoUrl = googleConfig["token_info_url"].as<std::string>();
                cfg.googleUserInfoUrl = googleConfig["user_info_url"].as<std::string>();
            }


            // Store the validated config
            serverConfig_ = cfg;
        } catch (const YAML::Exception &e) {
            throw std::runtime_error("Error parsing server configuration: " + std::string(e.what()));
        }
    }

    void Config::LoadDatabaseConfig(const YAML::Node &config) {
        try {
            const YAML::Node &dbConfig = config["database"];
            if (!dbConfig) {
                throw std::runtime_error("Database configuration section is missing");
            }

            DatabaseConfig cfg;

            // Load and validate host
            if (dbConfig["host"]) {
                const auto host = dbConfig["host"].as<std::string>();
                cfg.host = ResolveEnvironmentVariable(host);
                if (cfg.host.empty()) {
                    throw std::runtime_error("Database host cannot be empty");
                }
            }

            // Load and validate port
            if (dbConfig["port"]) {
                const auto port = dbConfig["port"].as<std::string>();
                cfg.port = std::stoi(ResolveEnvironmentVariable(port));
                if (cfg.port < 1 || cfg.port > 65535) {
                    throw std::runtime_error("Database port must be between 1 and 65535");
                }
            }

            // Load and validate database name
            if (dbConfig["name"]) {
                const auto database_name = dbConfig["name"].as<std::string>();
                cfg.database_name = ResolveEnvironmentVariable(database_name);
                if (cfg.database_name.empty()) {
                    throw std::runtime_error("Database name cannot be empty");
                }
            }

            // Load and validate username
            if (dbConfig["user"]) {
                const auto username = dbConfig["user"].as<std::string>();
                cfg.username = ResolveEnvironmentVariable(username);
                if (cfg.username.empty()) {
                    throw std::runtime_error("Database username cannot be empty");
                }
            }

            // Load and validate password
            if (dbConfig["password"]) {
                const auto password = dbConfig["password"].as<std::string>();
                cfg.password = ResolveEnvironmentVariable(password);
            }

            // Load and validate connection pool size
            if (dbConfig["pool_size"]) {
                cfg.pool_size = dbConfig["pool_size"].as<size_t>();
                if (cfg.pool_size < 1) {
                    throw std::runtime_error("Database pool size must be at least 1");
                }
            }

            // Store the validated config
            databaseConfig_ = cfg;

            // Build the connection string
            BuildConnectionString();
        } catch (const YAML::Exception &e) {
            throw std::runtime_error("Error parsing database configuration: " + std::string(e.what()));
        }
    }

    void Config::SetDatabaseConfig(const DatabaseConfig &config) {
        databaseConfig_ = config;
        BuildConnectionString();
    }

    void Config::SetServerConfig(const ServerConfig &config) {
        serverConfig_ = config;
    }

    std::string Config::ResolveEnvironmentVariable(const std::string &value) {
        if (value.empty() || value[0] != '$') {
            return value;
        }

        // Remove the leading '$' from the variable name
        std::string envVar = value;
        if (value.size() > 3 && value[1] == '{' && value[value.size() - 1] == '}') {
            envVar = value.substr(2, value.size() - 3);
        } else {
            envVar = value.substr(1);
        }

        const char *envValue = std::getenv(envVar.c_str());

        if (!envValue) {
            throw std::runtime_error("Environment variable not set: " + envVar + " (value: " + value + ")");
        }

        return envValue;
    }

    void Config::LoadEnvironmentFile() {
        const char* envPath = std::getenv("ENV_PATH");
        std::string path = envPath ? envPath : ".env";

        try {
            // Strict validation options for security
            nuansa::utils::Validation::PathValidationOptions options;
            options.maxFileSize = 1024 * 1024;  // 1MB limit
            options.mustBeRegularFile = true;
            options.checkWorldReadable = true;
            options.allowOutsideBaseDir = false;
            options.allowedExtensions = {".env"};  // Only allow .env files
            options.baseDir = std::filesystem::current_path();  // Restrict to current directory

            auto normalizedPath = nuansa::utils::Validation::NormalizePath(path, options);
            
            if (!std::filesystem::exists(normalizedPath)) {
                LOG_WARNING << "Environment file not found: " << normalizedPath.string();
                return;
            }

            std::ifstream file(normalizedPath, std::ios::in | std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open file: " + normalizedPath.string());
            }

            std::string line;

            while (std::getline(file, line)) {
                // Skip comments and empty lines
                if (line.empty() || line[0] == '#') {
                    continue;
                }

                // Find the position of the first '='
                auto pos = line.find('=');
                if (pos == std::string::npos) continue;

                // Extract key and value
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);

                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);

                // Remove quotes if present
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }

                // Set the environment variable
#ifdef _WIN32
        _putenv_s(key.c_str(), value.c_str());
#else
                setenv(key.c_str(), value.c_str(), 1);
#endif
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to load environment file: " << e.what();
            throw;
        }
    }

    void Config::BuildConnectionString() {
        std::stringstream ss;
        ss << "postgresql://";

        if (!databaseConfig_.username.empty()) {
            ss << databaseConfig_.username;
            if (!databaseConfig_.password.empty()) {
                ss << ":" << databaseConfig_.password;
            }
            ss << "@";
        }

        // Add host, port, and database name
        ss << databaseConfig_.host << ":" << databaseConfig_.port << "/" << databaseConfig_.database_name;

        // Optionally add additional parameters
        // ss << "?sslmode=verify-full";  // Example of adding SSL mode

        databaseConfig_.connection_string = ss.str();

        BOOST_LOG_TRIVIAL(debug) << "Built PostgreSQL connection string (credentials masked)";
    }

    void Config::Initialize(const std::string &configFile) {
        LoadEnvironmentFile();
        LoadFromFile(configFile);
    }
} // namespace nuansa::config
