#include "GameOfLife.h"
#include "GameOfLifeUI.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char **argv) {
    bool show_ui = true;
    std::string start_state_file;
    int total_rows = 128;
    int total_cols = 128;
    int iterations = -1;
    bool show_grid = false;
    constexpr uint cell_size = 6;   // for UI

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-n" || arg == "--no-ui") {
            show_ui = false;
        } else if ((arg == "-r" || arg == "--total_rows") && i + 1 < argc) {
            total_rows = std::stoi(argv[++i]);
            if ((total_rows & (total_rows - 1)) != 0) {
                throw std::invalid_argument("Error: Rows must be a power of 2.");
            }
        } else if ((arg == "-c" || arg == "--total_cols") && i + 1 < argc) {
            total_cols = std::stoi(argv[++i]);
            if ((total_cols & (total_cols - 1)) != 0) {
                throw std::invalid_argument("Error: Cols must be a power of 2.");
            }
        } else if ((arg == "-i" || arg == "--iterations") && i + 1 < argc) {
            iterations = std::stoi(argv[++i]);
        } else if ((arg == "-f" || arg == "--file") && i + 1 < argc) {
            start_state_file = argv[++i];
        } else if (arg == "--show-grid") {
            show_grid = true;
        }
    }

    if (total_cols != total_rows) {
        throw std::invalid_argument("Currently only square grids are supported!");
    }

    GameOfLife game(total_rows, total_cols);

    if (!start_state_file.empty()) {
        if (!game.load_from_file(start_state_file)) {
            std::cerr << "Failed to load start state from file: " << start_state_file
                      << std::endl;
            return 1;
        }
    } else {
        game.randomize(0.25);
    }
//
    std::unique_ptr<GameOfLifeUI> ui;
    if (show_ui && game.procno() == 0) {
        ui = std::make_unique<GameOfLifeUI>(total_rows, total_cols, cell_size, show_grid);
    }

    bool running = true;
    long done_iterations = 0;
    while (running) {
        if (show_ui && game.procno() == 0) {
            running = ui->poll_events();
            ui->clear();
        }

        game.step();
        ++done_iterations;
        if (iterations >= 0 && done_iterations >= iterations) {
            game.output_to_file("output.txt");
            if (show_ui && game.procno() == 0) {
                std::cout << "Reached " << iterations
                          << " iterations: Saving final state and stopping simulation. Window will close in 60s."
                          << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(60));
            }

            return 0;
        }

        if (show_ui) {
            game.gather_full_grid();
            if (game.procno() == 0) {
                ui->draw_grid(
                        [&game](int r, int c) { return game.is_alive(r, c); },
                        [&game](int r, int c) { return game.get_proc(r, c); }
                );
                ui->display();
            }
        }
    }
    game.output_to_file("output.txt");


    return 0;
}
