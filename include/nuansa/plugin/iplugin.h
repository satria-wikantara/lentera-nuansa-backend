#ifndef NUANSA_PLUGIN_IPLUGIN_H
#define NUANSA_PLUGIN_IPLUGIN_H

#include "nuansa/pch/pch.h"

namespace nuansa::plugin {
	class IPlugin {
	public:
		// Virtual destructor for proper cleanup
		virtual ~IPlugin() = default;

		// Core plugin lifecycle methods
		virtual void OnLoad() = 0;
		virtual void OnUnload() = 0;

		// Message handling
		virtual void HandleMessage(const std::string& sender, const nlohmann::json& message) = 0;

		// Plugin metadata
		virtual std::string GetName() const = 0;
		virtual std::string GetVersion() const = 0;
		virtual std::string GetDescription() const = 0;
	};
}

#endif // NUANSA_PLUGIN_IPLUGIN_H
