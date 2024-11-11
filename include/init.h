//
// Created by I Gede Panca Sutresna on 10/11/24.
//

#ifndef INIT_H
#define INIT_H

namespace App {
    bool InitializeConfig(const std::string& configPath);
    bool CheckForRootUser();
    void InitializeLogging();
}

#endif //INIT_H
