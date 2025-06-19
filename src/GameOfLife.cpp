#include "GameOfLife.h"
#include "LocalGameOfLife.h"

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <memory>

GameOfLife::GameOfLife(int global_rows, int global_cols)
        : global_rows_(global_rows), global_cols_(global_cols) {

    // Since we initialize MPI not in the main, but in this class we use a different INIT call named
    // MPI_Init_thread. See https://stackoverflow.com/questions/34851903/mpi-init-vs-mpi-init-thread
    int provided = 0;
    int err = MPI_Init_thread(nullptr, nullptr, MPI_THREAD_SERIALIZED, &provided);
    if (err != MPI_SUCCESS) {
        throw std::runtime_error("MPI_Init_thread failed");
    }

    // Normal MPI variables as defined in the lectures
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size_);

    // The grid of the Game Of Life is a 2D grid. Our domain decomposition scheme is using a 2D block decomposition
    // to minimize the necessary MPI communication (as compared to row-wise or column-wise 1D schemes, see the
    // Eijkhout-paper!
    // In addition, we are required to implement periodic (sometimes also called 'toroidal') boundary conditions
    // so that a cell on the very right of the simulation grid 'sees' the first cell on the very left.
    // (see https://en.wikipedia.org/wiki/Periodic_boundary_conditions for the basic idea)
    //
    // Since this kind of topology is very common MPI supports this by using the MPI_Dims_create and MPI_cart_create
    // functions. Please read their documentation to get an idea what they are doing. You must adapt the call to
    // one of them to enable periodic boundary conditions!
    dims_[0] = 0;
    dims_[1] = 0;
    MPI_Dims_create(mpi_size_, 2, dims_);

    if (mpi_rank_ == 0) {
        std::cout << "Using MPI grid: " << dims_[0] << " x " << dims_[1] << std::endl;
    }

    int periods[2] = {0, 0};
    int reorder = 1;

    MPI_Cart_create(MPI_COMM_WORLD, 2, dims_, periods, reorder, &cart_comm_);

    // Compute local block sizes as sanity check - if the grid size does not match we need to return an error, since
    // we did not implement handling of remainders!
    if (global_rows_ % dims_[0] != 0 || global_cols_ % dims_[1] != 0) {
        if (mpi_rank_ == 0)
            std::cerr << "Global grid dimensions must be divisible by dims\n";
        MPI_Abort(cart_comm_, 1);
    }

    int local_rows = global_rows_ / dims_[0];
    int local_cols = global_cols_ / dims_[1];

    // Instantiate local game with local information. This is the class where all the actual work is done.
    // Every MPI process will have its own local_game and runs through the logic. For parallelization, this
    // class must be changed most!
    local_game_ = std::make_unique<LocalGameOfLife>(local_rows, local_cols, cart_comm_, halo_size_);

    // The following matrix holds the rank per gridpoint. In principle this information could be calculated on the fly,
    // when it is needed, but for our teaching project storage is not such an issue and we accept having another
    // full-sized matrix.
    process_grid_.resize(global_rows, global_cols);
    int rank = 0;
    for (int pr = 0; pr < dims_[0]; ++pr) {
        for (int pc = 0; pc < dims_[1]; ++pc) {
            int row_start = pr * local_rows;
            int col_start = pc * local_cols;
            process_grid_.block(row_start, col_start, local_rows, local_cols).setConstant(rank);
            ++rank;
        }
    }

}

GameOfLife::~GameOfLife() {
    MPI_Finalize();
}

void GameOfLife::randomize(double alive_prob) {
    local_game_->randomize(alive_prob);
}

void GameOfLife::step() {
    local_game_->step();

}

void GameOfLife::output_to_file(const std::string &filename) {
    /*
     * Write the final output to a file.
     * TODO works for one process, but requires a small change for parallel implementation.
     */
    gather_full_grid();
    std::ofstream outfile(filename);
    if (!outfile) {
        throw std::runtime_error("Failed to open output file: " + filename);
    }

    for (int r = 0; r < global_rows_; ++r) {
        for (int c = 0; c < global_cols_; ++c) {
            outfile << (global_grid_(r, c) ? '1' : '0');
        }
        outfile << '\n';
    }
}

