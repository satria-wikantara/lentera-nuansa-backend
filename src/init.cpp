//
// Created by I Gede Panca Sutresna on 10/11/24.
//
#include <iostream>
#include <unistd.h>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "../include/global_config.h"
#include "../include/init.h"

namespace App {
    void CheckForRootUser() {
        if (getuid() == 0) {
            std::cerr << "Warning: running as root is not recommended. Please use a non-root user." << std::endl;
        }
    }

    void InitializeLogging() {
        boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

        // Only show debug messages when g_runDebug is true
        boost::log::core::get()->set_filter(
            boost::log::trivial::severity >= (App::g_runDebug ?
                boost::log::trivial::debug : boost::log::trivial::info)
        );

        boost::log::add_console_log(
            std::cout,
            boost::log::keywords::format = "[%TimeStamp%] [%Severity%] %Message%"
        );
        boost::log::add_common_attributes();
    }
}