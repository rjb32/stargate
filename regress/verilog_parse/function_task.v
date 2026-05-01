// Functions, tasks, and procedural control flow.
module function_task (
    input  wire [7:0] x,
    output reg  [7:0] y
);

    function [7:0] saturate_add;
        input [7:0] a;
        input [7:0] b;
        reg [8:0] sum;
        begin
            sum = a + b;
            if (sum[8])
                saturate_add = 8'hFF;
            else
                saturate_add = sum[7:0];
        end
    endfunction

    task display_value;
        input [7:0] v;
        begin
            $display("value = %d", v);
        end
    endtask

    integer i;
    reg [7:0] acc;

    always @(*) begin
        acc = 8'd0;
        for (i = 0; i < 4; i = i + 1) begin
            acc = saturate_add(acc, x);
        end
        y = acc;
        display_value(y);
    end

endmodule
