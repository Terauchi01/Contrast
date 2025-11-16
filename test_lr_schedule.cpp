#include <iostream>
#include <iomanip>
#include <cmath>

float calculate_learning_rate(int current_game, int total_games, float lr_max = 0.1f, float lr_min = 0.005f) {
    float progress = static_cast<float>(current_game - 1) / static_cast<float>(total_games - 1);
    float k = 19.0f;
    float lr = lr_min + (lr_max - lr_min) / (1.0f + k * progress * progress);
    return lr;
}

int main() {
    int total_games = 10000;
    
    std::cout << "Learning Rate Schedule (Inverse-Square Decay)\n";
    std::cout << "==============================================\n";
    std::cout << "Game      | Progress | Learning Rate\n";
    std::cout << "----------|----------|---------------\n";
    
    int checkpoints[] = {1, 100, 500, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
    
    for (int game : checkpoints) {
        float progress = static_cast<float>(game - 1) / static_cast<float>(total_games - 1);
        float lr = calculate_learning_rate(game, total_games);
        
        std::cout << std::setw(9) << game << " | "
                  << std::setw(7) << std::fixed << std::setprecision(1) << (progress * 100) << "% | "
                  << std::setw(13) << std::setprecision(6) << lr << "\n";
    }
    
    std::cout << "\nComparison with Linear Decay:\n";
    std::cout << "Game      | Inverse² | Linear\n";
    std::cout << "----------|----------|--------\n";
    
    for (int game : checkpoints) {
        float progress = static_cast<float>(game - 1) / static_cast<float>(total_games - 1);
        float lr_inverse = calculate_learning_rate(game, total_games);
        float lr_linear = 0.1f - (0.1f - 0.005f) * progress;
        
        std::cout << std::setw(9) << game << " | "
                  << std::setw(8) << std::fixed << std::setprecision(6) << lr_inverse << " | "
                  << std::setw(6) << std::setprecision(6) << lr_linear << "\n";
    }
    
    return 0;
}
