#include <iostream>
#include <vector>
#include <algorithm>

int main() {
    // 各基本パターンの情報
    struct PatternInfo {
        std::string name;
        std::vector<int> cells;
    };
    
    std::vector<PatternInfo> base_patterns = {
        {"3x3 square",      {0, 1, 2, 3, 4, 5, 6, 7, 8}},
        {"L-shape (0°)",    {0, 1, 2, 3, 4, 5, 6, 10, 11}},
        {"L-shape (270°)",  {0, 5, 10, 15, 16, 17, 20, 21, 22}},
        {"T-shape",         {0, 1, 2, 3, 4, 5, 10, 15, 20}},
        {"Diagonal-1",      {0, 1, 2, 3, 4, 6, 11, 16, 21}},
        {"Diagonal-2",      {0, 1, 2, 3, 4, 7, 12, 17, 22}},
        {"Cross-1 (0°)",    {0, 1, 2, 3, 5, 6, 7, 10, 11}},
        {"Cross-1 (270°)",  {0, 5, 6, 10, 11, 12, 15, 16, 17}},
        {"Cross-2",         {0, 1, 2, 5, 6, 7, 10, 11, 12}},
        {"Vertical",        {0, 1, 5, 6, 10, 11, 15, 16, 20}},
    };
    
    int total_patterns = 0;
    
    std::cout << "========================================\n";
    std::cout << "Memory Estimation for N-tuple Network\n";
    std::cout << "========================================\n\n";
    
    for (const auto& pattern : base_patterns) {
        // パターンの境界を計算
        int min_x = 5, max_x = -1, min_y = 5, max_y = -1;
        for (int cell : pattern.cells) {
            int x = cell % 5;
            int y = cell / 5;
            min_x = std::min(min_x, x);
            max_x = std::max(max_x, x);
            min_y = std::min(min_y, y);
            max_y = std::max(max_y, y);
        }
        
        int width = max_x - min_x + 1;
        int height = max_y - min_y + 1;
        
        // 平行移動の数
        int translations = (6 - width) * (5 - height);
        total_patterns += translations;
        
        std::cout << pattern.name << ":\n";
        std::cout << "  Size: " << width << "x" << height << "\n";
        std::cout << "  Possible translations: " << translations << "\n";
        std::cout << "\n";
    }
    
    // 各パターンのメモリ使用量
    long long states_per_pattern = 1;
    for (int i = 0; i < 9; i++) {
        states_per_pattern *= 9;
    }
    
    double gb_per_pattern = states_per_pattern * sizeof(float) / (1024.0 * 1024.0 * 1024.0);
    double total_gb = total_patterns * gb_per_pattern;
    
    std::cout << "========================================\n";
    std::cout << "Summary\n";
    std::cout << "========================================\n";
    std::cout << "Total patterns (with translations): " << total_patterns << "\n";
    std::cout << "States per pattern: " << states_per_pattern << "\n";
    std::cout << "Memory per pattern: " << gb_per_pattern << " GB\n";
    std::cout << "Total memory: " << total_gb << " GB\n";
    std::cout << "\n";
    
    if (total_gb > 16) {
        std::cout << "⚠️  WARNING: Memory usage exceeds typical RAM capacity!\n";
        std::cout << "Consider reducing the number of patterns or using smaller patterns.\n";
    } else if (total_gb > 8) {
        std::cout << "⚠️  CAUTION: High memory usage. May require 16GB+ RAM.\n";
    } else {
        std::cout << "✓ Memory usage is reasonable for most systems.\n";
    }
    
    return 0;
}
