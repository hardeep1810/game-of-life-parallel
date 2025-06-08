#include "GameOfLifeUI.h"
#include <SFML/System/Vector2.hpp>

const sf::Color colors[20] = {
        sf::Color(31, 119, 180),
        sf::Color(255, 127, 14),
        sf::Color(44, 160, 44),
        sf::Color(214, 39, 40),
        sf::Color(148, 103, 189),
        sf::Color(140, 86, 75),
        sf::Color(227, 119, 194),
        sf::Color(127, 127, 127),
        sf::Color(188, 189, 34),
        sf::Color(23, 190, 207),
        sf::Color(174, 199, 232),
        sf::Color(255, 187, 120),
        sf::Color(152, 223, 138),
        sf::Color(255, 152, 150),
        sf::Color(197, 176, 213),
        sf::Color(196, 156, 148),
        sf::Color(247, 182, 210),
        sf::Color(199, 199, 199),
        sf::Color(219, 219, 141),
        sf::Color(158, 218, 229),
};

GameOfLifeUI::GameOfLifeUI(int rows, int cols, uint cell_size, bool visualize_grid,
                           std::string window_title)
        : rows_(rows), cols_(cols), cell_size_(cell_size), visualize_grid_(visualize_grid),
          window_(sf::VideoMode(sf::Vector2u(cols * cell_size, rows * cell_size)),
                  window_title),
          cell_shape_(sf::Vector2f(cell_size - 1.f, cell_size - 1.f)) {
    window_.setFramerateLimit(fps_limit_);
}

bool GameOfLifeUI::poll_events() {
    while (const std::optional event = window_.pollEvent()) {
        if (event->is<sf::Event::Closed>())
            return false;

        if (auto keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                return false;

            else if (keyPressed->scancode == sf::Keyboard::Scancode::Up) {
                fps_limit_ = std::min(120, fps_limit_ + 5);
                window_.setFramerateLimit(fps_limit_);
            } else if (keyPressed->scancode == sf::Keyboard::Scancode::Down) {
                fps_limit_ = std::max(1, fps_limit_ - 5);
                window_.setFramerateLimit(fps_limit_);
            }
        }
    }
    return true;
}

void GameOfLifeUI::clear() { window_.clear(sf::Color::Black); }

void GameOfLifeUI::draw_grid(const GridAccessorFunc &is_alive, const GridRankFunc &get_proc) {
    for (int r = 0; r < rows_; ++r) {
        for (int c = 0; c < cols_; ++c) {
            if (is_alive(r, c)) {
                cell_shape_.setPosition(
                        sf::Vector2f(float(c * cell_size_), float(r * cell_size_)));

                if (visualize_grid_) {
                    int proc = get_proc(r, c);

                    cell_shape_.setFillColor(colors[proc % 11]);
                } else {
                    cell_shape_.setFillColor(sf::Color::White);
                }
                window_.draw(cell_shape_);
            }
        }
    }
}

void GameOfLifeUI::display() { window_.display(); }
