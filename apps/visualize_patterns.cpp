#include <iostream>
#include <vector>
#include <set>

using Pattern = std::vector<int>;

void print_pattern(const Pattern& pattern, const std::string& name) {
    std::set<int> cells(pattern.begin(), pattern.end());
    
    std::cout << name << ": [";
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << pattern[i];
    }
    std::cout << "]\n";
    
    // 盤面表示
    for (int y = 0; y < 5; ++y) {
        std::cout << "  ";
        for (int x = 0; x < 5; ++x) {
            int cell = y * 5 + x;
            if (cells.count(cell)) {
                std::cout << "■ ";
            } else {
                std::cout << "□ ";
            }
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    std::vector<Pattern> patterns = {
        {0, 1, 2, 3, 4, 5, 6, 7, 8},      // Pattern #1: 3x3 正方形
        {0, 1, 2, 3, 4, 5, 6, 10, 11},    // Pattern #13: L字型
        {0, 1, 2, 3, 4, 5, 10, 15, 20},   // Pattern #41: T字型
        {0, 1, 2, 3, 4, 6, 11, 16, 21},   // Pattern #61: 斜め
        {0, 1, 2, 3, 4, 7, 12, 17, 22},   // Pattern #67: 斜め逆
        {0, 1, 2, 3, 5, 6, 7, 10, 11},    // Pattern #71: 十字型
        {0, 1, 2, 5, 6, 7, 10, 11, 12},   // Pattern #283: 十字型2
        {0, 1, 5, 6, 10, 11, 15, 16, 20}, // Pattern #883: 縦長
    };
    
    std::vector<std::string> names = {
        "Pattern #1  (3x3 square)",
        "Pattern #13 (L-shape)",
        "Pattern #41 (T-shape)",
        "Pattern #61 (diagonal-1)",
        "Pattern #67 (diagonal-2)",
        "Pattern #71 (cross-1)",
        "Pattern #283 (cross-2)",
        "Pattern #883 (vertical)",
    };
    
    std::cout << "========================================\n";
    std::cout << "Selected N-tuple Patterns\n";
    std::cout << "========================================\n\n";
    
    for (size_t i = 0; i < patterns.size(); ++i) {
        print_pattern(patterns[i], names[i]);
    }
    
    return 0;
}
