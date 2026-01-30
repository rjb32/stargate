module top (
    input  wire clk,
    input  wire rst,
    output reg  led
);

always @(posedge clk) begin
    if (rst)
        led <= 1'b0;
    else
        led <= ~led;
end

endmodule
