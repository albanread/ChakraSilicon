#include <iostream>

int main() {
    std::cout << "Platform detection test:" << std::endl;

#ifdef APPLE_SILICON_JIT
    std::cout << "  APPLE_SILICON_JIT: DEFINED" << std::endl;
#else
    std::cout << "  APPLE_SILICON_JIT: NOT DEFINED" << std::endl;
#endif

#ifdef PROHIBIT_STP_LDP
    std::cout << "  PROHIBIT_STP_LDP: DEFINED" << std::endl;
#else
    std::cout << "  PROHIBIT_STP_LDP: NOT DEFINED" << std::endl;
#endif

#ifdef USE_INDIVIDUAL_STACK_OPERATIONS
    std::cout << "  USE_INDIVIDUAL_STACK_OPERATIONS: DEFINED" << std::endl;
#else
    std::cout << "  USE_INDIVIDUAL_STACK_OPERATIONS: NOT DEFINED" << std::endl;
#endif

#ifdef __APPLE__
    std::cout << "  __APPLE__: DEFINED" << std::endl;
#endif

#ifdef __arm64__
    std::cout << "  __arm64__: DEFINED" << std::endl;
#endif

    std::cout << "Platform detection test complete." << std::endl;
    return 0;
}
