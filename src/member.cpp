#include "member.h"
#include <memory>
#include <unordered_set>
#include <cmath>
#include <iostream>
#include <format>
#include <cstring>

using std::unordered_set;

void Member::_init(const uint edge_len) {
  this->block_width = uint(sqrt(edge_len));
  if (block_width * block_width != width) {
    std::string error = std::format("Invalid Edge Length {}. It is not a square.", edge_len);
    throw std::out_of_range(error);
  }
  this->length = edge_len;
  this->sudoku_size = width * length;
  this->checksum = width * (width + 1) / 2;
  this->occupancy = std::make_unique<uc[]>(sudoku_size);
  this->grid = std::make_unique<uc[]>(sudoku_size);
  this->score = width * 2 + 1;
}

void Member::initial_guess() {
  for (uint row = 0; row < width; row++) {
    row_guess(row);
  }
}

void Member::row_guess(uint row) {
  vector<uint> nums(width);
  for (int i = 0; i < width; i++) {
    nums[i] = i + 1;
  }
  for (int col = 0; col < width; col++) {
    uint i = idx(row, col);
    if (occupancy[i]) {
      // move removed element to the end
      auto newEnd = std::remove(nums.begin(), nums.end(), grid[i]);
      // erase removed element
      nums.erase(newEnd, nums.end());
    }
  }
  std::shuffle(nums.begin(), nums.end(), rng);
  for (int col = 0; col < width; col++) {
    uint i = idx(row, col);
    if(occupancy[i])
      continue;
    grid[i] = nums.back();
    nums.pop_back();
  }
}

bool Member::sanity_check(){
  for(uint row=0; row<width;row++){
    for(uint col=0; col<width;col++){
      if(grid[idx(row, col)] == 0){
        return false;
      }
    }
  }
  return true;
}

Member::Member(const uint edge_len) : width(edge_len) {
  _init(edge_len);
}

Member::Member(const Member &member) : width(member.width) {
  this->block_width = member.block_width;
  this->length = member.length;
  this->sudoku_size = member.sudoku_size;
  this->occupancy = std::make_unique<uc[]>(sudoku_size);
  this->grid = std::make_unique<uc[]>(sudoku_size);
  std::memcpy(occupancy.get(), member.occupancy.get(), sudoku_size);
  std::memcpy(grid.get(), member.grid.get(), sudoku_size);
  this->checksum = member.checksum;
  this->score = member.score;
}

Member &Member::operator=(const Member &member) {
  _init(member.width);
  for (uint i = 0; i < sudoku_size; i++) {
    grid[i] = member.grid[i];
    occupancy[i] = member.occupancy[i];
  }
  return *this;
}

void Member::exchange(uint row, Member &fuck_buddy) {
  for (uint col = 0; col < width; col++) {
    uint i = idx(row, col);
    grid[i] = fuck_buddy.grid[i];
  }
}

void Member::mutate(uint row) {
  // swap two non given numbers in row
  vector<uint> non_given;
  for (uint i = 0; i < width; i++) {
    if (!occupancy[idx(row, i)]) {
      non_given.push_back(i);
    }
  }
  uint first = get_another(non_given, width + 1);
  uint second = get_another(non_given, first);
  uint f_idx = idx(row, first);
  uint s_idx = idx(row, second);
  uint tmp = grid[f_idx];
  grid[f_idx] = grid[s_idx];
  grid[s_idx] = tmp;
}

void Member::reinitialize(uint row) {
  uint select = 0;
  vector<uint> not_occupied(width + 1);
  for (uint col = 0; col < width; col++) {
    uint i = idx(row, col);
    if (!occupancy[i]) {
      not_occupied[select] = grid[i];
      select++;
    }
  }

  std::shuffle(std::begin(not_occupied), std::begin(not_occupied)+select, rng);
  select = 0;
  for (uint col = 0; col < width; col++) {
    uint i = idx(row, col);
    if (!occupancy[i]) {
      grid[i] = not_occupied[select];
      select++;
    }
  }
}

uint Member::check_sum(uint *arr) {
  uint sum = 0;
  for (int r = 0; r < width; r++) {
    sum += arr[r];
  }
  return sum != checksum;
}

uint Member::bad_col(int c) {
  uint col[width + 1];
  for (int r = 0; r < width; r++) {
    uint val = grid[idx(r, c)];
    col[val] = val;
  }
  return check_sum(col);
}

uint Member::bad_block(int b) {
  uint block[width + 1];
  for (int i = 0; i < block_width; i++) {
    for (int j = 0; j < block_width; j++) {
      uint val = grid[bidx(i, j, b)];
      block[val] = val;
    }
  }
  return check_sum(block);
}


uint Member::fitness() {
  uint mistakes = 0;
  for (int i = 0; i < width; i++) {
    mistakes += bad_col(i);
    mistakes += bad_block(i);
  }
  return mistakes;
}

void Member::auto_fitness() {
  score = fitness();
}

uint Member::get_fitness() {
  return score;
}

uint Member::idx(uint i, uint j) const {
  return i * width + j;
}

