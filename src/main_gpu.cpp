#include <chrono>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include "member.h"


struct Times {
    long create_data;
    long execution;

    long total() { return create_data + execution; }
};


Times t;
uint POPULATION_SIZE = 150;
std::random_device generator;
std::default_random_engine rng = std::default_random_engine{generator()};
rand_float randfloat(0, 1);
rand_uint randint(0, POPULATION_SIZE);

uint MAX_GENERATIONS = 10;
uint ELITE_SIZE = 50;
uint generations = 10; // 10000
uint sudoku_n = 9;
uint best_score = sudoku_n * 2 + 1;
float PC1 = 0.2, PC2 = 0.1, PM1 = 0.3, PM2 = 0.05;
vector<Member> population, new_population;
vector<Member> elite;
vector<uint> scores;


uint zeros_from_right(uint bitmask, uint skip = 0) {
  uint res = 0;
  while (!(bitmask & 0b1) || skip) { // rightmost bit == 0
    res++;
    bitmask >>= 1;
  }
  return res;
}

void crossover() {
  for (auto &member: population) {
    if (member.is_parent) {
      continue;
    }
    if (randfloat(generator) < PC1) {
      Member child1 = member;
      Member child2;
      while (true) {
        // select second parent from population
        Member &fuck_buddy = get_another(population, member);
        if (fuck_buddy.is_parent) {
          continue;
        }
        child2 = fuck_buddy;
        for (uint r = 0; r < sudoku_n; r++) {
          if (randfloat(generator) < PC2) {
            // parents exchange rows
            child1.exchange(r, fuck_buddy);
            child2.exchange(r, member);
          }
        }
        member.is_parent = true;
        fuck_buddy.is_parent = true;
        break;
      }
      // save offspring
      new_population.push_back(child1);
      new_population.push_back(child2);
    } else {
      new_population.push_back(member); // give virgin another chance in the future <- No, because then a virgin could
      // both pass to the next generation AND be a parent
      // To prevent that, virgins also have to be parents (of themselves)
      member.is_parent = true;
    }
  }
  bool diff_size = population.size() != new_population.size();
  if (diff_size) {
    std::cout << "K PASO CHAVALES" << std::endl;
  }
}

void mutation() {
  for (auto &member: population) {
    for (uint r = 0; r < sudoku_n; r++) {
      if (randfloat(generator) < PM1) {
        if (member.non_given_n(r) >= 2) {
          member.mutate(r); // exchange 2 numbers in row
        }
      }
      if (randfloat(generator) < PM2) {
        member.reinitialize(r); // nuke row
        bool good = member.sanity_check();
        if (not good) {
          printf("help\n");
        }
      }
    }

  }
}

void local_search_cols() {
  for (auto &member: population) {
    // record all illegals
    vector<uint> illegals = member.illegal_cols();
    for (auto &col: illegals) {
      bool able[member.length];
      for (uint i = 0; i < member.length; i++) {
        able[i] = true;
      }
      uint other = get_another(illegals, col); // randomly select another
      uint a_mask = member.repeat_col_mask(col);
      uint b_mask = member.repeat_col_mask(other);
      uint match_mask = a_mask & b_mask;
      // if repeat numbers are in the same row
      if (match_mask) {
        uint row = 0;
        while (row < member.length) {
          if (match_mask & (1 << row)) {
            uint a_num = member.get(row, col);
            uint b_num = member.get(row, other);
            if (!(able[a_num] && able[b_num])) {
              row++;
              continue;
            }
            able[a_num] = false;
            able[b_num] = false;
            // if repeat numbers do not exist in both columns
            if (!member.num_in_col(a_num, other) && !member.num_in_col(b_num, col)) {
              // swap
              member.set(row, col, b_num);
              member.set(row, other, a_num);
            }
          }
          row++;
        }
      }
    }
  }
}

