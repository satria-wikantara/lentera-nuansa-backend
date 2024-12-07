#include "nuansa/utils/program_options.h"
#include "nuansa/core/app.h"

int main(const int argc, char *argv[]) {
     nuansa::utils::ProgramOptions options(argc, argv);

     if (!options.Parse() || !options.Validate()) {
          return 1;
     }

     // Run the application
     nuansa::core::Run(options);

     return 0;
}
