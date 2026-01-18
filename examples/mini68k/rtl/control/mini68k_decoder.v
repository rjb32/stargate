// Mini68k Instruction Decoder

module mini68k_decoder (
    input  wire [15:0] ir,

    // Decoded fields
    output wire [3:0]  opcode,
    output wire [2:0]  reg_x,
    output wire [2:0]  reg_y,
    output wire [2:0]  op_mode,
    output wire [2:0]  ea_mode,
    output wire [2:0]  ea_reg,
    output wire [7:0]  displacement,

    // Instruction type
    output wire        is_move,
    output wire        is_alu,
    output wire        is_branch,
    output wire        is_jump,
    output wire        is_immediate,

    // Size
    output wire [1:0]  op_size  // 00=byte, 01=word, 10=long
);

    assign opcode       = ir[15:12];
    assign reg_x        = ir[11:9];
    assign op_mode      = ir[8:6];
    assign ea_mode      = ir[5:3];
    assign ea_reg       = ir[2:0];
    assign reg_y        = ir[2:0];
    assign displacement = ir[7:0];

    // Instruction type decoding
    assign is_move = (opcode == 4'b0001) ||   // MOVE.B
                     (opcode == 4'b0011) ||   // MOVE.W
                     (opcode == 4'b0010);     // MOVE.L

    assign is_alu = (opcode == 4'b1101) ||    // ADD
                    (opcode == 4'b1001) ||    // SUB
                    (opcode == 4'b1100) ||    // AND
                    (opcode == 4'b1000);      // OR

    assign is_branch = (opcode == 4'b0110);   // Bcc

    assign is_jump = (opcode == 4'b0100) &&
                     (ir[11:6] == 6'b111010 || ir[11:6] == 6'b111011);  // JMP/JSR

    assign is_immediate = (opcode == 4'b0000);  // ORI, ANDI, etc.

    // Size decoding
    assign op_size = (opcode == 4'b0001) ? 2'b00 :  // MOVE.B
                     (opcode == 4'b0011) ? 2'b01 :  // MOVE.W
                     (opcode == 4'b0010) ? 2'b10 :  // MOVE.L
                     op_mode[1:0];

endmodule
