#include "contrast/board.hpp"
#include "contrast/symmetry.hpp"
#include <iostream>
#include <vector>
#include <set>
#include <algorithm>
#include <cmath>
#include <iomanip>

using namespace contrast;

/**
 * N-tuple Pattern Designer
 * 
 * N個のセルを選んでパターンを作成し、対称性を考慮して一意なパターンを列挙
 */

// パターンを表す型（セルインデックスのリスト）
using Pattern = std::vector<int>;

// パターンを正規化（平行移動と左右反転を考慮して最小形に変換）
Pattern normalize_pattern(const Pattern& pattern) {
    std::vector<Pattern> candidates;
    
    // 左右反転の2通り
    for (int flip = 0; flip < 2; ++flip) {
        Pattern transformed;
        for (int cell : pattern) {
            int x = cell % 5;
            int y = cell / 5;
            
            // 左右反転を適用
            int tx = (flip == 1) ? (4 - x) : x;
            int ty = y;
            
            transformed.push_back(ty * 5 + tx);
        }
        
        // 最も左上に寄せる（正規化）
        // 1. 最小のx座標を見つける
        int min_x = 5, min_y = 5;
        for (int cell : transformed) {
            int x = cell % 5;
            int y = cell / 5;
            if (x < min_x) min_x = x;
            if (y < min_y) min_y = y;
        }
        
        // 2. 原点(0,0)に移動
        Pattern normalized;
        for (int cell : transformed) {
            int x = cell % 5;
            int y = cell / 5;
            int nx = x - min_x;
            int ny = y - min_y;
            normalized.push_back(ny * 5 + nx);
        }
        
        std::sort(normalized.begin(), normalized.end());
        candidates.push_back(normalized);
    }
    
    // 辞書順で最小のものを返す
    return *std::min_element(candidates.begin(), candidates.end());
}

// パターンの全てのセルが隣接しているかチェック（BFSで連結性を確認）
bool is_connected(const Pattern& pattern) {
    if (pattern.empty()) return false;
    if (pattern.size() == 1) return true;
    
    std::set<int> all_cells(pattern.begin(), pattern.end());
    std::set<int> visited;
    std::queue<int> q;
    
    // 最初のセルから開始
    q.push(pattern[0]);
    visited.insert(pattern[0]);
    
    while (!q.empty()) {
        int cell = q.front();
        q.pop();
        
        int x = cell % 5;
        int y = cell / 5;
        
        // 4方向の隣接セルをチェック
        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};
        
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            if (nx < 0 || nx >= 5 || ny < 0 || ny >= 5) continue;
            
            int neighbor = ny * 5 + nx;
            if (all_cells.count(neighbor) && !visited.count(neighbor)) {
                visited.insert(neighbor);
                q.push(neighbor);
            }
        }
    }
    
    // 全てのセルが訪問されたか確認
    return visited.size() == pattern.size();
}

// パターンを視覚化
void print_pattern(const Pattern& pattern, int index = -1) {
    std::set<int> cells(pattern.begin(), pattern.end());
    
    if (index >= 0) {
        std::cout << "Pattern #" << index << ": ";
    }
    std::cout << "[";
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

// N個のセルを選ぶ全組み合わせを生成（再帰）
void generate_combinations(int n, int start, Pattern& current, 
                          std::set<Pattern>& unique_patterns) {
    if (current.size() == static_cast<size_t>(n)) {
        // n個選んだので連結性をチェックして正規化して追加
        if (is_connected(current)) {
            Pattern normalized = normalize_pattern(current);
            unique_patterns.insert(normalized);
        }
        return;
    }
    
    // 残りの選択数
    int remaining = n - current.size();
    
    // startから24まで試す（最後のセル）
    for (int i = start; i <= 25 - remaining; ++i) {
        current.push_back(i);
        generate_combinations(n, i + 1, current, unique_patterns);
        current.pop_back();
    }
}

// 四角形パターンかチェック（より学習しやすい）
bool is_rectangular(const Pattern& pattern) {
    if (pattern.size() < 4) return false;
    
    int min_x = 5, max_x = -1;
    int min_y = 5, max_y = -1;
    
    for (int cell : pattern) {
        int x = cell % 5;
        int y = cell / 5;
        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y);
    }
    
    int width = max_x - min_x + 1;
    int height = max_y - min_y + 1;
    
    return static_cast<size_t>(width * height) == pattern.size();
}

