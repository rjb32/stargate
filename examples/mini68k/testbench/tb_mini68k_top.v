// Mini68k Testbench - Top Level

`timescale 1ns / 1ps

module tb_mini68k_top;

    reg         clk;
    reg         rst_n;

    wire [23:0] addr;
    wire [15:0] data;
    wire        as_n;
    wire        rw;
    wire        uds_n;
    wire        lds_n;
    reg         dtack_n;

    reg  [2:0]  ipl_n;
    wire [2:0]  fc;

    reg         br_n;
    wire        bg_n;
    reg         bgack_n;

    // Memory simulation
    reg [15:0] memory [0:65535];

    // Bidirectional data bus
    reg [15:0] mem_data_out;
    reg        mem_driving;

    assign data = mem_driving ? mem_data_out : 16'hzzzz;

    // Instantiate DUT
    mini68k_top dut (
        .clk     (clk),
        .rst_n   (rst_n),
        .addr    (addr),
        .data    (data),
        .as_n    (as_n),
        .rw      (rw),
        .uds_n   (uds_n),
        .lds_n   (lds_n),
        .dtack_n (dtack_n),
        .ipl_n   (ipl_n),
        .fc      (fc),
        .br_n    (br_n),
        .bg_n    (bg_n),
        .bgack_n (bgack_n)
    );

    // Clock generation
    initial begin
        clk = 0;
        forever #5 clk = ~clk;  // 100MHz
    end

    // Memory access simulation
    always @(negedge as_n) begin
        #20;
        if (rw) begin
            // Read
            mem_data_out <= memory[addr[16:1]];
            mem_driving  <= 1'b1;
        end else begin
            // Write
            memory[addr[16:1]] <= data;
        end
        dtack_n <= 1'b0;
    end

    always @(posedge as_n) begin
        dtack_n     <= 1'b1;
        mem_driving <= 1'b0;
    end

    // Test sequence
    initial begin
        // Initialize
        rst_n    = 0;
        dtack_n  = 1;
        ipl_n    = 3'b111;
        br_n     = 1;
        bgack_n  = 1;
        mem_driving = 0;

        // Load test program
        memory[0] = 16'h203C;  // MOVE.L #$12345678, D0
        memory[1] = 16'h1234;
        memory[2] = 16'h5678;
        memory[3] = 16'hD040;  // ADD.W D0, D0
        memory[4] = 16'h4E72;  // STOP

        // Reset sequence
        #100;
        rst_n = 1;

        // Run simulation
        #10000;

        $display("Simulation complete");
        $finish;
    end

    // Waveform dump
    initial begin
        $dumpfile("tb_mini68k_top.vcd");
        $dumpvars(0, tb_mini68k_top);
    end

endmodule
