#include <gtest/gtest.h>

#ifdef LLVM_PROFILE_FILE
extern "C" {
    int __llvm_profile_dump(void);
}
#endif

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    // Initialize test database
    // nuansa::tests::utils::InitTestDatabase();

    int testResult = RUN_ALL_TESTS();

#ifdef LLVM_PROFILE_FILE
    __llvm_profile_dump(); // Dump coverage data before exit
#endif

    return testResult;
}
