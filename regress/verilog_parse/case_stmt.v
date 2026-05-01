// case / casex / casez statements.
module case_stmt (
    input  wire [2:0] sel,
    input  wire [7:0] a,
    input  wire [7:0] b,
    input  wire [7:0] c,
    input  wire [7:0] d,
    output reg  [7:0] y
);

    always @(*) begin
        case (sel)
            3'd0:    y = a;
            3'd1:    y = b;
            3'd2,
            3'd3:    y = c;
            default: y = d;
        endcase
    end

    reg [3:0] code;
    always @(*) begin
        casez (code)
            4'b1???: y = 8'hFF;
            4'b01??: y = 8'h7F;
            4'b001?: y = 8'h3F;
            4'b0001: y = 8'h1F;
            default: y = 8'h00;
        endcase
    end

endmodule
