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
    
    int total_with_translation = 0;
    int total_without_translation = base_patterns.size();
    
    std::cout << "========================================\n";
    std::cout << "平行移動の追加によるメモリ使用量の比較\n";
    std::cout << "========================================\n\n";
    
    std::cout << "【現在の設定（平行移動なし）】\n";
    std::cout << "パターン数: " << total_without_translation << "\n\n";
    
    std::cout << "【平行移動を追加した場合】\n";
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
        
        // 平行移動の数（5x5盤面ではなく6x5という認識で計算）
        int translations = (6 - width) * (5 - height);
        if (translations == 0) translations = 1; // 移動できない場合も1パターン
        
        total_with_translation += translations;
        
        std::cout << pattern.name << ":\n";
        std::cout << "  サイズ: " << width << "x" << height << "\n";
        std::cout << "  平行移動パターン数: " << translations << "\n";
    }
    
    // 各パターンのメモリ使用量
    long long states_per_pattern = 1;
    for (int i = 0; i < 9; i++) {
        states_per_pattern *= 9;
    }
    
    double gb_per_pattern = states_per_pattern * sizeof(float) / (1024.0 * 1024.0 * 1024.0);
    
    double mem_without = total_without_translation * gb_per_pattern;
    double mem_with = total_with_translation * gb_per_pattern;
    double ratio = mem_with / mem_without;
    
    std::cout << "\n========================================\n";
    std::cout << "メモリ使用量の比較\n";
    std::cout << "========================================\n";
    std::cout << "1パターンあたり: " << gb_per_pattern << " GB\n\n";
    
    std::cout << "【平行移動なし】\n";
    std::cout << "  パターン数: " << total_without_translation << "\n";
    std::cout << "  メモリ: " << mem_without << " GB\n\n";
    
    std::cout << "【平行移動あり】\n";
    std::cout << "  パターン数: " << total_with_translation << "\n";
    std::cout << "  メモリ: " << mem_with << " GB\n\n";
    
    std::cout << "倍率: " << ratio << "倍\n";
    std::cout << "増加量: +" << (mem_with - mem_without) << " GB\n\n";
    
    if (mem_with > 32) {
        std::cout << "⚠️  WARNING: 32GB以上のメモリが必要です！\n";
        std::cout << "    → 通常のPCでは実行不可能\n";
    } else if (mem_with > 16) {
        std::cout << "⚠️  WARNING: 16GB以上のメモリが必要です！\n";
        std::cout << "    → ハイエンドPCが必要\n";
    } else if (mem_with > 8) {
        std::cout << "⚠️  CAUTION: 8GB以上のメモリが必要です\n";
        std::cout << "    → 他のアプリケーションを閉じる必要があります\n";
    } else {
        std::cout << "✓ 一般的なPCでも実行可能な範囲です\n";
    }
    
    return 0;
}
