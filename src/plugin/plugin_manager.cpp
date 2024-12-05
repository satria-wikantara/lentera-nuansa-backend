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
		BOOST_LOG_TRIVIAL(debug) << "Plugin manager handling message from " << sender;

		try {
			std::string pluginName = message["plugin"].get<std::string>();
			auto it = plugins.find(pluginName);

			if (it != plugins.end()) {
				it->second->HandleMessage(sender, message);
			} else {
				BOOST_LOG_TRIVIAL(warning) << "Plugin '" << pluginName << "' not found";
			}
		} catch (const std::exception &e) {
			BOOST_LOG_TRIVIAL(error) << "Error handling plugin message: " << e.what();
		}
	}
}
