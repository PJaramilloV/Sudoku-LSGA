#include "kernel.hpp" // note: unbalanced round brackets () are not allowed and string literals can't be arbitrarily long, so periodically interrupt with )+R(

string opencl_c_container() {
  return R( // ########################## begin of OpenCL C code ####################################################################

kernel void crossoverKernel(
                global uchar* grid, global uchar* newGrid, global int* crossingMap,
                const uint mult, const uint adder, const uint seed, const float chance,  const uint N){
  const uint thIdx = get_global_id(0);
  const uint member = thIdx / 81;
  if(member < N){
    const uint MAX_UINT = 0xFFFFFFFF;
    const uint column = thIdx % 9;
    const uint row = (thIdx % 81) / 9;
    const uint memberOffset = member*81;
    const uint randinteger = (mult*(seed + (member + row)*(row + 1)) + adder) % MAX_UINT; // same for all threads of the member's row
    const float randfloat = (float) randinteger / (float) MAX_UINT;
    const int partner = crossingMap[member];
    const uchar do_swap = (randfloat < chance) && (partner ^ -1); // 
    const uint gridPosition = row*9 + column;
    if(do_swap){
      newGrid[member*memberOffset + gridPosition] = grid[partner*memberOffset + gridPosition];
    }
  }
}

kernel void fitnessColKernel(global uchar* grid, global uchar *mistakes, const uint N){
  const uint thIdx = get_global_id(0);
  const uint member = thIdx / 81;
  const uint column = (thIdx % 81) / 9;
  const uint thLocal = thIdx % 9;
  const uint memberOffset = member*81;
  const uint localOffset = thLocal*9;
  uint bad = 0;
  local uint shared_mem[9];
  shared_mem[thLocal] = 0;
  barrier(CLK_LOCAL_MEM_FENCE);
  if (member < N){
    uchar val = grid[memberOffset + column +  localOffset]; // member grid origin + column offset + row*ID
    shared_mem[val-1] = val;
    barrier(CLK_LOCAL_MEM_FENCE);
    if (thLocal == 0){
      for (uint i=0; i < 9; i++){    // all threads can run because they all write the same data
        bad = shared_mem[i];
      }
      mistakes[member+column] = (bad != 45); // checksum, 1 if failed, 0 if passed
    }
  }
}

kernel void fitnessBlockKernel(global uchar* grid, global uchar* mistakes, const uint N){
  const uint thIdx = get_global_id(0);
  const uint member = thIdx / 81;
  const uint block = (thIdx % 81) / 9;
  const uint thLocal = thIdx % 9;
  const uint memberOffset = member*81;
  const uint localOffset = (thLocal/3)*9 + thLocal%3;
  uint bad = 0;
  local uint shared_mem[9];
  shared_mem[thLocal] = 0;
  barrier(CLK_LOCAL_MEM_FENCE);
  if (member < N){
    uchar val = grid[memberOffset + block +  localOffset]; // member grid origin + column offset + row*ID
    shared_mem[val-1] = val;
    barrier(CLK_LOCAL_MEM_FENCE);
    if (thLocal == 0){
      for (uint i=0; i < 9; i++){    // all threads can run because they all write the same data
        bad = shared_mem[i];
      }
      mistakes[member+block+9] = (bad != 45); // checksum, 1 if failed, 0 if passed
    }
  }
}

kernel void mistakesAddKernel(global uchar* mistakes, global uchar* scores, const uint N){
  local uchar shared_mem[18];

  const uint thIdx = get_global_id(0);
  const uint thLocal = get_local_id(0);
  const uint member = thIdx / 18;

  shared_mem[thLocal] = 0;

  barrier(CLK_LOCAL_MEM_FENCE);

  if (member < N){
    shared_mem[thLocal-1] = mistakes[member*18 + thLocal];
    barrier(CLK_LOCAL_MEM_FENCE);
    if (thLocal == 0){
      uint errors = 0;
      for (uint i=0; i<18; i++){
        errors += shared_mem[i];
      }
      scores[member] = errors;
    }
  }
}

kernel void mutateRowKernel(
              global uchar* grid, global uchar* non_given,
              const uint mult, const uint adder, const uint seed, const float chance, const uint N){
  const uint thIdx =  get_global_id(0);
  const uint member = thIdx / 81;
  const uint memberOffset = member*81;
  const uint row = thIdx % 9;
  if (member < N){
    const uint MAX_UINT = 0xFFFFFFFF;
    uint non_given_bits = non_given[row];
    uint can_swap = 0;
    for(uint i=0; i < 9; i++){
      can_swap += !((non_given_bits >> i) & 0b1); // add if not (is hint)
    }
    const uint randinteger = (mult*(seed + thIdx) + adder) % MAX_UINT;
    const float randfloat = (float) randinteger / (float) MAX_UINT;
    const uchar do_swap = (randfloat < chance) & (can_swap > 1);
    uint a_col = randinteger % 9;
    uint first = MAX_UINT;
    uint second = MAX_UINT;
    uint first_selected = 0;
    uint second_selected = 0;
    uint val = MAX_UINT;
    uint valid = 0; 
    for(uint i=0; i < 9; i++){  // choose columns to swap numbers from
      valid = !((non_given_bits >> i) & 0b1);        // can swap value if not hint
      val = ((!valid) & MAX_UINT) | (valid & a_col); // make value into default if not valid
      first_selected = first != MAX_UINT;            // cond to set first
      second_selected = second != MAX_UINT;          // cond to set second
      first = (first_selected & first) | ((!first_selected) & val); // keep first if set, change otherwise
      // keep second if set, set if first is set, must be default if first is not set
      second = (second_selected & second) | ((first_selected & !second_selected) & val) | ((!first_selected) & MAX_UINT );
      a_col += 6700417; // rotation by prime factor
      a_col %= 9;
    }
    const uint row_position = member*memberOffset + row*9;
    uint tmp = grid[row_position + first];
    grid[row_position + first] = grid[row_position + second];
    grid[row_position + second] = tmp;  
  }
}

//kernel void lsga_col_kernel(global uchar* grid, global uchar* occupancy, global uchar* mistakes, const uint N){
//  const uint member = get_global_id(0);
//  const uint member_offset = member*81;
//  if(member < N){
//    for (uint col=0; col < 9; col++){
//      uchar is_illegal = mistakes[member+col];
//      uint other = 0; // TODO: this line, make a way to get another column
//
//      uchar able[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
//      uint a_spot[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//      uint b_spot[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//      uint a_mask = 0;
//      uint b_mask = 0;
//      uint a_spotted = 0;
//      uint b_spotted = 0;
//      // find repeated numbers in both columns
//      for (uint row=0; row<9; row++){
//        uint a_i =  member_offset + (row*9 + col);
//        uint b_i =  member_offset + (row*9 + other);
//        uint a_val = grid[a_i] -1;
//        uint b_val = grid[b_i] -1;
//        uint a_bin_val = 1 << val;
//        uint b_bin_val = 1 << val;
//        uchar a_is_hint = occupancy[a_i];
//        uchar b_is_hint = occupancy[b_i];
//        uchar a_not_hint = 1 ^ a_is_hint;
//        uchar b_not_hint = 1 ^ b_is_hint;
//        uint a_change = a_spotted & a_bin_val;
//        uint a_change = b_spotted & b_bin_val;
//        a_mask |= (a_change & ((a_not_hint) << row)  | ((a_not_hint-1) & a_spot[a_val]));
//        b_mask |= (b_change & ((!b_not_hint) << row)  | ((b_not_hint-1) & b_spot[b_val]));
//        a_spotted |= ((a_change-1) & a_bin_val);
//        b_spotted |= ((b_change-1) & b_bin_val);
//        a_spot[a_val] = ((a_change-1) & (1<<row));
//        b_spot[b_val] = ((b_change-1) & (1<<row));
//      }
//      // change numbers if repeat rows coincide
//      uint match_mask = a_mask & b_mask;
//      uint row = 0;
//      if (match_mask) {
//        for (row = 0; row < 9; row++) {
//          uint row_offset = row*9;
//          // check validity of changing these numbers
//          uint a_num = grid[member_offset + row_offset + col];
//          uint b_num = grid[member_offset + row_offset + other];
//          uchar a_valid = able[a_num];
//          uchar b_valid = able[b_num];
//          uchar valid = a_valid & b_valid;
//          able[a_num] &= valid;
//          able[b_num] &= valid;
//          // if a is not in b and vice versa change
//          uint a_val = 1 << a_num;
//          uint b_val = 1 << b_num;
//          uint good_change = (((a_val & b_spotted)  |  (b_val & a_spotted)) -1) & valid;
//          grid[member_offset + row_offset + col] = (good_change & b_num) | (!good_change & a_num);
//          grid[member_offset + row_offset + other] = (good_change & a_num) | (!good_change & b_num);
//        }
//      }
//    }
//  }
//}


//kernel void lsga_block_kernel(global uchar* grid, global uchar* occupancy, global uchar* mistakes, const uint N){
//  // TODO: change into block version of LS
//  const uint member = get_global_id(0);
//  const uint member_offset = member*81;
//  if(member < N){
//    for (uint col=0; col < 9; col++){
//      uchar is_illegal = mistakes[member+col];
//      uint other = 0; // TODO: this line, make a way to get another column
//
//      uchar able[9] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
//      uint a_spot[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//      uint b_spot[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
//      uint a_mask = 0;
//      uint b_mask = 0;
//      uint a_spotted = 0;
//      uint b_spotted = 0;
//      // find repeated numbers in both columns
//      for (uint row=0; row<9; row++){
//        uint a_i =  member_offset + (row*9 + col);
//        uint b_i =  member_offset + (row*9 + other);
//        uint a_val = grid[a_i] -1;
//        uint b_val = grid[b_i] -1;
//        uint a_bin_val = 1 << val;
//        uint b_bin_val = 1 << val;
//        uchar a_is_hint = occupancy[a_i];
//        uchar b_is_hint = occupancy[b_i];
//        uchar a_not_hint = 1 ^ a_is_hint;
//        uchar b_not_hint = 1 ^ b_is_hint;
//        uint a_change = a_spotted & a_bin_val;
//        uint a_change = b_spotted & b_bin_val;
//        a_mask |= (a_change & ((a_not_hint) << row)  | ((a_not_hint-1) & a_spot[a_val]));
//        b_mask |= (b_change & ((!b_not_hint) << row)  | ((b_not_hint-1) & b_spot[b_val]));
//        a_spotted |= ((a_change-1) & a_bin_val);
//        b_spotted |= ((b_change-1) & b_bin_val);
//        a_spot[a_val] = ((a_change-1) & (1<<row));
//        b_spot[b_val] = ((b_change-1) & (1<<row));
//      }
//      // change numbers if repeat rows coincide
//      uint match_mask = a_mask & b_mask;
//      uint row = 0;
//      if (match_mask) {
//        for (row = 0; row < 9; row++) {
//          uint row_offset = row*9;
//          // check validity of changing these numbers
//          uint a_num = grid[member_offset + row_offset + col];
//          uint b_num = grid[member_offset + row_offset + other];
//          uchar a_valid = able[a_num];
//          uchar b_valid = able[b_num];
//          uchar valid = a_valid & b_valid;
//          able[a_num] &= valid;
//          able[b_num] &= valid;
//          // if a is not in b and vice versa change
//          uint a_val = 1 << a_num;
//          uint b_val = 1 << b_num;
//          uint good_change = (((a_val & b_spotted)  |  (b_val & a_spotted)) -1) & valid;
//          grid[member_offset + row_offset + col] = (good_change & b_num) | (!good_change & a_num);
//          grid[member_offset + row_offset + other] = (good_change & a_num) | (!good_change & b_num);
//        }
//      }
//    }
//  }
//}

  );
} // ############################################################### end of OpenCL C code #####################################################################