int main(int argc, char* argv[]) {
    int n = 4;  // デフォルトは4セル（2x2）
    
    if (argc > 1) {
        n = std::atoi(argv[1]);
        if (n < 1 || n > 9) {
            std::cerr << "Error: n must be between 1 and 9\n";
            std::cerr << "Usage: " << argv[0] << " <n>\n";
            return 1;
        }
    }
    
    std::cout << "========================================\n";
    std::cout << "N-tuple Pattern Designer\n";
    std::cout << "========================================\n";
    std::cout << "Generating all unique patterns with " << n << " cells\n";
    std::cout << "Board size: 5x5 (25 cells total)\n";
    std::cout << "Considering 8-fold symmetry\n\n";
    
    // 全組み合わせを生成
    std::set<Pattern> unique_patterns;
    Pattern current;
    
    std::cout << "Generating combinations...\n";
    generate_combinations(n, 0, current, unique_patterns);
    
    std::cout << "Total unique patterns (with symmetry): " << unique_patterns.size() << "\n";
    
    // 連結パターンのみフィルタ
    std::vector<Pattern> connected_patterns;
    for (const auto& p : unique_patterns) {
        if (is_connected(p)) {
            connected_patterns.push_back(p);
        }
    }
    
    std::cout << "Connected patterns: " << connected_patterns.size() << "\n";
    
    // 四角形パターンのみフィルタ
    std::vector<Pattern> rectangular_patterns;
    for (const auto& p : connected_patterns) {
        if (is_rectangular(p)) {
            rectangular_patterns.push_back(p);
        }
    }
    
    std::cout << "Rectangular patterns: " << rectangular_patterns.size() << "\n\n";
    
    // メモリ使用量の見積もり
    long long states_per_pattern = 1;
    for (int i = 0; i < n; ++i) {
        states_per_pattern *= 9;
    }
    
    std::cout << "Memory per pattern:\n";
    std::cout << "  States: 9^" << n << " = " << states_per_pattern << "\n";
    double mb = states_per_pattern * sizeof(float) / (1024.0 * 1024.0);
    if (mb < 1024) {
        std::cout << "  Size: " << mb << " MB\n";
    } else {
        std::cout << "  Size: " << (mb / 1024.0) << " GB\n";
    }
    std::cout << "\n";
    
    // ユーザーに表示オプションを聞く
    std::cout << "Display options:\n";
    std::cout << "  1. All unique patterns\n";
    std::cout << "  2. Connected patterns only\n";
    std::cout << "  3. Rectangular patterns only\n";
    std::cout << "  4. Summary only (no display)\n";
    std::cout << "Choose (1-4): ";
    
    int choice = 2;  // デフォルトは連結パターン
    // std::cin >> choice;
    choice = 2;
    
    std::vector<Pattern> patterns_to_show;
    switch (choice) {
        case 1:
            patterns_to_show.assign(unique_patterns.begin(), unique_patterns.end());
            break;
        case 2:
            patterns_to_show = connected_patterns;
            break;
        case 3:
            patterns_to_show = rectangular_patterns;
            break;
        case 4:
            // サマリーのみ
            std::cout << "\n=== Summary ===\n";
            std::cout << "Total patterns: " << unique_patterns.size() << "\n";
            std::cout << "Connected: " << connected_patterns.size() << "\n";
            std::cout << "Rectangular: " << rectangular_patterns.size() << "\n";
            return 0;
    }
    
    std::cout << "\n========================================\n";
    std::cout << "Displaying " << patterns_to_show.size() << " patterns\n";
    std::cout << "========================================\n\n";
    
    for (size_t i = 0; i < patterns_to_show.size(); ++i) {
        print_pattern(patterns_to_show[i], i + 1);
        
        // 20パターンごとに一時停止
        // if ((i + 1) % 20 == 0 && i + 1 < patterns_to_show.size()) {
        //     std::cout << "Press Enter to continue...";
        //     std::cin.ignore();
        //     std::cin.get();
        // }
    }
    
    // おすすめパターンの提案
    std::cout << "\n========================================\n";
    std::cout << "Recommendations\n";
    std::cout << "========================================\n";
    
    if (n == 4) {
        std::cout << "For n=4 (2x2):\n";
        std::cout << "  - Use 16 overlapping 2x2 patterns (high coverage)\n";
        std::cout << "  - Total memory: ~400 KB\n";
        std::cout << "  - Fast training, good generalization\n";
    } else if (n == 6) {
        std::cout << "For n=6 (2x3 or 3x2):\n";
        std::cout << "  - Good balance between memory and expressiveness\n";
        std::cout << "  - Total memory per pattern: ~20 MB\n";
    } else if (n == 9) {
        std::cout << "For n=9 (3x3):\n";
        std::cout << "  - High expressiveness but large memory\n";
        std::cout << "  - Total memory per pattern: ~1.44 GB\n";
        std::cout << "  - Consider using 1-3 strategic positions\n";
    }
    
    return 0;
}
