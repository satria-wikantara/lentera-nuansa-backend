#ifndef NUANSA_UTILS_PROGRAM_OPTIONS_H
#define NUANSA_UTILS_PROGRAM_OPTIONS_H


#include "nuansa/utils/pch.h"

namespace nuansa::utils {
	class ProgramOptions {
	public:
		ProgramOptions(int argc, char *argv[]);

		bool Parse();

		bool Validate() const;

		std::filesystem::path GetConfigFilePath() const { return configFilePath; }
		std::string GetCommand() const { return command; }

	private:
		void InitializeOptions();

		int argc_;
		char **argv_;
		bool verbose{false};
		std::string command;
		std::string configFilePath;

		boost::program_options::positional_options_description positional_options_;
		boost::program_options::options_description descriptions_;
		boost::program_options::variables_map variables_;
	};
}

#endif // NUANSA_UTILS_PROGRAM_OPTIONS_H
