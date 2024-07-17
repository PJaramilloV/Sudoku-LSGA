#include <chrono>
#include <iostream>
#include "member.h"

struct Times {
    long create_data;
    long execution;

    long total() { return create_data + execution; }
};


Times t;
uint POPULATION_SIZE = 150;
uint ELITE_SIZE = 50;
std::random_device generator;
std::default_random_engine rng = std::default_random_engine{generator()};
rand_float randfloat(0, 1);
rand_uint randint(0, POPULATION_SIZE);

uint MAX_GENERATIONS = 200;
uint sudoku_n = 9;
uint best_score = sudoku_n * 2 + 1;
float PC1 = 0.2, PC2 = 0.1, PM1 = 0.3, PM2 = 0.05;
vector<Member> population, new_population, elite;

Member best_sudoker;

uint zeros_from_right(uint bitmask, uint skip = 0) {
  uint res = 0;
  while (!(bitmask & 0b1) || skip) { // rightmost bit == 0
    res++;
    bitmask >>= 1;
  }
  return res;
}

uint *get_another_illegal_block_in_row(uint block, vector<uint> &illegals) {
  uint other_idx = randint(generator) % illegals.size();
  uint *other_ptr = &illegals[other_idx];
  bool same_row = block / 3 == *other_ptr / 3;
  uint tries = illegals.size();
  while ((block == *other_ptr || !same_row) && tries--) {
    other_idx = (other_idx + 1) % illegals.size();
    other_ptr = &illegals[other_idx];
    same_row = block / 3 == *other_ptr / 3;
  }
  if (tries == 0) { // no block was found
    return nullptr;
  }
  return other_ptr;
}


