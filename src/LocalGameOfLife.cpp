//
// Created by work on 5/26/25.
//
//

#include "LocalGameOfLife.h"

#include <random>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include "Logger.h"

LocalGameOfLife::LocalGameOfLife(int local_rows, int local_cols, MPI_Comm cart_comm, int halo_size)
        : local_rows_(local_rows), local_cols_(local_cols), 
        cart_comm_(cart_comm), halo_size_(halo_size) {
    /*
     * General initialization of the process local GameOfLife. Contains the calculated neighbours / local sizes.
     */
    MPI_Comm_rank(cart_comm_, &cart_rank_);
    MPI_Comm_size(cart_comm_, &cart_size_);

    // Retrieve the topology information from the communicator. This was created in the GameOfLife constructor
    // and serves as main communicator for the parallelization.
    // See https://www.mpich.org/static/docs/v3.1/www3/MPI_Cart_get.html
    // MPI_Cart_get(cart_comm_, 2, dims_, (int[2]) {0, 0}, coords_);
        int periods[2] = {0,0};
        MPI_Cart_get(cart_comm_, 2 ,dims_, periods , coords_);
    
    // MPI_Cart_Shift is a nice helper functions to get the neighbours for the topology.
    // We use the periods of cartesian topology
    // to implement the toroidal/periodic boundary conditions of the Game Of Life.
    // See https://www.mpich.org/static/docs/v4.1/www3/MPI_Cart_shift.html

    // nbr_x_ are all neighbor ranks (==subgrids) in all 4 directions in 2D, expressed via North, South, East, West.
    //
    // TODO: Implement correct neighbor ranks for each process and do not hardcode it like in the example! Use
    // TODO  MPI_Cart_shift (more convenient) or calculate them manually using dimensions and ranks.
    // Currently all are set to rank 0, this means 0 is its own neighbor in all directions! When you onlu use
    // one MPI process (mpirun -n 1) and after implementing exchange_halo() this would even work for the periodic
    // boundary conditions! Try to understand why!
    // nbr_n_ = 0;
    // nbr_s_ = 0;
    // nbr_w_ = 0;
    // nbr_e_ = 0;
    MPI_Cart_shift(cart_comm_,0,1, &nbr_n_, &nbr_s_); //rows (north/south)
    MPI_Cart_shift(cart_comm_,1,1,&nbr_w_, &nbr_e_); //Cols (west/east)

    // Setup of current and next grid for Game Of Life. Check any web tutorial or the wikipedia page for why two
    // grids are used.
    // current_grid_ = Eigen::MatrixXi::Zero(local_rows_, local_cols_);
    // next_grid_ = Eigen::MatrixXi::Zero(local_rows_, local_cols_);
    current_grid_ = Eigen::MatrixXi::Zero(local_rows_ + 2 * halo_size_, local_cols_ + 2 * halo_size_);
    next_grid_ = Eigen::MatrixXi::Zero(local_rows_ + 2 * halo_size_, local_cols_ + 2 * halo_size_);
}

void LocalGameOfLife::randomize(double alive_prob) {
    std::random_device rd;
    // Initialization of random number generator using the rank (cart_rank_) of the process.
    // This makes sure all processes are initialized with different random numbers, otherwise
    // the grid might look quite symmetric. Test it out if you want!
    std::mt19937 gen(rd() + cart_rank_);
    std::bernoulli_distribution dist(alive_prob);
    for (int r = halo_size_; r < local_rows_ + halo_size_; ++r) {
        for (int c = halo_size_; c < local_cols_ + halo_size_; ++c) {
            current_grid_(r, c) = dist(gen) ? 1 : 0;
        }
    }
}

