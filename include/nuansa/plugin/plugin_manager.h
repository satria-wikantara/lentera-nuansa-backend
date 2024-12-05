#ifndef NUANSA_PLUGIN_PLUGIN_MANAGER_H
#define NUANSA_PLUGIN_PLUGIN_MANAGER_H

#include "nuansa/pch/pch.h"
#include "nuansa/plugin/iplugin.h"

namespace nuansa::plugin {

	class PluginManager {
	public:
		static PluginManager& GetInstance();

		void LoadPlugin(std::shared_ptr<IPlugin> plugin);
		void UnloadPlugin(const std::string& pluginName);
		void HandleMessage(const std::string& sender, const nlohmann::json& message);

		template<typename T>
		std::shared_ptr<T> GetPlugin(const std::string& name);

	private:
		PluginManager() = default;
		std::unordered_map<std::string, std::shared_ptr<IPlugin>> plugins;
		std::mutex pluginMutex;

	};
}

#endif // NUANSA_PLUGIN_PLUGIN_MANAGER_H