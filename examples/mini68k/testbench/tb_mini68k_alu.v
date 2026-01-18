// Mini68k Testbench - ALU

`timescale 1ns / 1ps

module tb_mini68k_alu;

    reg         clk;
    reg  [3:0]  op;
    reg  [31:0] a;
    reg  [31:0] b;
    wire [31:0] result;
    wire [4:0]  ccr;

    // Instantiate DUT
    mini68k_alu dut (
        .clk    (clk),
        .op     (op),
        .a      (a),
        .b      (b),
        .result (result),
        .ccr    (ccr)
    );

    // Clock generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    // Test sequence
    initial begin
        op = 0;
        a  = 0;
        b  = 0;

        #20;

        // Test ADD
        op = 4'b0000;
        a  = 32'h00000010;
        b  = 32'h00000020;
        #20;
        $display("ADD: %h + %h = %h", a, b, result);

        // Test SUB
        op = 4'b0001;
        a  = 32'h00000050;
        b  = 32'h00000020;
        #20;
        $display("SUB: %h - %h = %h", a, b, result);

        // Test AND
        op = 4'b0010;
        a  = 32'hFF00FF00;
        b  = 32'h0F0F0F0F;
        #20;
        $display("AND: %h & %h = %h", a, b, result);

        // Test OR
        op = 4'b0011;
        a  = 32'hFF00FF00;
        b  = 32'h00FF00FF;
        #20;
        $display("OR: %h | %h = %h", a, b, result);

        // Test XOR
        op = 4'b0100;
        a  = 32'hAAAAAAAA;
        b  = 32'h55555555;
        #20;
        $display("XOR: %h ^ %h = %h", a, b, result);

        // Test LSL
        op = 4'b0110;
        a  = 32'h00000001;
        b  = 32'h00000004;
        #20;
        $display("LSL: %h << %d = %h", a, b[4:0], result);

        // Test LSR
        op = 4'b0111;
        a  = 32'h80000000;
        b  = 32'h00000004;
        #20;
        $display("LSR: %h >> %d = %h", a, b[4:0], result);

        #100;
        $display("ALU tests complete");
        $finish;
    end

    // Waveform dump
    initial begin
        $dumpfile("tb_mini68k_alu.vcd");
        $dumpvars(0, tb_mini68k_alu);
    end

endmodule
