// Mini68k Barrel Shifter

module mini68k_barrel_shifter (
    input  wire [31:0] data_in,
    input  wire [4:0]  shift_amt,
    input  wire [1:0]  shift_type,  // 00=LSL, 01=LSR, 10=ASR, 11=ROL
    output reg  [31:0] data_out,
    output wire        carry_out
);

    reg [32:0] shift_result;

    always @(*) begin
        case (shift_type)
            2'b00: shift_result = {1'b0, data_in} << shift_amt;           // LSL
            2'b01: shift_result = {data_in, 1'b0} >> shift_amt;           // LSR
            2'b10: shift_result = {data_in[31], data_in, 1'b0} >>> shift_amt; // ASR
            2'b11: shift_result = {1'b0, data_in, data_in} >> (32 - shift_amt); // ROL
            default: shift_result = {1'b0, data_in};
        endcase
        data_out = shift_result[31:0];
    end

    assign carry_out = shift_result[32];

endmodule
