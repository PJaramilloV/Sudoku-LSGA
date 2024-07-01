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
uint generations = 10; // 10000
uint sudoku_n;
float PC1=0.2, PC2=0.1, PM1=0.3, PM2=0.05;
vector<Member> population;
std::random_device generator;
rand_float random(0,1);
rand_uint randint(0,POPULATION_SIZE);

template <typename T>
T get_another(const vector<T> vec, const T&not_this){
  T other;
  do{
    uint idx = randint(generator)%vec.size();
    other = vec[idx];
  }while(other == not_this);
  return other;
}

void crossover(){
  for(auto &member: population){
    if(random(generator) < PC1){
      // select second parent from population
      Member fuck_buddy = get_another(population, member);
      Member child = member;
      for(uint r=0; r<sudoku_n; r++){
        if(random(generator) < PC2){
          // parents exchange rows
          child.exchange(r, fuck_buddy);
        }
      }
    }
    // save offspring
  }
}

void mutation(){
  for(auto &member: population){
    for(uint r=0; r<sudoku_n; r++){
      if(random(generator) < PM1){
        if(member.non_given_n(r) >= 2){
          member.mutate(r); // exchange 2 numbers in row
        }
      }
      if(random(generator) < PM2){
        member.reinitialize(r); // nuke row
      }
    }
  }
}

void local_search_cols(){
  for(auto &member: population){
    // record all illegals
    vector<uint> illegals = member.illegal_cols();
    for(auto &col: illegals){
      bool able[member.length];
      for(uint i=0; i<member.length; i++){
        able[i] = true;
      }
      uint other = get_another(illegals, col); // randomly select another
      uint a_mask = member.repeat_col_mask(col);
      uint b_mask = member.repeat_col_mask(other);
      uint match_mask = a_mask & b_mask;
      // if repeat numbers are in the same row
      if(match_mask){
        uint row = 0;
        while(row < member.length){
          if(match_mask & (1<<row)){
            uint a_num = member.get(row, col);
            uint b_num = member.get(row, other);
            if(!(able[a_num] && able[b_num])) {
              row++;
              continue;
            }
            able[a_num] = false;
            able[b_num] = false;
            // if repeat numbers do not exist in both columns
            if(!member.num_in_col(a_num, other) && !member.num_in_col(b_num, col)){
              // swap
              member.set(row,col,b_num);
              member.set(row,other,a_num);
            }
          }
          row++;
        }
      }
    }
  }
}

/**
 *  1 2 2
 *  4 5 6
 *  8 8 9
 *        --> mask=
 */
// for row:
//  mmask_a = a_mask & 0b111<<(row*3)  --> 100
//  mmask_b = b_mask & 0b111<<(row*3)  --> 010
//  match_mmask = mmask_a && mmask_b   --> 1
//  if (match_mmask):
//    for bit in match_mmask:
//      swap nums in row
//
void local_search_block(){
  for(auto &member: population){
    // record all illegals
    vector<uint> illegals = member.illegal_blocks();
    for(auto &block: illegals){
      // TODO: - Able array
      //       - Match and access
      uint other = get_another(illegals, block); // randomly select another
      uint a_mask = member.repeat_block_mask(block);
      uint b_mask = member.repeat_block_mask(other);
      uint match_mask = a_mask & b_mask; // TODO: change this to a row match (not cell)
      // if repeat numbers are in the same row
      if(match_mask){
        uint local = 0;
        while(local < member.length){
          if(match_mask & (1<<local)){ // TODO: change cond
            uint a_num = member.get(local, block);
            uint b_num = member.get(local, other);
            // if repeat numbers do not exist in both blocks
            if(!member.num_in_block(a_num, other) && !member.num_in_block(b_num, block)){
              // swap
              member.set(local,block,b_num);
              member.set(local,other,a_num);
            }
          }
          local++;
        }
      }
    }
  }
}


int main(int argc, char* argv[]) {
  for(int i=0; i<POPULATION_SIZE; i++){
    population[i] = Member(sudoku_n);
  }
  return 0;
}
