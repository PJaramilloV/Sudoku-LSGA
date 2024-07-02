#include <memory>
#include <random>
#include <string>
#include <unordered_set>

using std::vector;
using std::string;

typedef unsigned char uc;
typedef unsigned int uint;
typedef std::uniform_int_distribution<uint> rand_uint;
typedef std::uniform_real_distribution<float> rand_float;


class Member{
private:
  std::unique_ptr<uc[]> occupancy, grid;
  const uint width=9;
  uint sudoku_size, block_width, checksum;

  void _init(const uint edge_len);
  uint bad_col(int c);      // 1 if col has repeated numbers
  uint bad_block(int b);    // 1 if block has repeated numbers
  uint check_sum(uint *arr);// 1 if sum(arr) == summation of k from 1 to width
public:
  uint length;

  Member(const uint edge_len);
  Member(const Member &member);
  Member() = default;
  ~Member() = default;

  // LSGA methods
  uint fitness();                             // counts flaws, 0 if perfect
  void exchange(uint row, Member &fuck_buddy);// makes offspring
  void mutate(uint row);                      // changes chromosomes
  uint non_given_n(uint row);                 // num of non clue spaces in row
  void reinitialize(uint row);                // makes a new chromosome

  // LSGA utils
  vector<uint> illegal_cols();              // array of invalid col idx
  vector<uint> illegal_blocks();            // array of invalid block idx
  uint repeat_col_mask(uint col);           // mask of positions where repeated numbers are present
  uint repeat_block_mask(uint block);       // square mask of positions where repeated numbers are present
  bool num_in_col(uint value, uint col);    // check if num is in col
  bool num_in_block(uint value, uint block);// check if num is in block

  // utils
  uint get(uint row, uint col);                 // get value from global idx
  uint set(uint row, uint col, uint value);     // set value at global idx
  uint idx(uint i, uint j) const;               // global i,j idx
  uint bidx(uint i, uint j, uint block) const;  // i,j idx relative to block
  void load_sudoku(string solution, const string& not_hints);

  // operator overrides
  friend std::ostream & operator<<(std::ostream &os, const Member &member);// Display member to console
  bool operator==(const Member &other) const;
  bool operator!=(const Member &other) const;
  Member &operator=(const Member &member);
};

