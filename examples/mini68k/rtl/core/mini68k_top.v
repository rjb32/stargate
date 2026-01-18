// Mini68k Top-Level Module
// A simplified 68000-inspired processor core

module mini68k_top (
    input  wire        clk,
    input  wire        rst_n,

    // Memory interface
    output wire [23:0] addr,
    inout  wire [15:0] data,
    output wire        as_n,
    output wire        rw,
    output wire        uds_n,
    output wire        lds_n,
    input  wire        dtack_n,

    // Interrupts
    input  wire [2:0]  ipl_n,
    output wire [2:0]  fc,

    // Bus control
    input  wire        br_n,
    output wire        bg_n,
    input  wire        bgack_n
);

    // Internal buses
    wire [31:0] alu_result;
    wire [31:0] reg_data_out;
    wire [15:0] ir;
    wire [3:0]  alu_op;
    wire [2:0]  reg_sel;
    wire        reg_we;

    // Instantiate submodules
    mini68k_control u_control (
        .clk        (clk),
        .rst_n      (rst_n),
        .ir         (ir),
        .alu_op     (alu_op),
        .reg_sel    (reg_sel),
        .reg_we     (reg_we),
        .fc         (fc)
    );

    mini68k_registers u_registers (
        .clk        (clk),
        .rst_n      (rst_n),
        .reg_sel    (reg_sel),
        .reg_we     (reg_we),
        .data_in    (alu_result),
        .data_out   (reg_data_out)
    );

    mini68k_alu u_alu (
        .clk        (clk),
        .op         (alu_op),
        .a          (reg_data_out),
        .b          (32'h0),
        .result     (alu_result)
    );

endmodule
