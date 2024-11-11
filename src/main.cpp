//
// Created by panca on 11/6/24.
//

#include "../include/app.h"
#include "../include/init.h"
#include "../include/global_config.h"
#include "../include/program_options.h"

int main(int argc, char *argv[]) {
     App::ProgramOptions options = App::ParseCommandLine(argc, argv);

     bool runCheckList = App::CheckForRootUser() && App::InitializeConfig(options.configFile);
     if (!runCheckList) {
          return 1;
     }

     // Initialize logging
     App::InitializeLogging();

     // Run the application
     App::Run(options);

     return 0;
}