bool GameOfLife::is_alive(int row, int col) const {
    return global_grid_(row, col) == 1;
}

int GameOfLife::get_proc(int row, int col) const {
    return process_grid_(row, col);
}


void GameOfLife::gather_full_grid() {
    /*
     * Fetches full grid from all processes for output or visualization.
     * You may ignore this method, it is already implemented and will work if you
     * stick to the original variable names and other parts of the code.
     * You could read and understand it to gain some understanding of how the grid is collected,
     * this might be handy for the exchange_halo()-method.
     */
    const int local_rows = local_game_->local_rows();
    const int local_cols = local_game_->local_cols();

    MPI_Comm comm = local_game_->get_cart_comm();

    // Each process prepares its local data block (excluding halos)
    const Eigen::MatrixXi &local_grid = local_game_->get_current_grid();
    Eigen::MatrixXi local_block = local_grid.block(halo_size_, halo_size_, local_rows, local_cols);

    if (mpi_rank_ == 0) {
        // Allocate the full global grid
        global_grid_.resize(global_rows_, global_cols_);

        // Copy rank 0's own local block immediately
        int px = 0;
        int py = 0;
        global_grid_.block(px * local_rows, py * local_cols, local_rows, local_cols) = local_block;

        MPI_Status status;
        for (int proc = 1; proc < mpi_size_; ++proc) {
            // Compute process grid coordinates of proc
            int proc_x = proc / dims_[1];
            int proc_y = proc % dims_[1];

            // Buffer to receive proc's local grid block
            Eigen::MatrixXi recv_block(local_rows, local_cols);

            MPI_Recv(recv_block.data(), local_rows * local_cols, MPI_INT, proc, 0, comm, &status);

            // Copy into correct location in global grid
            global_grid_.block(proc_x * local_rows, proc_y * local_cols, local_rows, local_cols) = recv_block;
        }
    } else {
        // Send local block to rank 0
        MPI_Send(local_block.data(), local_rows * local_cols, MPI_INT, 0, 0, comm);
    }
}

int GameOfLife::procno() const { return mpi_rank_; }

int GameOfLife::nprocs() const { return mpi_size_; }

bool GameOfLife::load_from_file(const std::string &filename) {
    /*
   * Loads an input file with some pre-defined game state and distributes it to all local games.
   * You may ignore this method, it is already implemented and will work if you
   * stick to the original variable names and other parts of the code.
   * You could read and understand it to gain some understanding of how the grid is distributed,
   * this might be handy for the exchange_halo()-method.
   */
    MPI_Comm comm = local_game_->get_cart_comm();

    int local_rows = local_game_->local_rows();
    int local_cols = local_game_->local_cols();

    if (mpi_rank_ == 0) {
        // Rank 0 reads the file into global grid
        Eigen::MatrixXi global_grid(global_rows_, global_cols_);
        std::ifstream infile(filename);
        if (!infile) {
            std::cerr << "Cannot open file: " << filename << std::endl;
            return false;
        }

        std::string line;
        int r = 0;
        while (std::getline(infile, line) && r < global_rows_) {
            for (int c = 0; c < std::min<int>(line.size(), global_cols_); ++c) {
                global_grid(r, c) = (line[c] == '1') ? 1 : 0;
            }
            ++r;
        }
        infile.close();

        // Send sub-blocks to other ranks
        for (int proc = 1; proc < mpi_size_; ++proc) {
            int proc_x = proc / dims_[1];
            int proc_y = proc % dims_[1];

            Eigen::MatrixXi block = global_grid.block(proc_x * local_rows, proc_y * local_cols, local_rows, local_cols);

            MPI_Send(block.data(), local_rows * local_cols, MPI_INT, proc, 0, comm);
        }

        // Copy own sub-block into local grid
        local_game_->set_local_block(global_grid.block(0, 0, local_rows, local_cols));

    } else {
        // Other ranks receive their sub-block
        Eigen::MatrixXi local_block(local_rows, local_cols);
        MPI_Recv(local_block.data(), local_rows * local_cols, MPI_INT, 0, 0, comm, MPI_STATUS_IGNORE);

        local_game_->set_local_block(local_block);
    }

    return true;
}