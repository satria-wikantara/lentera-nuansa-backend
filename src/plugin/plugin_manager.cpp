//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/pch/pch.h"

#include "nuansa/plugin/plugin_manager.h"

namespace nuansa::plugin {
	PluginManager &PluginManager::GetInstance() {
		static PluginManager instance;
		return instance;
	}

	void PluginManager::HandleMessage(const std::string &sender, const nlohmann::json &message) {
		LOG_DEBUG << "Plugin manager handling message from " << sender;

		try {
			std::string pluginName = message["plugin"].get<std::string>();
			auto it = plugins.find(pluginName);

			if (it != plugins.end()) {
				it->second->HandleMessage(sender, message);
			} else {
				LOG_WARNING << "Plugin '" << pluginName << "' not found";
			}
		} catch (const std::exception &e) {
			LOG_ERROR << "Error handling plugin message: " << e.what();
		}
	}
}
