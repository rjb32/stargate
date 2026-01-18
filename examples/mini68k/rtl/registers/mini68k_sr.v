// Mini68k Status Register

module mini68k_sr (
    input  wire        clk,
    input  wire        rst_n,

    // CCR inputs from ALU
    input  wire [4:0]  ccr_in,
    input  wire        ccr_we,

    // Full SR access
    input  wire [15:0] sr_in,
    input  wire        sr_we,

    // Outputs
    output wire [15:0] sr_out,
    output wire [4:0]  ccr_out,
    output wire        supervisor,
    output wire [2:0]  int_mask
);

    reg [15:0] sr;

    // SR bit positions
    // [4:0]   - CCR (C, V, Z, N, X)
    // [7:5]   - Reserved
    // [10:8]  - Interrupt mask
    // [12:11] - Reserved
    // [13]    - Supervisor mode
    // [14]    - Reserved
    // [15]    - Trace mode

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            sr <= 16'h2700;  // Supervisor mode, interrupts masked
        end else if (sr_we) begin
            sr <= sr_in;
        end else if (ccr_we) begin
            sr[4:0] <= ccr_in;
        end
    end

    assign sr_out     = sr;
    assign ccr_out    = sr[4:0];
    assign supervisor = sr[13];
    assign int_mask   = sr[10:8];

endmodule
