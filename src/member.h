#include <memory>
#include <random>
#include <string>
#include <algorithm>
#include <unordered_set>

using std::vector;
using std::string;

typedef unsigned char uc;
typedef unsigned int uint;
typedef std::uniform_int_distribution<uint> rand_uint;
typedef std::uniform_real_distribution<float> rand_float;


class Member {
private:
  std::unique_ptr<uc[]> occupancy, grid;
  const uint width = 9;
public:
    const uint get_width() const;

private:
    uint sudoku_size, block_width, checksum, score;


  void _init(const uint edge_len);

  uint bad_col(int c);      // 1 if col has repeated numbers
  uint bad_row(int r);      // 1 if row has repeated numbers
  uint bad_block(int b);    // 1 if block has repeated numbers
  uint check_sum(uint *arr);// 1 if sum(arr) == summation of k from 1 to width

  void initial_guess();

  void row_guess(uint row);

public:
  uint length;
  bool is_parent = false;

  Member(const uint edge_len);
  Member(const Member &member);
  Member() = default;
  ~Member() = default;

  // LSGA methods
  uint fitness();                             // counts flaws and returns value
  void auto_fitness();                        // counts flaws, 0 if perfect
  uint get_fitness();                         // returns last calculated fitness
  void exchange(uint row, Member &fuck_buddy);// makes offspring
  void mutate(uint row);                      // changes chromosomes
  uint non_given_n(uint row);                 // num of non clue spaces in row
  void reinitialize(uint row);                // makes a new chromosome

  // LSGA utils
  vector<uint> illegal_cols();              // array of invalid col idx
  vector<uint> illegal_blocks();            // array of invalid block idx
  uint repeat_col_mask(uint col);           // mask of positions where repeated numbers are present
  uint repeat_block_mask(uint block,
                         uint &spotted); // flattened square mask of positions where repeated numbers are present
  bool num_in_col(uint value, uint col);    // check if num is in col
  bool num_in_block(uint value, uint block);// check if num is in block

  // utils
  uint get(uint row, uint col);                  // get value from global idx
  uint set(uint row, uint col, uint value);      // set value at global idx
  uint block_get(uint row, uint col, uint block);// get value from block
  uint block_set(uint row, uint col, uint block, uint value);
  uint get_block_width();

  uint idx(uint i, uint j) const;                // global i,j idx
  uint bidx(uint i, uint j, uint block) const;   // i,j idx relative to block
  void load_sudoku(string solution, const string &not_hints);



  std::unique_ptr<uc[]> &get_grid();
  bool sanity_check();
  void row_check();
  void hint_check();

  // operator overrides
  friend std::ostream &operator<<(std::ostream &os, const Member &member);// Display member to console
  bool operator==(const Member &other) const;
  bool operator!=(const Member &other) const;
  bool operator<(const Member &other) const;
  bool operator>(const Member &other) const;
  Member &operator=(const Member &member);
};

extern uint POPULATION_SIZE;
extern std::random_device generator;
extern std::default_random_engine rng;
extern rand_float randfloat;
extern rand_uint randint;

template<typename T>
T& get_another(vector<T> &vec, const T &not_this) {
  T *other = nullptr;
  do {
    uint idx = randint(generator) % vec.size();
    other = &vec[idx];
  } while (*other == not_this);
  return *other;
}

