// Mini68k Instruction Prefetch Queue

module mini68k_prefetch (
    input  wire        clk,
    input  wire        rst_n,

    // PC interface
    input  wire [31:0] pc,
    input  wire        flush,

    // Bus interface
    output reg  [23:0] fetch_addr,
    output reg         fetch_req,
    input  wire [15:0] fetch_data,
    input  wire        fetch_done,

    // Decoder interface
    output wire [15:0] ir,
    output wire [15:0] ext1,
    output wire [15:0] ext2,
    output wire        ir_valid,
    input  wire        ir_consume
);

    reg [15:0] queue [0:3];
    reg [2:0]  queue_head;
    reg [2:0]  queue_tail;
    reg [2:0]  queue_count;

    wire queue_full;
    wire queue_empty;

    assign queue_full  = (queue_count >= 3'd4);
    assign queue_empty = (queue_count == 3'd0);

    assign ir       = queue[queue_head[1:0]];
    assign ext1     = queue[(queue_head + 1) & 2'b11];
    assign ext2     = queue[(queue_head + 2) & 2'b11];
    assign ir_valid = (queue_count >= 3'd1);

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n || flush) begin
            queue_head  <= 3'd0;
            queue_tail  <= 3'd0;
            queue_count <= 3'd0;
            fetch_req   <= 1'b0;
            fetch_addr  <= pc[23:0];
        end else begin
            // Handle consumption
            if (ir_consume && !queue_empty) begin
                queue_head  <= queue_head + 1;
                queue_count <= queue_count - 1;
            end

            // Handle fetch completion
            if (fetch_done) begin
                queue[queue_tail[1:0]] <= fetch_data;
                queue_tail  <= queue_tail + 1;
                queue_count <= queue_count + 1;
                fetch_addr  <= fetch_addr + 2;
                fetch_req   <= 1'b0;
            end

            // Request next fetch if queue not full
            if (!queue_full && !fetch_req) begin
                fetch_req <= 1'b1;
            end
        end
    end

endmodule
