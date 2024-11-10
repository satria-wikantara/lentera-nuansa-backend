//
// Created by panca on 11/6/24.
//

#include "../include/program_options.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <unistd.h>

namespace po = boost::program_options;

namespace App {
    void CheckForRootUser() {
      	if (getuid() == 0) {
           std::cerr << "Warning: running as root is not recommended. Please use a non-root user." << std::endl;
      	}
    }

    ProgramOptions ParseCommandLine(int argc, char* argv[]) {
		ProgramOptions options;

			try {
				po::options_description desc("Allowed options");

				desc.add_options()
				("help,h", "Display this help message")
				("verbose,v", po::bool_switch(&options.verbose), "Enable verbose output")
				("command", po::value<std::string>(&options.command), "Command to execute")
				("config,c", po::value<std::string>(&options.configFile), "Configuration file")
				("debug,d", po::bool_switch(&options.runDebug), "Run in debug mode");

				po::positional_options_description p;
				p.add("command", 1);
                p.add("config", 1);

                po::variables_map vm;
                po::store(po::command_line_parser(argc, argv)
					.options(desc)
                	.positional(p)
                	.run(), vm);
                po::notify(vm);

                if (vm.count("help")) {
                	std::cout << desc << std::endl;
					std::exit(0);
                }
			} catch (const po::error& e) {
				std::cerr << "Error: " << e.what() << std::endl;
				std::exit(1);
			}
		return options;
    }
};
