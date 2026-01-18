// Mini68k Bus Controller

module mini68k_bus_ctrl (
    input  wire        clk,
    input  wire        rst_n,

    // Internal interface
    input  wire [23:0] addr_in,
    input  wire [15:0] data_in,
    output reg  [15:0] data_out,
    input  wire        read_req,
    input  wire        write_req,
    input  wire        byte_sel,     // 0=word, 1=byte
    input  wire        byte_high,    // For byte access: 0=low, 1=high
    output reg         bus_busy,
    output reg         bus_done,

    // External bus interface
    output reg  [23:0] addr,
    inout  wire [15:0] data,
    output reg         as_n,
    output reg         rw,
    output reg         uds_n,
    output reg         lds_n,
    input  wire        dtack_n
);

    // Bus states
    localparam IDLE   = 3'b000;
    localparam ADDR   = 3'b001;
    localparam WAIT   = 3'b010;
    localparam DONE   = 3'b011;

    reg [2:0] state;
    reg [15:0] data_out_reg;
    reg data_oe;

    assign data = data_oe ? data_out_reg : 16'hzzzz;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            state <= IDLE;
            as_n <= 1'b1;
            rw <= 1'b1;
            uds_n <= 1'b1;
            lds_n <= 1'b1;
            bus_busy <= 1'b0;
            bus_done <= 1'b0;
            data_oe <= 1'b0;
        end else begin
            case (state)
                IDLE: begin
                    bus_done <= 1'b0;
                    if (read_req || write_req) begin
                        addr <= addr_in;
                        rw <= read_req;
                        bus_busy <= 1'b1;
                        state <= ADDR;

                        if (byte_sel) begin
                            uds_n <= ~byte_high;
                            lds_n <= byte_high;
                        end else begin
                            uds_n <= 1'b0;
                            lds_n <= 1'b0;
                        end

                        if (write_req) begin
                            data_out_reg <= data_in;
                            data_oe <= 1'b1;
                        end
                    end
                end

                ADDR: begin
                    as_n <= 1'b0;
                    state <= WAIT;
                end

                WAIT: begin
                    if (!dtack_n) begin
                        if (rw) begin
                            data_out <= data;
                        end
                        state <= DONE;
                    end
                end

                DONE: begin
                    as_n <= 1'b1;
                    uds_n <= 1'b1;
                    lds_n <= 1'b1;
                    data_oe <= 1'b0;
                    bus_busy <= 1'b0;
                    bus_done <= 1'b1;
                    state <= IDLE;
                end
            endcase
        end
    end

endmodule