void crossover() {
  uint virgin_count = population.size();

  for (auto &member: population) {
    if (member.is_parent) {
      continue;
    }
    if (randfloat(generator) < PC1 && virgin_count >= 2) {
      Member child1 = member;
      Member child2;

      uint partner_idx = randint(generator) % population.size();
      Member *partner = &population[partner_idx];
      while (partner->is_parent || *partner == member) {
        partner_idx = (partner_idx + 1) % population.size();
        partner = &population[partner_idx];
      }

      child2 = *partner;
      for (uint r = 0; r < sudoku_n; r++) {
        if (randfloat(generator) < PC2) {
          // parents exchange rows
          child1.exchange(r, *partner);
          child2.exchange(r, member);
        }
      }
      member.is_parent = true;
      partner->is_parent = true;

      // save offspring
      new_population.push_back(child1);
      new_population.push_back(child2);

      virgin_count -= 2;
    } else {
      new_population.push_back(member);
      member.is_parent = true;

      virgin_count -= 1;
    }
  }
  bool diff_size = population.size() != new_population.size();
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
      if (illegals.size() == 1) continue;
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

      uint *other_ptr = get_another_illegal_block_in_row(block, illegals);
      if (other_ptr == nullptr) continue;
      uint other = *other_ptr;

      uint a_spotted = 0, b_spotted = 0;
      uint a_mask = member.repeat_block_mask(block, a_spotted); // get mask of repeated numbers in block
      uint b_mask = member.repeat_block_mask(other, b_spotted);
      uint rows = member.get_block_width();

      for (uint i = 0; i < rows * rows; i++) {
        // if number is marked as repeated
        if (a_mask & (1 << i)) {
          uint a_num = member.block_get(i / rows, i % rows, block);
          // but number is in B, unmark
          if (b_spotted & (1 << (a_num - 1))) {
            a_mask ^= (1 << i);
          }
        }
      }

      for (uint row = 0; row < rows; row++) {
        uint rmask_a = a_mask & (0b111 << (row * 3)); // get mask of repeated numbers in row
        uint rmask_b = b_mask & (0b111 << (row * 3));

        //zeros_from_right(uint) :   0b00100 --> 2  ;  0b00010 --> 1  ;  0b00001 --> 0
        while (rmask_a && rmask_b) { // there are repeats in both rows
          uint a_col = zeros_from_right(rmask_a);  // 0b 001 001 000
          uint b_col = zeros_from_right(rmask_b);  // 0b 000 110 000

          uint a_num = member.block_get(row, a_col % 3, block);
          uint b_num = member.block_get(row, b_col % 3, other);
          // if repeat numbers do not exist in both blocks  -- should be taken care of already
          if (able[a_num] && able[b_num]) {
            // swap
            member.block_set(row, a_col % 3, block, b_num);
            member.block_set(row, b_col % 3, other, a_num);

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
  if (elite.size() == ELITE_SIZE) {
    // Select random elite member
    uint idx = randint(generator) % elite.size();
    Member random_elite = elite[idx];

    // Calc. reinitialization probability
    float Pb = (float) (population[0].get_fitness() - random_elite.get_fitness()) / (float) population[0].get_fitness();

    if (randfloat(generator) < Pb) {
      // Replace the worst individual with a random elite member
      population[0] = random_elite;
    } else {
      // Reinitialize the worst member
      for (uint row = 0; row < sudoku_n; row++) {
        population[0].reinitialize(row);
      }
      population[0].auto_fitness();
    }

    // elite is ordered asc
    std::sort(elite.begin(), elite.end());
    // population is ordered desc
    std::sort(population.begin(), population.end(), std::greater<>());

    uint elite_idx = elite.size() - 1;
    uint pop_idx = population.size() - 1;
    while (elite[elite_idx].get_fitness() > population[pop_idx].get_fitness() && elite_idx > 0 && pop_idx > 0) {
      // Current best is better than elite
      elite_idx--;
    }
    // replace the worst elite members with the bests from current population
    while(elite_idx < ELITE_SIZE && elite[elite_idx] > population[pop_idx]) {
      elite[elite_idx] = population[pop_idx];
      pop_idx--;
      elite_idx++;
    }

  } else if (elite.empty()) {
    // initialize elite population with the best individuals
    for (int i = 0; i < ELITE_SIZE; i++) {
      elite.push_back(population[population.size() - 1 - i]);
    }
  }
}

void population_row_check() {
  for (auto &member: population) {
    member.row_check();
  }
}

void population_hint_check() {
  for (auto &member: population) {
    member.hint_check();
  }
}

int main() {
  string solution = "918745632532619784647283915286534179394178256751926843169457328825361497473892561";
  string editable = "001011111100010101110101011100101001011111110100101001110101011101010001111110100";

  // init population
  for (int i = 0; i < POPULATION_SIZE; i++) {
    Member m = Member(sudoku_n);
    m.load_sudoku(solution, editable);
    population.push_back(m);
  }
  // eval population
  for (int mit = 0; mit < POPULATION_SIZE; mit++) {
    Member &someone = population[mit];
    someone.auto_fitness();
  }

  population_hint_check();
  population_row_check();
  while (MAX_GENERATIONS) {
    // crossover
    crossover();

    population.clear();
    population = new_population;
    new_population.clear();

    // mutation
    mutation();

    // column LS
    local_search_cols();

    // Sub-block LS
    local_search_block();

    // eval population
    for (int i = 0; i < POPULATION_SIZE; i++) {
      Member &member = population[i];
      member.auto_fitness();
    }

    // Sort members by fitness
    std::sort(population.begin(), population.end(), std::greater<>());

    // TODO: elite population learning
    elite_learning();

    // save best
    best_sudoker = population.back(); // If population is sorted

    // finish early if sudoku is solved
    if (best_sudoker.fitness() == 0) break;

    MAX_GENERATIONS--;
  }
  population_hint_check();
  population_row_check();
  std::cout << "The best solution found is\n " << best_sudoker;
  best_score = best_sudoker.get_fitness();
  if (best_score) {
    std::cout << "It made " << best_score << " mistakes\n";
    std::cout << "Worst score: " << population.front().get_fitness() << "\n";
  } else {
    std::cout << "It is a correct solution!\n";
  }
  return 0;
}
