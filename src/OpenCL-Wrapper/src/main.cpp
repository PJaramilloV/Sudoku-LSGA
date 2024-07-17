#include "opencl.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <numeric>
#include <random>

typedef std::uniform_int_distribution<uint> rand_uint;
typedef std::uniform_real_distribution<float> rand_float;

uint POPULATION_SIZE = 150;
std::random_device generator;
std::default_random_engine rng = std::default_random_engine{generator()};
rand_float randfloat(0, 1);
rand_uint randint(0, POPULATION_SIZE);


uint zeros_from_right(uint bitmask) {
  uint res = 0;
  while (!(bitmask & 0b1) ) { // rightmost bit == 0
    res++;
    bitmask >>= 1;
  }
  return res;
}

//void local_search_cols() {
//  for (auto &member: population) {
//    // record all illegals
//    vector<uint> illegals = member.illegal_cols();
//    for (auto &col: illegals) {
//      bool able[member.length];
//      for (uint i = 0; i < member.length; i++) {
//        able[i] = true;
//      }
//      uint other = get_another(illegals, col); // randomly select another
//      uint a_mask = member.repeat_col_mask(col);
//      uint b_mask = member.repeat_col_mask(other);
//      uint match_mask = a_mask & b_mask;
//      // if repeat numbers are in the same row
//      if (match_mask) {
//        uint row = 0;
//        while (row < member.length) {
//          if (match_mask & (1 << row)) {
//            uint a_num = member.get(row, col);
//            uint b_num = member.get(row, other);
//            if (!(able[a_num] && able[b_num])) {
//              row++;
//              continue;
//            }
//            able[a_num] = false;
//            able[b_num] = false;
//            // if repeat numbers do not exist in both columns
//            if (!member.num_in_col(a_num, other) && !member.num_in_col(b_num, col)) {
//              // swap
//              member.set(row, col, b_num);
//              member.set(row, other, a_num);
//            }
//          }
//          row++;
//        }
//      }
//    }
//  }
//}
//
//void local_search_block() {
//  for (auto &member: population) {
//    // record all illegals
//    vector<uint> illegals = member.illegal_blocks();
//    for (auto &block: illegals) {
//      bool able[member.length];
//
//      // array to check each number is swapped at most once
//      for (uint i = 0; i < member.length; i++) {
//        able[i] = true;
//      }
//
//      uint other = get_another(illegals, block); // randomly select another
//      uint a_spotted = 0, b_spotted = 0;
//      uint a_mask = member.repeat_block_mask(block, a_spotted); // get mask of repeated numbers in block
//      uint b_mask = member.repeat_block_mask(other, b_spotted);
//      uint rows = member.get_block_width();
//
//      for (uint i = 0; i < rows * rows; i++) {
//        // if number is marked as repeated
//        /*   a          b
//         * 1 2 3      6 9 3
//         * 4 5 2      1 6 4
//         * 7 8 9      5 7 8
//         *
//         * a_mask -> 0b000 100 010
//         * b_mask -> 0b000 010 001
//         *
//         * a_spotted -> 0b000 000 010   (2)
//         * b_spotted -> 0b000 100 000   (6)
//         *
//         * i = 1
//         * a_mask & 1<<i (0b10) == true (si numero en cuestion es repetido en a)
//         * a_num = 2
//         *
//         * (b_spotted & 1<<(a_num-1)) (0b1) == false
//         */
//        if (a_mask & (1 << i)) {
//          uint a_num = member.block_get(i / rows, i % rows, block);
//          // but number is in B, unmark
//          if (b_spotted & 1 << (a_num - 1)) {
//            a_mask ^= 1 << i;
//          }
//        }
//      }
//
//      for (uint row = 0; row < rows; row++) {
//        uint rmask_a = a_mask & (0b111 << (row * 3)); // get mask of repeated numbers in row
//        uint rmask_b = b_mask & (0b111 << (row * 3));
//
//        //zeros_from_right(uint) :   0b00100 --> 2  ;  0b00010 --> 1  ;  0b00001 --> 0
//        while (rmask_a && rmask_b) { // there is a repeat in both rows
//          uint a_col = zeros_from_right(rmask_a);  // 0b 001 001 000
//          uint b_col = zeros_from_right(rmask_b);  // 0b 000 110 000
//
//          uint a_num = member.block_get(row, a_col, block);
//          uint b_num = member.block_get(row, b_col, other);
//          // if repeat numbers do not exist in both blocks  -- should be taken care of already
//          //if(in_a_n_not_b & 1<<(a_num-1) && in_b_n_not_a & 1<<(b_num-1)) {
//          //if(!member.num_in_block(a_num, other) && !member.num_in_block(b_num, block)){
//          if (able[a_num] && able[b_num]) {
//            // swap
//            member.block_set(row, a_col, block, b_num);
//            member.block_set(row, b_col, other, a_num);
//
//            able[a_num] = false;
//            able[b_num] = false;
//          }
//
//          rmask_a ^= 1 << a_col;
//          rmask_b ^= 1 << b_col;
//        }
//      }
//    }
//  }
//}