uint Member::bidx(uint i, uint j, uint block) const {
  uint b_row = block / block_width;
  uint b_col = block % block_width;
  uint up_left_corner = b_col * block_width + b_row;
  return up_left_corner + idx(i, j);
}


bool Member::operator==(const Member &other) const {
  return occupancy == other.occupancy;
}

bool Member::operator!=(const Member &other) const {
  return !((*this) == other);
}

bool Member::operator<(const Member &other) const {
  return score < other.score;
}

bool Member::operator>(const Member &other) const {
  return score > other.score;
}


uint Member::get(uint row, uint col) {
  return grid[idx(row, col)];
}

uint Member::block_get(uint row, uint col, uint block) {
  return grid[bidx(row, col, block)];
}

uint Member::set(uint row, uint col, uint value) {
  return grid[idx(row, col)] = value;
}

uint Member::block_set(uint row, uint col, uint block, uint value) {
  return grid[bidx(row, col, block)] = value;
}

bool Member::num_in_col(uint value, uint col) {
  for (uint r = 0; r < width; r++) {
    if (grid[idx(r, col)] == value) return true;
  }
  return false;
}

bool Member::num_in_block(uint value, uint block) {
  for (uint r = 0; r < block_width; r++) {
    for (uint c = 0; c < block_width; c++) {
      if (grid[bidx(r, c, block)] == value) return true;
    }
  }
  return false;
}

uint Member::repeat_col_mask(uint col) {
  uint mask = 0;     // mask of repeated positions
  uint spotted = 0;  // mask of spotted values, starts as 0b000 000 000
  uint first_spotted[width];  // array to store first spotted positions
  for (uint row = 0; row < width; row++) {
    uint i = idx(row, col);
    uint val = grid[i]; // find 2
    uint c_val = val - 1;
    uint bin_val = 1 << c_val;       // 2 is 0b000 000 010
    if (spotted & bin_val) {    // a repeated 2 is declared by 0b000 000 010 & 0b000 000 010 ---> true
      uc not_hint = !occupancy[i];
      mask |= (not_hint << row);       // mark position of repeated 2
      mask |= not_hint ? first_spotted[c_val] : 0;
    } else {
      spotted |= bin_val;     // store 2 as spotted as 0b000 000 010
      first_spotted[c_val] = 1 << row;
    }
  }
  return mask;
}

uint Member::repeat_block_mask(uint block, uint &spotted) {
  uint mask = 0;     // block mask of repeated positions
  //uint spotted = 0;  // starts as 0b000 000 000
  uint first_spotted[width];  // array to store first spotted positions
  for (uint row = 0; row < block_width; row++) {
    for (uint col = 0; col < block_width; col++) {
      uint i = bidx(row, col, block);
      uint val = grid[i]; // find 2
      uint c_val = val - 1;
      uint bin_val = 1 << c_val;       // 2 is 0b000 000 010
      if (spotted & bin_val) {
        uc not_hint = !occupancy[i];
        mask |= not_hint << (row * block_width + col); // mark position of 2
        mask |= not_hint ? first_spotted[c_val] : 0;
      } else {
        spotted |= bin_val;                   // store 2 as 0b000 000 010
        first_spotted[c_val] = 1 << row;
      }
    }
  }
  return mask;
}

vector<uint> Member::illegal_cols() {
  vector<uint> bad;
  for (int c = 0; c < width; c++) {
    if (bad_col(c)) {
      bad.push_back(c);
    }
  }
  return bad;
}

vector<uint> Member::illegal_blocks() {
  vector<uint> bad;
  for (int b = 0; b < width; b++) {
    if (bad_block(b)) {
      bad.push_back(b);
    }
  }
  return bad;
}

uint Member::non_given_n(uint row) {
  uint non_given = width;
  for (uint col = 0; col < width; col++) {
    non_given -= occupancy[idx(row, col)];
  }
  return non_given;
}

uint Member::get_block_width() {
  return block_width;
}

void Member::load_sudoku(string solution, const string &not_hints) {
  uint i = 0;
  for (auto &c: not_hints) {
    uc hint = (c == '0') ? 1 : 0; // cell occupied by hint if it is not editable
    occupancy[i] = hint;
    grid[i] = hint ? (solution.at(i) - '0') : 0; // apply hint, else leave as 0 to symbolize empty
    i++;
  }
  initial_guess();
}

//std::unique_ptr<uc[]> Member::get_grid() {
//  return grid;
//}
//
//void print_grid(uc* grid, uint block_width, uint width, std::ostream os) {
//  for (uint row = 0; row < width; row++) {
//    os << "\t";
//    uint i = 0;
//    for (uint col = 0; col < width; col++) {
//      uc val = grid[row * width + col];
//      os << (val ? std::to_string(val) : " ") << " ";
//      i += 1;
//      if (i == block_width) {
//        i = 0;
//        os << " ";
//      }
//    }
//    os << "\n";
//  }
//  os << "\n";
//}
//
//std::ostream &operator<<(std::ostream &os, const Member &member) {// Display matrix to console
//  os << "Member: Sudoku\n [\n ";
//  print_grid(member.get_grid(), member.get_block_width(), member.length, os);
//  os << "]\n";
//  return os;
//}


