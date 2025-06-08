#ifndef GAME_OF_LIFE_UI_H
#define GAME_OF_LIFE_UI_H

#include <SFML/Graphics.hpp>
#include <functional>
#include <string>

class GameOfLifeUI {
public:
    using GridAccessorFunc = std::function<bool(int row, int col)>;
    using GridRankFunc = std::function<int(int row, int col)>;

    GameOfLifeUI(int rows, int cols, uint cell_size, bool visualize_grid,
                 std::string window_title = "Conway's Game of Life");

    bool poll_events();

    void clear();

    void draw_grid(const GridAccessorFunc &is_alive, const GridRankFunc &get_grid);

    void display();

private:
    int rows_, cols_;
    bool visualize_grid_ = false;
    uint cell_size_;
    int fps_limit_ = 5;
    sf::RenderWindow window_;
    sf::RectangleShape cell_shape_;
};

#endif // GAME_OF_LIFE_UI_H