//void elite_learning() {
//  // TODO: Replace the worst individuals with elite members
//
//
//
//  // Replace or reinitialize the worst members with a random member of the elite population
//}

uint seed = randint(generator);
uint MAX_GENERATIONS = 10;
uint ELITE_SIZE = 50;
float PC1 = 0.2, PC2 = 0.1, PM1 = 0.3, PM2 = 0.05;
uint best_sudoker = 300;
uint block_width = 3;
uint sudoku_n = block_width*block_width;
uint sudoku_space = sudoku_n*sudoku_n;
uint best_score = sudoku_n * 2 + 1;
uchar* tmp;
Device device = select_device_with_most_flops();
uint total_cell_count = sudoku_space*POPULATION_SIZE;
uint total_row_count = sudoku_n*POPULATION_SIZE;
uint double_row_count = total_row_count*2;
Memory<uchar> grid(device, sudoku_space*POPULATION_SIZE);
Memory<uchar> new_grid(device, sudoku_space*POPULATION_SIZE);
Memory<uchar> occupancy(device, sudoku_space*POPULATION_SIZE);
Memory<uchar> non_given(device, sudoku_n*POPULATION_SIZE);
Memory<uchar> mistakes(device, sudoku_n*2*POPULATION_SIZE);
Memory<uchar> scores(device, POPULATION_SIZE);
Memory<int> crossoverMap(device, POPULATION_SIZE);
Kernel fitnessColKernel(
    device, total_cell_count, "fitnessColKernel",
    grid,  mistakes, POPULATION_SIZE);
Kernel fitnessBlockKernel(
    device, total_cell_count, "fitnessBlockKernel",
    grid, mistakes, POPULATION_SIZE);
Kernel mistakesAddKernel(
    device, double_row_count, "mistakesAddKernel",
    mistakes, scores, POPULATION_SIZE);
Kernel crossoverKernel(
    device, POPULATION_SIZE, "crossoverKernel",
    grid, new_grid, crossoverMap,
    randint(generator), randint(generator), seed,
    PC2, POPULATION_SIZE);
Kernel mutateRowKernel(
    device, total_row_count, "mutateRowKernel",
    grid, non_given,
    randint(generator), randint(generator), seed,
    PM1, POPULATION_SIZE);
Kernel reinitRowKernel( // TODO: create this
    device, total_row_count, "reinitRowKernel",
    grid, non_given,
    randint(generator), randint(generator), seed,
    PM2, POPULATION_SIZE);
Kernel lsga_col_kernel( // TODO: uncomment & define method to get other col
    device, POPULATION_SIZE, "lsga_col_kernel",
    grid, occupancy, mistakes, POPULATION_SIZE);
Kernel lsga_block_kernel( // TODO: create this & uncomment
    device, POPULATION_SIZE, "lsga_block_kernel",
    grid, occupancy, mistakes, POPULATION_SIZE);
Kernel elite_learning_kernel( // TODO: create this
    device, POPULATION_SIZE, "elite_learning_kernel",
    grid, occupancy,
    /* add other parameters */ POPULATION_SIZE);

