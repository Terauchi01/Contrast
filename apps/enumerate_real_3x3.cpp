#include <iostream>
#include <vector>
#include <set>

using Pattern = std::vector<int>;

void print_pattern_code(const Pattern& pattern, const std::string& comment) {
    std::cout << "    {";
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << pattern[i];
    }
    std::cout << "},  " << comment << "\n";
}

void print_pattern_visual(const Pattern& pattern, const std::string& name) {
    std::set<int> cells(pattern.begin(), pattern.end());
    
    std::cout << name << ":\n";
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
    // 真の3x3正方形パターン
    Pattern base_3x3 = {0, 1, 2, 5, 6, 7, 10, 11, 12};
    
    std::cout << "========================================\n";
    std::cout << "真の3x3正方形パターンの全平行移動\n";
    std::cout << "========================================\n\n";
    
    std::cout << "基本パターン:\n";
    print_pattern_visual(base_3x3, "Base 3x3");
    
    // サイズ確認
    int min_x = 5, max_x = -1, min_y = 5, max_y = -1;
    for (int cell : base_3x3) {
        int x = cell % 5;
        int y = cell / 5;
        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
    }
    
    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;
    
    std::cout << "パターンサイズ: " << width << "x" << height << "\n";
    std::cout << "5x5盤面での配置可能数: " << (6 - width) << " × " << (5 - height) << " = " 
              << (6 - width) * (5 - height) << "\n\n";
    
    std::cout << "========================================\n";
    std::cout << "全パターンのコード（コピペ用）:\n";
    std::cout << "========================================\n";
    
    int count = 0;
    for (int dy = 0; dy <= 5 - height; ++dy) {
        for (int dx = 0; dx <= 5 - width; ++dx) {
            Pattern shifted;
            
            for (int cell : base_3x3) {
                int x = cell % 5;
                int y = cell / 5;
                int new_x = x - min_x + dx;
                int new_y = y - min_y + dy;
                shifted.push_back(new_y * 5 + new_x);
            }
            
            std::string comment = "// 3x3 at (" + std::to_string(dx) + ", " + std::to_string(dy) + ")";
            print_pattern_code(shifted, comment);
            count++;
        }
    }
    
    std::cout << "\n========================================\n";
    std::cout << "詳細ビジュアル:\n";
    std::cout << "========================================\n\n";
    
    count = 0;
    for (int dy = 0; dy <= 5 - height; ++dy) {
        for (int dx = 0; dx <= 5 - width; ++dx) {
            Pattern shifted;
            
            for (int cell : base_3x3) {
                int x = cell % 5;
                int y = cell / 5;
                int new_x = x - min_x + dx;
                int new_y = y - min_y + dy;
                shifted.push_back(new_y * 5 + new_x);
            }
            
            std::string name = "Position (" + std::to_string(dx) + ", " + std::to_string(dy) + ")";
            print_pattern_visual(shifted, name);
            count++;
        }
    }
    
    std::cout << "========================================\n";
    std::cout << "合計: " << count << " パターン\n";
    std::cout << "メモリ使用量: " << count << " × 1.443 GB = " 
              << (count * 1.44325) << " GB\n";
    std::cout << "========================================\n";
    
    return 0;
}
