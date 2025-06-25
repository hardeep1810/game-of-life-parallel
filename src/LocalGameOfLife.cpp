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

    int periods[2] = {1,1}; //{0,0};
    MPI_Cart_get(cart_comm_, 2 ,dims_, periods , coords_);
    
    // nbr_n_ = 0;
    // nbr_s_ = 0;
    // nbr_w_ = 0;
    // nbr_e_ = 0;
    MPI_Cart_shift(cart_comm_,0,1, &nbr_n_, &nbr_s_); //rows (north/south)
    MPI_Cart_shift(cart_comm_,1,1,&nbr_w_, &nbr_e_); //Cols (west/east)

    current_grid_ = Eigen::MatrixXi::Zero(local_rows_ + 2 * halo_size_, local_cols_ + 2 * halo_size_);
    next_grid_ = Eigen::MatrixXi::Zero(local_rows_ + 2 * halo_size_, local_cols_ + 2 * halo_size_);
}

void LocalGameOfLife::randomize(double alive_prob) {
    std::random_device rd;
    std::mt19937 gen(rd() + cart_rank_);
    std::bernoulli_distribution dist(alive_prob);
    for (int r = halo_size_; r < local_rows_ + halo_size_; ++r) {
        for (int c = halo_size_; c < local_cols_ + halo_size_; ++c) {
            current_grid_(r, c) = dist(gen) ? 1 : 0;
        }
    }
}

int LocalGameOfLife::count_alive_neighbors(int r, int c) const {

    int count = 0;
    for(int dr = -1; dr <=1; ++dr)
    {
        for(int dc = -1; dc<=1; ++dc)
        {
            if(dr ==0 && dc==00)
                continue;
            if(current_grid_(r + dr, c + dc) == 1) //Assuming 1 for alive
            {
                ++count;
            }
        }
    }
    return count;
}

