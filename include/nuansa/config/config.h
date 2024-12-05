#ifndef NUANSA_CONFIG_CONFIG_H
#define NUANSA_CONFIG_CONFIG_H

#include "nuansa/pch/pch.h"

#include "nuansa/config/config_types.h"

namespace nuansa::config {
	class Config {
	public:
		Config() = default;

		Config(const Config &) = delete;

		Config &operator=(const Config &) = delete;

		static Config &GetInstance();

		void Initialize(const std::string &configFile);

		const ServerConfig &GetServerConfig() const { return serverConfig_; }
		const DatabaseConfig &GetDatabaseConfig() const { return databaseConfig_; }

		void SetDatabaseConfig(const DatabaseConfig &config);

		void SetServerConfig(const ServerConfig &config);

		// Other Getters as needed
		const YAML::Node &GetRawConfig() const { return config_; }

	private:
		void LoadFromFile(const std::string &configPath);

		void LoadServerConfig(const YAML::Node &config);

		void LoadDatabaseConfig(const YAML::Node &config);

		std::string ResolveEnvironmentVariable(const std::string &value) const;

		void LoadEnvironmentFile();

		void BuildConnectionString();

		ServerConfig serverConfig_;
		DatabaseConfig databaseConfig_;

		// Raw Configuration
		YAML::Node config_;

		// Command line options
		boost::program_options::options_description options_{"Allowed options"};
		boost::program_options::variables_map vm_;
	};

	// Global accessor functions
	inline Config &GetConfig() { return Config::GetInstance(); }
} // namespace nuansa::config

#endif // NUANSA_CONFIG_CONFIG_H
