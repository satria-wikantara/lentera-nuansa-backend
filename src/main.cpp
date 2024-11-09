//
// Created by panca on 11/6/24.
//

#include "../include/app.h"

int main(int argc, char *argv[]) {
     App::CheckForRootUser();

     auto options = App::ParseCommandLine(argc, argv);

     App::Run(options);

     return 0;
}
