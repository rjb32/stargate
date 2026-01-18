// Mini68k Register File
// D0-D7: Data registers
// A0-A6: Address registers
// A7/USP/SSP: Stack pointers

module mini68k_registers (
    input  wire        clk,
    input  wire        rst_n,
    input  wire [2:0]  reg_sel,
    input  wire        reg_we,
    input  wire        is_addr_reg,
    input  wire [31:0] data_in,
    output wire [31:0] data_out
);

    // Data registers D0-D7
    reg [31:0] d_regs [0:7];

    // Address registers A0-A7
    reg [31:0] a_regs [0:7];

    integer i;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (i = 0; i < 8; i = i + 1) begin
                d_regs[i] <= 32'h0;
                a_regs[i] <= 32'h0;
            end
        end else if (reg_we) begin
            if (is_addr_reg)
                a_regs[reg_sel] <= data_in;
            else
                d_regs[reg_sel] <= data_in;
        end
    end

    assign data_out = is_addr_reg ? a_regs[reg_sel] : d_regs[reg_sel];

endmodule
