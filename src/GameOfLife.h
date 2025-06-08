#ifndef GAME_OF_LIFE_H
#define GAME_OF_LIFE_H

#include <mpi.h>
#include "LocalGameOfLife.h"
#include <memory>
#include <Eigen/Dense>

class GameOfLife {
public:
    GameOfLife(int global_rows, int global_cols);

    ~GameOfLife();

    void randomize(double alive_prob = 0.3);

    void step();

    void output_to_file(const std::string &filename);

    int nprocs() const;

    int procno() const;

    bool load_from_file(const std::string &filename);

    void gather_full_grid();

    bool is_alive(int row, int col) const;

    int get_proc(int row, int col) const;

private:
    int global_rows_;
    int global_cols_;
    int halo_size_ = 0;

    int mpi_rank_{};
    int mpi_size_{};

    MPI_Comm cart_comm_{};
    int dims_[2]{};
    Eigen::MatrixXi global_grid_;
    Eigen::MatrixXi process_grid_;      // used for proc colors in UI, otherwise useless memory sink.
    std::unique_ptr<LocalGameOfLife> local_game_;


};

#endif  // GAME_OF_LIFE_H