//
// Created by panca on 11/6/24.
//

#include "../include/app.h"
#include "../include/init.h"
#include "../include/global_config.h"
#include "../include/program_options.h"

int main(int argc, char *argv[]) {
     App::CheckForRootUser();
     App::InitializeLogging();

     App::ProgramOptions options = App::ParseCommandLine(argc, argv);
     App::g_runDebug = options.runDebug;

     App::Run(options);

     return 0;
}
