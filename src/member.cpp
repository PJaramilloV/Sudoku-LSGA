#include "member.h"
#include <memory>
#include <unordered_set>
#include <math.h>
#include <iostream>
#include <format>
#include <cstring>

using std::unordered_set;

void Member::_init(const uint edge_len){
  this->block_width = uint(sqrt(edge_len));
  if(block_width * block_width != width){
    std::string error = std::format("Invalid Edge Length {}. It is not a square.", edge_len);
    throw std::out_of_range(error);
  }
  this->length = edge_len;
  this->sudoku_size = width * length;
  this->checksum = width*(width+1)/2;
  this->occupancy = std::make_unique<uc[]>(sudoku_size);
  this->grid = std::make_unique<uc[]>(sudoku_size);
}

Member::Member(const uint edge_len) : width(edge_len){
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
}


uint Member::check_sum(uint *arr){
  uint sum = 0;
  for(int r=0; r<width; r++){
    sum += arr[r];
  }
  return sum!=checksum;
}

uint Member::bad_col(int c){
  uint col[width+1];
  for(int r=0; r<width; r++){
    uint val = grid[idx(r, c)];
    col[val] = val;
  }
  return check_sum(col);
}

uint Member::bad_block(int b){
  uint block[width+1];
  for(int i=0; i<block_width; i++){
    for(int j=0; j<block_width; j++){
      uint val = grid[bidx(i, j, b)];
      block[val] = val;
    }
  }
  return check_sum(block);
}


uint Member::fitness() {
  uint mistakes = 0;
  for(int i=0; i<width; i++){
    mistakes += bad_col(i);
    mistakes += bad_block(i);
  }
  return mistakes;
}

uint Member::idx(uint i, uint j) const {
  return i*width + j;
}

uint Member::bidx(uint i, uint j, uint block) const {
  uint b_row = block/block_width;
  uint b_col = block%block_width;
  uint up_left_corner = b_col*block_width + b_row;
  return up_left_corner + idx(i,j);
}



bool Member::operator==(const Member &other) const {
  return occupancy == other.occupancy;
}

bool Member::operator!=(const Member &other) const {
  return !((*this)==other);
}

Member &Member::operator=(const Member &member) {
  _init(member.width);
  for(uint i=0; i<sudoku_size; i++){
    grid[i] = member.grid[i];
    occupancy[i] = member.occupancy[i];
  }
  return *this;
}

uint Member::get(uint row, uint col) {
  return grid[idx(row,col)];
}

uint Member::set(uint row, uint col, uint value) {
  return grid[idx(row,col)] = value;
}

bool Member::num_in_col(uint value, uint col) {
  for(uint r=0; r<width; r++){
    if(grid[idx(r,col)] == value) return true;
  }
  return false;
}

bool Member::num_in_block(uint value, uint block) {
  for(uint r=0; r<block_width; r++){
    for(uint c=0; c<block_width; c++) {
      if (grid[bidx(r, c, block)] == value) return true;
    }
  }
  return false;
}

uint Member::repeat_col_mask(uint col) {
  uint mask = 0;
  uint spotted = 0;
  for(uint row=0; row<width; row++){
    uint val = grid[idx(row, col)];
    uint bin_val = 1<<val;
    if(spotted & bin_val){
      mask += 1<<row;
      spotted += bin_val;
    }
  }
  return mask;
}

uint Member::repeat_block_mask(uint block) {
  uint mask = 0;
  uint spotted = 0;
  for(uint row=0; row<block_width; row++){
    for(uint col=0; col<block_width; col++){
      uint val = grid[bidx(row, col, block)];
      uint bin_val = 1<<val;
      if(spotted & bin_val){
        mask += 1<<(row*block_width + col);
        spotted += bin_val;
      }
  }}
  return mask;
}

vector<uint> Member::illegal_cols(){
  vector<uint> bad;
  for(int c=0; c<width; c++){
    if(bad_col(c)){
      bad.push_back(c);
    }
  }
  return bad;
}

vector<uint> Member::illegal_blocks(){
  vector<uint> bad;
  for(int b=0; b<width; b++){
    if(bad_block(b)){
      bad.push_back(b);
    }
  }
  return bad;
}

uint Member::non_given_n(uint row) {
  uint non_given = width;
  for(uint col=0; col<width; col++){
    non_given -= occupancy[idx(row, col)];
  }
  return non_given;
}

void Member::load_sudoku(string solution, const string& not_hints) {
  uint i = 0;
  for(auto &c: not_hints){
    uint hint = (c=='0')? '1': '0'; // cell occupied by hint if it is not editable
    occupancy[i] = hint;
    grid[i] = hint? solution.at(i) : 0; // apply hint, else leave as 0 to symbolize empty
  }
}

void print_grid(std::unique_ptr<uc[]> grid,  uint block_width, uint width, std::ostream os){
  for(uint row=0; row<width; row++){
    os << "\t";
    uint i=0;
    for(uint col=0; col<width; col++) {
      uc val = grid[row*width+col];
      os << (val? val : " ") << " ";

      i += 1;
      if(i==block_width){
        i = 0;
        os << " ";
      }
    }
    os << "\n"
  }
}

std::ostream & operator<<(std::ostream &os, const Member &member){// Display matrix to console
  os << "Member: Sudoku\n [\n ";

  os << "]\n";
  return os;
}


