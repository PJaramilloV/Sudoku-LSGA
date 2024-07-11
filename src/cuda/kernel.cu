#include "kernel.cuh"

/*__global__ void vec_sum(int *a, int *b, int *c, int n){
  int idx = blockDim.x * blockIdx.x + threadIdx.x;
  if(idx < n) {
    c[idx] = a[idx] + b[idx];
  }
}*/

int somehow_get_another() {
  return 0;
}


__global__ void lsga_col_kernel(unsigned char* grid, unsigned char* occupancy, unsigned char* mistakes) {
    const unsigned int member = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int member_offset = member * 81;
    for(unsigned int col = 0; col < 9; col++) {
        unsigned char is_illegal = mistakes[member + col];
        unsigned int other = somehow_get_another();

        unsigned char able[9] = {1,1,1, 1,1,1, 1,1,1};
        unsigned int a_spot[9] = {0,0,0 ,0,0,0 ,0,0,0};
        unsigned int b_spot[9] = {0,0,0 ,0,0,0 ,0,0,0};
        unsigned int a_mask = 0;
        unsigned int b_mask = 0;
        unsigned int a_spotted = 0;
        unsigned int b_spotted = 0;

        // find repeated numbers in both columns
        for(unsigned int row = 0; row < 9; row++) {
            unsigned int a_i = member_offset + (row * 9 + col);
            unsigned int b_i = member_offset + (row * 9 + other);
            unsigned int a_val = grid[a_i] - 1;
            unsigned int b_val = grid[b_i] - 1;
            unsigned int a_bin_val = 1 << a_val;
            unsigned int b_bin_val = 1 << b_val;
            unsigned char a_is_hint = occupancy[a_i];
            unsigned char b_is_hint = occupancy[b_i];
            unsigned char a_not_hint = 1 ^ a_is_hint;
            unsigned char b_not_hint = 1 ^ b_is_hint;
            unsigned int a_change = a_spotted & a_bin_val;
            unsigned int b_change = b_spotted & b_bin_val;
            a_mask |= (a_change & (((a_not_hint) << row)  | ((a_not_hint - 1) & a_spot[a_val])));
            b_mask |= (b_change & (((b_not_hint) << row)  | ((b_not_hint - 1) & b_spot[b_val])));
            a_spotted |= ((a_change - 1) & a_bin_val);
            b_spotted |= ((b_change - 1) & b_bin_val);
            a_spot[a_val] = ((a_change - 1) & (1 << row));
            b_spot[b_val] = ((b_change - 1) & (1 << row));
        }

        // change numbers if repeat rows coincide
        unsigned int match_mask = a_mask & b_mask;
        unsigned int row = 0;
        if(match_mask) {
            for (row = 0; row < 9; row++) {
                unsigned int row_offset = row * 9;
                // check validity of changing these numbers
                unsigned int a_num = grid[member_offset + row_offset + col];
                unsigned int b_num = grid[member_offset + row_offset + other];
                unsigned char a_valid = able[a_num];
                unsigned char b_valid = able[b_num];
                unsigned char valid = a_valid & b_valid;
                able[a_num] &= valid;
                able[b_num] &= valid;
                // if a is not in b and vice versa change
                unsigned int a_val = 1 << a_num;
                unsigned int b_val = 1 << b_num;
                unsigned int good_change = (((a_val & b_spotted)  |  (b_val & a_spotted)) - 1) & valid;
                grid[member_offset + row_offset + col] = (good_change & b_num) | (!good_change & a_num);
                grid[member_offset + row_offset + other] = (good_change & a_num) | (!good_change & b_num);
            }
        }
    }
}

int main() {
    // Assuming N is the number of members and is defined
    int N = 1024; // Example value

    // Allocate host memory and initialize data here
    unsigned char *h_grid, *h_occupancy, *h_mistakes;

    // Allocate device memory
    unsigned char *d_grid, *d_occupancy, *d_mistakes;
    cudaMalloc(&d_grid, N * 81 * sizeof(unsigned char));
    cudaMalloc(&d_occupancy, N * 81 * sizeof(unsigned char));
    cudaMalloc(&d_mistakes, N * 9 * sizeof(unsigned char));

    // Copy data from host to device
    cudaMemcpy(d_grid, h_grid, N * 81 * sizeof(unsigned char), cudaMemcpyHostToDevice);
    cudaMemcpy(d_occupancy, h_occupancy, N * 81 * sizeof(unsigned char), cudaMemcpyHostToDevice);
    cudaMemcpy(d_mistakes, h_mistakes, N * 9 * sizeof(unsigned char), cudaMemcpyHostToDevice);

    // Define grid and block dimensions
    dim3 blockDim(256);
    dim3 gridDim((N + blockDim.x - 1) / blockDim.x);

    // Launch the kernel
    lsga_col_kernel<<<gridDim, blockDim>>>(d_grid, d_occupancy, d_mistakes);

    // Copy the results back to host
    cudaMemcpy(h_grid, d_grid, N * 81 * sizeof(unsigned char), cudaMemcpyDeviceToHost);

    // Free device memory
    cudaFree(d_grid);
    cudaFree(d_occupancy);
    cudaFree(d_mistakes);

    // Free host memory and perform other cleanup here

    return 0;
}