//
// Created by panca on 11/6/24.
//

#ifndef NUANSA_CORE_APP_H
#define NUANSA_CORE_APP_H

#include "nuansa/pch/pch.h"
#include "nuansa/utils/program_options.h"

namespace nuansa::core {
	bool CheckForRootUser();

	bool Initialize(const std::string &configPath);

	bool InitializeConfig(const std::string &configPath);

	bool InitializeLogging();

	bool InitializeDatabase();

	void Run(const nuansa::utils::ProgramOptions &options);
} // namespace App

#endif  // NUANSA_CORE_APP_H
