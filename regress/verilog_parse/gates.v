// Basic gate primitives and continuous assignment.
module gates (
    input  wire        a,
    input  wire        b,
    output wire        y_and,
    output wire        y_or,
    output wire        y_xor,
    output wire        y_not,
    output wire [3:0]  bus
);

    wire t1, t2;

    and  g1 (t1, a, b);
    or   g2 (t2, a, b);
    xor  g3 (y_xor, a, b);
    not  g4 (y_not, a);

    assign y_and = t1;
    assign y_or  = t2;
    assign bus   = {a, b, a, b};

endmodule
