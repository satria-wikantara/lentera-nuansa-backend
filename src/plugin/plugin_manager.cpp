//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/utils/pch.h"

#include "nuansa/plugin/plugin_manager.h"

namespace nuansa::plugin {
	PluginManager &PluginManager::GetInstance() {
		static PluginManager instance;
		return instance;
	}

	// TODO: Implement this
	void PluginManager::LoadPlugin(std::shared_ptr<IPlugin> plugin) {
	}

	// TODO: Implement this
	void PluginManager::UnloadPlugin(const std::string &pluginName) {
	}

	void PluginManager::HandleMessage(const std::string &sender, const nlohmann::json &message) {
		LOG_DEBUG << "Plugin manager handling message from " << sender;

		try {
			auto pluginName = message["plugin"].get<std::string>();

			if (const auto it = plugins.find(pluginName); it != plugins.end()) {
				it->second->HandleMessage(sender, message);
			} else {
				LOG_WARNING << "Plugin '" << pluginName << "' not found";
			}
		} catch (const std::exception &e) {
			LOG_ERROR << "Error handling plugin message: " << e.what();
		}
	}

	template<typename T>
	std::shared_ptr<T> PluginManager::GetPlugin(const std::string &name) {
		return nullptr;
	}
}
