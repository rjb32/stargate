// Mini68k Package - Common definitions

// ALU Operations
`define ALU_ADD   4'b0000
`define ALU_SUB   4'b0001
`define ALU_AND   4'b0010
`define ALU_OR    4'b0011
`define ALU_XOR   4'b0100
`define ALU_NOT   4'b0101
`define ALU_LSL   4'b0110
`define ALU_LSR   4'b0111
`define ALU_ASR   4'b1000
`define ALU_ROL   4'b1001
`define ALU_ROR   4'b1010

// Condition codes
`define CC_C  0  // Carry
`define CC_V  1  // Overflow
`define CC_Z  2  // Zero
`define CC_N  3  // Negative
`define CC_X  4  // Extend

// Function codes
`define FC_USER_DATA    3'b001
`define FC_USER_PROG    3'b010
`define FC_SUPER_DATA   3'b101
`define FC_SUPER_PROG   3'b110
`define FC_INT_ACK      3'b111