void LocalGameOfLife::exchange_halos() {
    MPI_Status status;



    // Uncomment the following line to see neighbor ranks for debugging

    DEBUG_LOG("Rank " << cart_rank_ << " neighbors: N=" << nbr_n_

                      << ", S=" << nbr_s_ << ", W=" << nbr_w_ << ", E=" << nbr_e_ << std::endl);



    // Number of Request: 4 sends + 4 receives = 8 requests

    MPI_Request requests[8];

    int request_idx = 0;



    // 1. Allocate temporary buffers for sending and receiving

    // Each Buffer needs to hold 'Halo_size_' rows/columns.

    // For Rows: width is local_cols_

    // For Cols: height is local_rows_



    // For sending rows (North/South)

    Eigen::MatrixXi send_north_buf(halo_size_, local_cols_); // Top 2 rows of my core data

    Eigen::MatrixXi send_south_buf(halo_size_, local_cols_); // Bottom 2 rows of my core data



    // For sending columns (West/East)

    Eigen::MatrixXi send_west_buf(local_rows_, halo_size_); // Left 2 cols of my core data

    Eigen::MatrixXi send_east_buf(local_rows_, halo_size_); // Right 2 cols of my core data



    // For Receiving rows (North/South halo)

    Eigen::MatrixXi recv_north_halo_buf(halo_size_, local_cols_); // Into my top 2 halo rows

    Eigen::MatrixXi recv_south_halo_buf(halo_size_, local_cols_); // Into my bottom 2 halo rows



    // For Receiving columns (West/East halo)

    Eigen::MatrixXi recv_west_halo_buf(local_rows_, halo_size_); // Into my left 2 halo cols

    Eigen::MatrixXi recv_east_halo_buf(local_rows_, halo_size_); // Into my right 2 halo cols



    // 2. Copy data from current_grid_ into send buffers (from core regions)

    // Rows to send North (your core rows at indices halo_size_ and halo_size_+1)

    send_north_buf = current_grid_.block(halo_size_, halo_size_, halo_size_,local_cols_);

    // Rows to send South (Core rows at indices local_rows_ + halo_size_ - halo_size_ and ...)

    send_south_buf = current_grid_.block(local_rows_,halo_size_,halo_size_,local_cols_);



    // Columns to send West (Core columns at indices halo_size_ and halo_size_+1)

    send_west_buf = current_grid_.block(halo_size_,halo_size_,local_rows_,halo_size_);

    // Columns to send East (Core columns at indices local_cols_ + halo_size_ - halo_size_ and ...)

    send_east_buf = current_grid_.block(halo_size_,local_cols_,local_rows_,halo_size_); // CORRECTED: was send_west_buf



    // 3. Post all receives first (to prevent deadlock)

    // Receive North Halo (from South neighbor, into your top halo)

    MPI_Irecv(recv_north_halo_buf.data(),halo_size_ * local_cols_, MPI_INT, nbr_s_,

              0,cart_comm_, &requests[request_idx++]);

    // Receive South Halo (from North Neighbor, into your bottom halo)

    MPI_Irecv(recv_south_halo_buf.data(),halo_size_ * local_cols_, MPI_INT, nbr_n_,

              1,cart_comm_, &requests[request_idx++]);

    // Receive West halo (from East neighbor, into your left halo)

    MPI_Irecv(recv_west_halo_buf.data(),local_rows_ * halo_size_, MPI_INT, nbr_e_, // CORRECTED: was recv_north_halo_buf

              2,cart_comm_, &requests[request_idx++]);

    // Receive East halo (from West neighbor, into your right halo)

    MPI_Irecv(recv_east_halo_buf.data(),local_rows_ * halo_size_, MPI_INT, nbr_w_, // CORRECTED: was recv_north_halo_buf

              3,cart_comm_, &requests[request_idx++]);



    // 4. Post all sends

    // send to North (top core rows, to North neighbor)

    MPI_Isend(send_north_buf.data(),halo_size_ * local_cols_, MPI_INT, nbr_n_,

                0,cart_comm_,&requests[request_idx++]);

    // send to South (bottom core rows, to South neighbor)

    MPI_Isend(send_south_buf.data(),halo_size_ * local_cols_, MPI_INT, nbr_s_, // CORRECTED: was send_north_buf

                1,cart_comm_,&requests[request_idx++]);

    // send to West (left core columns, to West neighbor)

    MPI_Isend(send_west_buf.data(),local_rows_* halo_size_, MPI_INT, nbr_w_, // CORRECTED: was send_north_buf

                2,cart_comm_,&requests[request_idx++]);

    // send to East (right core columns, to East neighbor)

    MPI_Isend(send_east_buf.data(),local_rows_ * halo_size_, MPI_INT, nbr_e_, // CORRECTED: was send_north_buf

                3,cart_comm_,&requests[request_idx++]);



    // 5. Wait for all communications to complete

    MPI_Waitall(request_idx, requests, MPI_STATUSES_IGNORE);



    // 6. Copy Received data from temporary buffers into current_grid_ halo regions

    // Copy North Halo (received into top 2 halo rows of current_grid_)

    current_grid_.block(0,halo_size_,halo_size_,local_cols_) = recv_north_halo_buf;

    // Copy South Halo (received into bottom 2 halo rows of current_grid_)

    current_grid_.block(local_rows_ + halo_size_,halo_size_,halo_size_,local_cols_) = recv_south_halo_buf;



    // Copy West Halo (received into left 2 halo cols of current_grid_)

    current_grid_.block(halo_size_,0,local_rows_,halo_size_) = recv_west_halo_buf;

    // Copy East Halo (received into right 2 halo cols of current_grid_)

    current_grid_.block(halo_size_,local_cols_ + halo_size_, local_rows_,halo_size_) = recv_east_halo_buf;

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
    std::cout<< "RANK" << cart_rank_ << ": local_rows=" << local_rows_
             << " local_cols=" << local_cols_
             << " current_grid size: " << current_grid_.rows()
             << "x" << current_grid_.cols() << std::endl;
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
