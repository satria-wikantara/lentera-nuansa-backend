//
// Created by panca on 11/6/24.
//

#ifndef PROGRAM_OPTIONS_H
#define PROGRAM_OPTIONS_H

#include <string>

namespace App {

    struct ProgramOptions {
      bool verbose{false};
      std::string command;
      std::string configFile;
        bool runDebug{false};
    };

    ProgramOptions ParseCommandLine(int argc, char* argv[]);
    void CheckForRootUser();

} // namespace App

#endif //PROGRAM_OPTIONS_H