void prepare_crossover(){
  std::vector<int> indices(POPULATION_SIZE);
  std::iota(indices.begin(), indices.end(), 0);
  std::shuffle(indices.begin(), indices.end(), generator);
  for(int i=0; i<POPULATION_SIZE; i++){
    crossoverMap[i] = -1;
  }
  int half = POPULATION_SIZE/2;
  for(int i=0; i<half; i++){
    int idx = indices[i];
    int val = indices[idx];
    float random_val = randfloat(generator);
    int make_crossover = (random_val < PC1) && (val ^ idx);
    int dont_crossover = (!make_crossover) & -1;
    crossoverMap[idx] = (make_crossover & val) | dont_crossover;
    crossoverMap[val] = (make_crossover & idx) | dont_crossover;
  }
}

std::vector<uint> sort_scores_idxs(Memory<uchar> &scores_mem, uchar size) {
  std::vector<uint> indices(size);
  for (int i = 0; i < size; ++i) {
    indices[i] = i;
  }
  // Sort indices based on scores
  std::sort(indices.begin(), indices.end(), [&scores_mem](int a, int b) {
      return scores_mem[a] < scores_mem[b];
  });
  return indices;
}

int main(int argc, char *argv[]) {
  string solution = "918745632532619784647283915286534179394178256751926843169457328825361497473892561";
  string editable = "001011111100010101110101011100101001011111110100101001110101011101010001111110100";

  // init population
  for (int m = 0; m < POPULATION_SIZE; m++) {
    uint s=0;
    for (auto &c: editable) {
      uint i = s+m*sudoku_space;
      uchar hint = (c == '0') ? 1 : 0;
      occupancy[i] = hint;
      grid[i] = hint ? (solution.at(i) - '0') : 0;
      new_grid[i] = hint ? (solution.at(i) - '0') : 0;
      s++;
    }
  }
  // eval population
  fitnessColKernel.enqueue_run(); // thIdx = global_idx;  -->  1 thread --> 1 col
  fitnessBlockKernel.run(); //                      -->  1 thread --> 1 block
  mistakesAddKernel.run();
  // fCK -> mistakes[m+0:0+8]
  // fBK -> mistakes[m+9:0+17]
  // mAK -> sum(mistakes[m:m+17]) --> scores[m]

  vector<uint> sorted_idx = sort_scores_idxs(scores, POPULATION_SIZE);

  while (MAX_GENERATIONS) {
    // tournament selection

    // cross over
    prepare_crossover();
    crossoverKernel.run();

    tmp = grid.exchange_host_buffer(tmp);      // old <--> empty
    tmp = new_grid.exchange_host_buffer(tmp);  // new <--> old
    tmp = grid.exchange_host_buffer(tmp);      // empty <--> new

    // mutation
    // 1 thread --> 1 row
    mutateRowKernel().enqueue_run();
    reinitRowKernel().run();

    // column LS
    fitnessColKernel.enqueue_run();
    lsga_col_kernel.run();

    // Sub-block LS
    fitnessColKernel.enqueue_run();
    //local_search_block();

    // eval population
    fitnessColKernel.enqueue_run(); // thIdx = global_idx;  -->  1 thread --> 1 col
    fitnessBlockKernel.run(); //                      -->  1 thread --> 1 block
    mistakesAddKernel.run();

    // Sort members by fitness
    sorted_idx = sort_scores_idxs(scores, POPULATION_SIZE);

    // TODO: elite population learning
    //elite_learning();

    // save best
    best_sudoker = sorted_idx.front(); // If population is sorted

    // finish early if sudoku is solved
    if (scores[best_sudoker] == 0) break;

    MAX_GENERATIONS--;

  }
  std::cout << "The best solution found is\n ";// << best_sudoker;
  best_score = scores[best_sudoker];
  if (best_score) {
    std::cout << "It made " << best_score << " mistakes\n";
  } else {
    std::cout << "It is a correct solution!\n";
  }
  return 0;
}