int LocalGameOfLife::count_alive_neighbors(int r, int c) const {
/*
 * Method to count current alive neighbours based on the current grid and
 * returning the count. Be careful, this method implements the periodic
 * boundary condition for the sequential case (how?), but this will not be
 * working anymore with the parallel version and a halo region!
 * TODO: Adapt this method to work with the Halo region!
 */
    int count = 0;
    // for (int dr = -1; dr <= 1; ++dr) {
    //     int row = (r + dr + local_rows_) % local_rows_;
    //     for (int dc = -1; dc <= 1; ++dc) {
    //         int col = (c + dc + local_cols_) % local_cols_;
    //         if (dr == 0 && dc == 0)
    //             continue;
    //         if (current_grid_(row, col) != 0)
    //             ++count;
    //     }
    // }
    // return count;
    for (int dr = -1; dr <= 1; ++dr)
    {
        for (int dc = -1; dc <= 1; ++dc)
        {
            if (dr == 0 && dc == 0)
                continue;

            int row = r + dr;
            int col = c + dc;

            //Basic Bounds check for now (assumes halo is filled)
            if(row >= 0 && row < current_grid_.rows() &&
                col >= 0 && col < current_grid_.cols() &&
                current_grid_(row, col) != 0)
                {
                    ++count;
                }
        }
    }
    return count;
    
}

void LocalGameOfLife::exchange_halos() {
    MPI_Status status;

    DEBUG_LOG("Rank " << cart_rank_ << " neighbors: N=" << nbr_n_
                      << ", S=" << nbr_s_ << ", W=" << nbr_w_ << ", E=" << nbr_e_ << std::endl);

    // TODO This is the main challenge of the project! You must implement the halo exchange logic using MPI
    // Here are some hints:
    // * Memory layout is critical to this task! Both rows and columns may not have a contiguous layout, even
    //   as Eigen::Matrix (think about why! It is related to the Halo region!)
    // * Eigen::Matrix has column major order by default
    // * You may need to use temporary buffers (std::vector or eigen::Matrix) with the correct size and memory layout!
    // * To simplify your life, you may think about exchanging single rows / columns for the Halo instead of two
    //   at once. When you start implementing a Halo size 1, you'll still get an almost correct simulation. Only
    //   for the corners of each local grid a halo size 1 will break down. Why?
}

void LocalGameOfLife::print_matrix(const Eigen::MatrixXi &matrix) {
    // Little helper function, sometimes useful for printing small matrices.
    std::cout << matrix << std::endl << std::endl;
}

void LocalGameOfLife::step() {
    /*
     * This method implements an actual step of the simulation. It calls exchange_halos() first
     * because it must update the boundary region between the processes!
     */
    exchange_halos();
    for (int r = halo_size_; r < local_rows_ + halo_size_; ++r) {
        for (int c = halo_size_; c < local_cols_ + halo_size_; ++c) {
            int neighbors = count_alive_neighbors(r, c);
            if (current_grid_(r, c) != 0) {
                next_grid_(r, c) = (neighbors == 2 || neighbors == 3) ? 1 : 0;
            } else {
                next_grid_(r, c) = (neighbors == 3) ? 1 : 0;
            }
        }
    }
    current_grid_.swap(next_grid_);
    ++local_iteration_;
}

void LocalGameOfLife::set_local_block(const Eigen::MatrixXi &block) {
    /*
     * You may ignore this method. It is required to distribute the input file (-f option) to all ranks.
     */
    if (block.rows() != local_rows_ || block.cols() != local_cols_)
        throw std::runtime_error("set_local_block: size mismatch");

    current_grid_.setZero();
    current_grid_.block(halo_size_, halo_size_, local_rows_, local_cols_) = block;
}

int LocalGameOfLife::local_rows() const { return local_rows_; }

int LocalGameOfLife::local_cols() const { return local_cols_; }

Eigen::MatrixXi LocalGameOfLife::get_current_grid() { return current_grid_; }

MPI_Comm LocalGameOfLife::get_cart_comm() { return cart_comm_; }


void LocalGameOfLife::output_to_file(const std::string &identifier) const {
    /*
     * This is a debugging function that might be useful to investigate how the local grid is looking at different
     * steps and processes. It may help when debugging issues with the exchange_halo() method!
     */
    auto filename = std::to_string(local_iteration_) + "_grid_proc_" + identifier + std::to_string(cart_rank_) + ".txt";
    std::ofstream outfile(filename);
    if (!outfile) {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    // Debug output will print full matrix including the halo on all edges (2*halo_size)
    for (int r = 0; r < local_rows_ + 2 * halo_size_; ++r) {
        for (int c = 0; c < local_cols_ + 2 * halo_size_; ++c) {
            outfile << (current_grid_(r, c) == 1 ? '1' : '0');
        }
        outfile << '\n';
    }
}
