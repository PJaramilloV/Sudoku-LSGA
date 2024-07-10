#include "kernel.hpp" // note: unbalanced round brackets () are not allowed and string literals can't be arbitrarily long, so periodically interrupt with )+R(
#include <cstring>
string opencl_c_container() { return R( // ########################## begin of OpenCL C code ####################################################################
kernel void lsga_col_kernel(global uchar* grid, global uchar* occupancy, global uchar* mistakes){
    const uint member = get_global_id(0);
    const uint member_offset = member*81;
    for(uint col=0; col < 9; col++){
      uchar is_illegal = mistakes[member+col];
      uint other = somehow get another;

      uchar able[9] = {1,1,1, 1,1,1, 1,1,1};
      uint a_spot[9] = {0,0,0 ,0,0,0 ,0,0,0};
      uint b_spot[9] = {0,0,0 ,0,0,0 ,0,0,0};
      uint a_mask = 0;
      uint b_mask = 0;
      uint a_spotted = 0;
      uint b_spotted = 0;
      // find repeated numbers in both columns
      for(uint row=0; row<9; row++){
        uint a_i =  member_offset + (row*9 + col);
        uint b_i =  member_offset + (row*9 + other);
        uint a_val = grid[a_i] -1;
        uint b_val = grid[b_i] -1;
        uint a_bin_val = 1 << val;
        uint b_bin_val = 1 << val;
        uchar a_is_hint = occupancy[a_i];
        uchar b_is_hint = occupancy[b_i];
        uchar a_not_hint = 1 ^ a_is_hint;
        uchar b_not_hint = 1 ^ b_is_hint;
        uint a_change = a_spotted & a_bin_val;
        uint a_change = b_spotted & b_bin_val;
        a_mask |= (a_change & (((a_not_hint) << row)  | ((a_not_hint-1) & a_spot[a_val])  );
        b_mask |= (b_change & ((!b_not_hint) << row)  | ((b_not_hint-1) & b_spot[b_val])  );
        a_spotted |= ((a_change-1) & a_bin_val);
        b_spotted |= ((b_change-1) & b_bin_val);
        a_spot[a_val] = ((a_change-1) & (1<<row));
        b_spot[b_val] = ((b_change-1) & (1<<row));
      }
      // change numbers if repeat rows coincide
      uint match_mask = a_mask & b_mask;
      uint row = 0;
      if(match_mask) {
        for (row = 0; row < 9; row++) {
          uint row_offset = row*9;
          // check validity of changing these numbers
          uint a_num = grid[member_offset + row_offset + col];
          uint b_num = grid[member_offset + row_offset + other];
          uchar a_valid = able[a_num];
          uchar b_valid = able[b_num];
          uchar valid = a_valid & b_valid;
          able[a_num] &= valid;
          able[b_num] &= valid;
          // if a is not in b and vice versa change
          uint a_val = 1 << a_num;
          uint b_val = 1 << b_num;
          uint good_change = ( ((a_val & b_spotted)  |  (b_val & a_spotted)) -1) & valid;
          grid[member_offset + row_offset + col] = (good_change & b_num) | (!good_change & a_num);
          grid[member_offset + row_offset + other] = (good_change & a_num) | (!good_change & b_num);
        }
      }
    }
  }
  );} // ############################################################### end of OpenCL C code #####################################################################