//
// Created by I Gede Panca Sutresna on 05/12/24.
//
#include "nuansa/pch/pch.h"

#include "nuansa/utils/program_options.h"
#include "nuansa/utils/option_constants.h"

namespace nuansa::utils {
    ProgramOptions::ProgramOptions(int argc, char *argv[])
        : argc_(argc)
          , argv_(argv)
          , descriptions_("Allowed options")
          , positional_options_()
          , variables_() {
        InitializeOptions();
    }

    void ProgramOptions::InitializeOptions() {
        descriptions_.add_options()
                ("help,h", "Display this help message")
                ("verbose,v", boost::program_options::bool_switch(&verbose), "Enable verbose output")
                ("command", boost::program_options::value<std::string>(&command), "Command to execute")
                ("config,c", boost::program_options::value<std::string>(&configFilePath)->default_value("config.yml"),
                 "Path to configuration file");
    }

    bool ProgramOptions::Parse() {
        try {
            // Parse positional options
            positional_options_.add("command", 1); // Required positional argument
            positional_options_.add("config", 1); // Optional positional argument

            // Store options on command line
            boost::program_options::store(boost::program_options::command_line_parser(argc_, argv_)
                                          .options(descriptions_)
                                          .positional(positional_options_)
                                          .run(), variables_);

            // Notify the user of any invalid options
            boost::program_options::notify(variables_);

            // If help was specified, display it and return false
            if (variables_.count("help")) {
                std::cout << descriptions_ << std::endl;
                return false;
            }

            return true;
        } catch (const boost::program_options::error &e) {
            std::cerr << "Error parsing command line options: " << e.what() << std::endl;
            std::cout << descriptions_ << std::endl;
            return false;
        }
    }

    bool ProgramOptions::Validate() const {
        try {
            // Validate config file path
            if (variables_.count("config")) {
                std::filesystem::path configFilePath = GetConfigFilePath();
                if (!std::filesystem::exists(configFilePath)) {
                    std::cerr << "Config file does not exist: " << configFilePath << std::endl;
                    return false;
                }
            }

            return true;
        } catch (const std::filesystem::filesystem_error &e) {
            std::cerr << "Filesystem error: " << e.what() << std::endl;
            return false;
        } catch (const std::exception &e) {
            std::cerr << "Error validating command line options: " << e.what() << std::endl;
            return false;
        }
    }
}
