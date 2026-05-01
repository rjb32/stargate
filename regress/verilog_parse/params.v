// parameter / localparam / defparam.
module params #(
    parameter WIDTH = 8,
    parameter DEPTH = 16
) (
    input  wire [WIDTH-1:0] din,
    output wire [WIDTH-1:0] dout
);

    localparam ZERO  = {WIDTH{1'b0}};
    localparam ONES  = {WIDTH{1'b1}};
    localparam HALF  = WIDTH / 2;

    parameter signed [WIDTH-1:0] BIAS = -1;

    assign dout = (din == ZERO) ? ONES : (din + BIAS);

endmodule
