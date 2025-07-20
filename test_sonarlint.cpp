// Test file to verify SonarLint is working
#include <iostream>

int main() {
    // This should trigger some SonarLint warnings:
    
    // Unused variable (should be flagged)
    int unused_variable = 42;
    
    // Magic number (should be flagged)
    if (5 > 3) {
        std::cout << "This uses magic numbers" << std::endl;
    }
    
    // Empty catch block (should be flagged)
    try {
        int x = 10 / 0;
    } catch (...) {
        // Empty catch block - bad practice
    }
    
    return 0;
}
