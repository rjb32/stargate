// Mini68k Exception Handler

module mini68k_exception (
    input  wire        clk,
    input  wire        rst_n,

    // Exception inputs
    input  wire [2:0]  ipl_n,
    input  wire        bus_error,
    input  wire        address_error,
    input  wire        illegal_instr,
    input  wire        div_zero,
    input  wire        trap_req,
    input  wire [3:0]  trap_vector,

    // Status register
    input  wire [2:0]  int_mask,
    input  wire        supervisor,

    // Exception outputs
    output reg         exception_req,
    output reg  [7:0]  vector_num,
    output reg         enter_supervisor,

    // Acknowledge
    input  wire        exception_ack
);

    // Exception vectors
    localparam VEC_RESET      = 8'd0;
    localparam VEC_BUS_ERROR  = 8'd2;
    localparam VEC_ADDR_ERROR = 8'd3;
    localparam VEC_ILLEGAL    = 8'd4;
    localparam VEC_DIV_ZERO   = 8'd5;
    localparam VEC_TRAP_BASE  = 8'd32;
    localparam VEC_INT_BASE   = 8'd24;

    wire [2:0] int_level;
    wire       int_pending;

    assign int_level   = ~ipl_n;
    assign int_pending = (int_level > int_mask) && (int_level != 3'b000);

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            exception_req    <= 1'b0;
            vector_num       <= 8'd0;
            enter_supervisor <= 1'b0;
        end else if (exception_ack) begin
            exception_req <= 1'b0;
        end else if (!exception_req) begin
            // Priority order
            if (bus_error) begin
                exception_req    <= 1'b1;
                vector_num       <= VEC_BUS_ERROR;
                enter_supervisor <= 1'b1;
            end else if (address_error) begin
                exception_req    <= 1'b1;
                vector_num       <= VEC_ADDR_ERROR;
                enter_supervisor <= 1'b1;
            end else if (illegal_instr) begin
                exception_req    <= 1'b1;
                vector_num       <= VEC_ILLEGAL;
                enter_supervisor <= 1'b1;
            end else if (div_zero) begin
                exception_req    <= 1'b1;
                vector_num       <= VEC_DIV_ZERO;
                enter_supervisor <= 1'b1;
            end else if (trap_req) begin
                exception_req    <= 1'b1;
                vector_num       <= VEC_TRAP_BASE + {4'b0, trap_vector};
                enter_supervisor <= 1'b1;
            end else if (int_pending) begin
                exception_req    <= 1'b1;
                vector_num       <= VEC_INT_BASE + {5'b0, int_level};
                enter_supervisor <= 1'b1;
            end
        end
    end

endmodule
