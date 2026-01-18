// Mini68k ALU - Arithmetic Logic Unit

module mini68k_alu (
    input  wire        clk,
    input  wire [3:0]  op,
    input  wire [31:0] a,
    input  wire [31:0] b,
    output reg  [31:0] result,
    output reg  [4:0]  ccr
);

    `include "mini68k_pkg.v"

    wire [32:0] add_result;
    wire [32:0] sub_result;

    assign add_result = {1'b0, a} + {1'b0, b};
    assign sub_result = {1'b0, a} - {1'b0, b};

    always @(posedge clk) begin
        case (op)
            `ALU_ADD: begin
                result <= add_result[31:0];
                ccr[`CC_C] <= add_result[32];
            end

            `ALU_SUB: begin
                result <= sub_result[31:0];
                ccr[`CC_C] <= sub_result[32];
            end

            `ALU_AND: begin
                result <= a & b;
            end

            `ALU_OR: begin
                result <= a | b;
            end

            `ALU_XOR: begin
                result <= a ^ b;
            end

            `ALU_NOT: begin
                result <= ~a;
            end

            `ALU_LSL: begin
                result <= a << b[4:0];
            end

            `ALU_LSR: begin
                result <= a >> b[4:0];
            end

            `ALU_ASR: begin
                result <= $signed(a) >>> b[4:0];
            end

            default: begin
                result <= a;
            end
        endcase

        // Update Z and N flags
        ccr[`CC_Z] <= (result == 32'h0);
        ccr[`CC_N] <= result[31];
    end

endmodule
