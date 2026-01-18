// Mini68k Program Counter

module mini68k_pc (
    input  wire        clk,
    input  wire        rst_n,

    input  wire [31:0] pc_in,
    input  wire        pc_load,
    input  wire        pc_inc,
    input  wire [1:0]  pc_inc_size,  // 00=2, 01=4, 10=6

    output reg  [31:0] pc_out
);

    wire [31:0] inc_value;

    assign inc_value = (pc_inc_size == 2'b00) ? 32'd2 :
                       (pc_inc_size == 2'b01) ? 32'd4 : 32'd6;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            pc_out <= 32'h0;
        end else if (pc_load) begin
            pc_out <= pc_in;
        end else if (pc_inc) begin
            pc_out <= pc_out + inc_value;
        end
    end

endmodule
