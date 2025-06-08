#ifndef LOCAL_GAME_OF_LIFE_H
#define LOCAL_GAME_OF_LIFE_H

#include <mpi.h>
#include <Eigen/Dense>

class LocalGameOfLife {
public:
    LocalGameOfLife(int local_rows, int local_cols, MPI_Comm cart_comm, int halo_size);

    void randomize(double alive_prob = 0.3);

    void step();

    bool is_alive(int r, int c) const;

    int local_rows() const;

    int local_cols() const;

    Eigen::MatrixXi get_current_grid();

    MPI_Comm get_cart_comm();

    void set_local_block(const Eigen::MatrixXi &block);

    void output_to_file(const std::string &identifier) const;

private:
    int local_rows_;
    int local_cols_;
    int halo_size_;
    MPI_Comm cart_comm_;
    int cart_rank_;
    int cart_size_;
    int dims_[2];
    int coords_[2];

    static void print_matrix(const Eigen::MatrixXi &matrix);

    int nbr_n_, nbr_s_, nbr_w_, nbr_e_;

    Eigen::MatrixXi current_grid_;
    Eigen::MatrixXi next_grid_;

    void exchange_halos();

    int count_alive_neighbors(int r, int c) const;

    int local_iteration_ = 0;

};

#endif  // LOCAL_GAME_OF_LIFE_H