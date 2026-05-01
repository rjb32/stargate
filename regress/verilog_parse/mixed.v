// Hierarchical refs, attributes, compiler directives, escaped id,
// parameter override, named port connections.
`timescale 1ns/1ps
`define WIDTH 8

module mixed (
    input  wire             clk,
    input  wire [`WIDTH-1:0] din,
    output wire [`WIDTH-1:0] dout
);

    (* keep = "true" *) reg [`WIDTH-1:0] r;

    wire \escaped$id ;

    always @(posedge clk) begin : main_block
        r <= din;
    end

    assign dout = r;

    sub #(.W(`WIDTH)) u_sub (
        .clk(clk),
        .x(r),
        .y()
    );

endmodule

module sub #(
    parameter W = 4
) (
    input  wire         clk,
    input  wire [W-1:0] x,
    output wire [W-1:0] y
);
    assign y = x ^ {W{1'b1}};
endmodule
