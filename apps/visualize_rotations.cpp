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

// 90度回転（時計回り）: (x, y) -> (4-y, x)
Pattern rotate_90(const Pattern& pattern) {
    Pattern rotated;
    for (int cell : pattern) {
        int x = cell % 5;
        int y = cell / 5;
        int new_x = 4 - y;
        int new_y = x;
        rotated.push_back(new_y * 5 + new_x);
    }
    std::sort(rotated.begin(), rotated.end());
    return rotated;
}

// パターンを正規化（最小座標を原点に）
Pattern normalize(const Pattern& pattern) {
    int min_x = 5, min_y = 5;
    for (int cell : pattern) {
        int x = cell % 5;
        int y = cell / 5;
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
    }
    
    Pattern normalized;
    for (int cell : pattern) {
        int x = cell % 5;
        int y = cell / 5;
        normalized.push_back((y - min_y) * 5 + (x - min_x));
    }
    std::sort(normalized.begin(), normalized.end());
    return normalized;
}

int main() {
    Pattern l_shape = {0, 1, 2, 3, 4, 5, 6, 10, 11};
    Pattern cross_1 = {0, 1, 2, 3, 5, 6, 7, 10, 11};
    
    std::cout << "========================================\n";
    std::cout << "L-shape Pattern Rotations\n";
    std::cout << "========================================\n\n";
    
    print_pattern(l_shape, "Original L-shape");
    
    Pattern l_90 = normalize(rotate_90(l_shape));
    print_pattern(l_90, "L-shape rotated 90°");
    
    Pattern l_180 = normalize(rotate_90(l_90));
    print_pattern(l_180, "L-shape rotated 180°");
    
    Pattern l_270 = normalize(rotate_90(l_180));
    print_pattern(l_270, "L-shape rotated 270°");
    
    std::cout << "========================================\n";
    std::cout << "Cross-1 Pattern Rotations\n";
    std::cout << "========================================\n\n";
    
    print_pattern(cross_1, "Original Cross-1");
    
    Pattern c_90 = normalize(rotate_90(cross_1));
    print_pattern(c_90, "Cross-1 rotated 90°");
    
    Pattern c_180 = normalize(rotate_90(c_90));
    print_pattern(c_180, "Cross-1 rotated 180°");
    
    Pattern c_270 = normalize(rotate_90(c_180));
    print_pattern(c_270, "Cross-1 rotated 270°");
    
    return 0;
}
