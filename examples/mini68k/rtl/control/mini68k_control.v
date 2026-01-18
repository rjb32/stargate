// Mini68k Control Unit

module mini68k_control (
    input  wire        clk,
    input  wire        rst_n,

    // Instruction input
    input  wire [15:0] ir,
    input  wire        ir_valid,
    output reg         ir_consume,

    // ALU control
    output reg  [3:0]  alu_op,
    output reg         alu_start,

    // Register control
    output reg  [2:0]  reg_sel,
    output reg         reg_we,
    output reg         is_addr_reg,

    // Memory control
    output reg         mem_read,
    output reg         mem_write,

    // Status
    output reg  [2:0]  fc
);

    // Execution states
    localparam FETCH    = 4'b0000;
    localparam DECODE   = 4'b0001;
    localparam EXECUTE  = 4'b0010;
    localparam MEMORY   = 4'b0011;
    localparam WRITEBACK = 4'b0100;

    reg [3:0] state;

    // Instruction fields
    wire [3:0] opcode;
    wire [2:0] reg_x;
    wire [2:0] reg_y;
    wire [2:0] mode;
    wire [5:0] ea_mode;

    assign opcode  = ir[15:12];
    assign reg_x   = ir[11:9];
    assign reg_y   = ir[2:0];
    assign mode    = ir[8:6];
    assign ea_mode = ir[5:0];

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state      <= FETCH;
            ir_consume <= 1'b0;
            alu_start  <= 1'b0;
            reg_we     <= 1'b0;
            mem_read   <= 1'b0;
            mem_write  <= 1'b0;
            fc         <= 3'b010;  // User program
        end else begin
            case (state)
                FETCH: begin
                    ir_consume <= 1'b0;
                    reg_we     <= 1'b0;
                    if (ir_valid) begin
                        state <= DECODE;
                    end
                end

                DECODE: begin
                    // Decode instruction and set up control signals
                    case (opcode)
                        4'b0000: begin  // ORI, ANDI, etc.
                            alu_op <= ir[11:8];
                            state <= EXECUTE;
                        end

                        4'b0100: begin  // Miscellaneous
                            state <= EXECUTE;
                        end

                        4'b1101: begin  // ADD
                            alu_op <= 4'b0000;
                            reg_sel <= reg_x;
                            state <= EXECUTE;
                        end

                        4'b1001: begin  // SUB
                            alu_op <= 4'b0001;
                            reg_sel <= reg_x;
                            state <= EXECUTE;
                        end

                        default: begin
                            state <= FETCH;
                            ir_consume <= 1'b1;
                        end
                    endcase
                end

                EXECUTE: begin
                    alu_start <= 1'b1;
                    state <= WRITEBACK;
                end

                WRITEBACK: begin
                    alu_start <= 1'b0;
                    reg_we    <= 1'b1;
                    state     <= FETCH;
                    ir_consume <= 1'b1;
                end

                default: state <= FETCH;
            endcase
        end
    end

endmodule
