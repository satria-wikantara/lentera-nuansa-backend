//
// Created by I Gede Panca Sutresna on 01/12/24.
//

#ifndef NUANSA_CONFIG_CONFIG_H
#define NUANSA_CONFIG_CONFIG_H

#include <yaml-cpp/node/node.h>

#include "nuansa/pch/pch.h"
#include "nuansa/config/config_types.h"

namespace nuansa::config {
	class Config {
	public:
		Config() = default;

		Config(const Config &) = delete;

		Config &operator=(const Config &) = delete;

		static Config &instance();

		void Initialize(const std::string &configFile);

		const ServerConfig &GetServerConfig() const { return serverConfig_; }

		const DatabaseConfig &GetDatabaseConfig() const { return databaseConfig_; }

		void SetServerConfig(const ServerConfig &config);

		void SetDatabaseConfig(const DatabaseConfig &config);

		const YAML::Node &GetRawConfig() const { return config_; }

	private:
		YAML::Node config_;
		ServerConfig serverConfig_;
		DatabaseConfig databaseConfig_;

		// Command line options
		boost::program_options::options_description options_{"Allowed options"};
		boost::program_options::variables_map vm_;

		void LoadFromFile(const std::string &configPath);

		void LoadServerConfig(const YAML::Node &config);

		void LoadDatabaseConfig(const YAML::Node &config);

		std::string ResolveEnvironmentVariable(const std::string &value) const;

		void LoadEnvironmentFile();

		void BuildConnectionString();
	};

	inline Config &GetConfig() { return Config::GetInstance(); }
}

#endif //NUANSA_CONFIG_CONFIG_H