void local_search_block() {
  for (auto &member: population) {
    // record all illegals
    vector<uint> illegals = member.illegal_blocks();
    for (auto &block: illegals) {
      bool able[member.length];

      // array to check each number is swapped at most once
      for (uint i = 0; i < member.length; i++) {
        able[i] = true;
      }

      uint other = get_another(illegals, block); // randomly select another
      uint a_spotted = 0, b_spotted = 0;
      uint a_mask = member.repeat_block_mask(block, a_spotted); // get mask of repeated numbers in block
      uint b_mask = member.repeat_block_mask(other, b_spotted);
      uint rows = member.get_block_width();

      for (uint i = 0; i < rows * rows; i++) {
        // if number is marked as repeated
        /*   a          b
         * 1 2 3      6 9 3
         * 4 5 2      1 6 4
         * 7 8 9      5 7 8
         *
         * a_mask -> 0b000 100 010
         * b_mask -> 0b000 010 001
         *
         * a_spotted -> 0b000 000 010   (2)
         * b_spotted -> 0b000 100 000   (6)
         *
         * i = 1
         * a_mask & 1<<i (0b10) == true (si numero en cuestion es repetido en a)
         * a_num = 2
         *
         * (b_spotted & 1<<(a_num-1)) (0b1) == false
         */
        if (a_mask & (1 << i)) {
          uint a_num = member.block_get(i / rows, i % rows, block);
          // but number is in B, unmark
          if (b_spotted & 1 << (a_num - 1)) {
            a_mask ^= 1 << i;
          }
        }
      }

      for (uint row = 0; row < rows; row++) {
        uint rmask_a = a_mask & (0b111 << (row * 3)); // get mask of repeated numbers in row
        uint rmask_b = b_mask & (0b111 << (row * 3));

        //zeros_from_right(uint) :   0b00100 --> 2  ;  0b00010 --> 1  ;  0b00001 --> 0
        while (rmask_a && rmask_b) { // there is a repeat in both rows
          uint a_col = zeros_from_right(rmask_a);  // 0b 001 001 000
          uint b_col = zeros_from_right(rmask_b);  // 0b 000 110 000

          uint a_num = member.block_get(row, a_col, block);
          uint b_num = member.block_get(row, b_col, other);
          // if repeat numbers do not exist in both blocks  -- should be taken care of already
          //if(in_a_n_not_b & 1<<(a_num-1) && in_b_n_not_a & 1<<(b_num-1)) {
          //if(!member.num_in_block(a_num, other) && !member.num_in_block(b_num, block)){
          if (able[a_num] && able[b_num]) {
            // swap
            member.block_set(row, a_col, block, b_num);
            member.block_set(row, b_col, other, a_num);

            able[a_num] = false;
            able[b_num] = false;
          }

          rmask_a ^= 1 << a_col;
          rmask_b ^= 1 << b_col;
        }
      }
    }
  }
}

void elite_learning() {
  // TODO: Replace the worst individuals with elite members



  // Replace or reinitialize the worst members with a random member of the elite population
}
uint best_sudoker = 300;
uint block_width = 3;
sudoku_n = block_width*block_width;
uint sudoku_space = sudoku_n*sudoku_n;
Memory<uc> grid(device, sudoku_space*POPULATION_SIZE);
Memory<uc> occupancy(device, sudoku_space*POPULATION_SIZE);
Memory<uc> non_given(device, sudoku_n*POPULATION_SIZE);
Memory<uc> mistakes(device, sudoku_n*2*POPULATION_SIZE);
Memory<uc> scores(device, POPULATION_SIZE);
Kernel fitnessColKernel(grid, mistakes);
Kernel fitnessBlockKernel(grid, mistakes);
Kernel mistakesAddKernel(mistakes, scores);
Kernel mutateRowKernel(grid, non_given, rand_a, rand_b, seed, chance);
Kernel reinitRowKernel(grid, non_given, rand_a, rand_b, seed, chance);

std::vector<uint> sort_scores_idxs(Memory<uc> &scores, uc size) {
  // Create a vector of indices
  std::vector<uint> indices(size);
  for (int i = 0; i < size; ++i) {
    indices[i] = i;
  }

  // Sort indices based on scores
  std::sort(indices.begin(), indices.end(), [&scores](int a, int b) {
      return scores[a] < scores[b];
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
      uc hint = (c == '0') ? 1 : 0;
      occupancy[i] = hint;
      grid[i] = hint ? (solution.at(i) - '0') : 0;
      s++;
    }
  }
  // eval population
  fitnessColKernel.enqueue(); // thIdx = global_idx;  -->  1 thread --> 1 col
  fitnessBlockKernel.run(); //                      -->  1 thread --> 1 block
  mistakesAddKernel.run();
  // fCK -> mistakes[m+0:0+8]
  // fBK -> mistakes[m+9:0+17]
  // mAK -> sum(mistakes[m:m+17]) --> scores[m]

  vector<uint> sorted_idx = sort_scores_idxs(scores, POPULATION_SIZE);

  while (MAX_GENERATIONS) {
    // tournament selection

    // cross over
    crossover();

    population.clear();
    population = new_population;
    new_population.clear();

    // mutation
    // 1 thread --> 1 row
    mutateRowKernel().enqueue();
    reinitRowKernel().run();

    // column LS
    fitnessColKernel.enqueue();
    local_search_cols();

    // Sub-block LS
    fitnessColKernel.enqueue();
    local_search_block();

    // eval population
    fitnessColKernel.enqueue(); // thIdx = global_idx;  -->  1 thread --> 1 col
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
  //best_score = scores[0];
  best_score = scores[best_sudoker];
  if (best_score) {
    std::cout << "It made " << best_score << " mistakes\n";
  } else {
    std::cout << "It is a correct solution!\n";
  }
  return 0;
}
