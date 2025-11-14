// ---------------- CLOCK CORE ----------------
module clock(
    input  wire clk,            // fast system clock from top
    input  wire tick,           // 1-Hz tick pulse (single-cycle)
    input  wire pause_pulse,    // pulse on b1 falling edge (from main)
    input  wire shift_pulse,    // pulse on b2 falling edge (from main)
    input  wire inc_pulse,      // pulse on b3 falling edge (from main)
    input  wire dec_pulse,      // pulse on b4 falling edge (from main)
    output reg  [3:0] h2, h1, m2, m1, s2, s1,
    output reg  [2:0] sel,      // selected digit: 0->s1 ... 5->h2
    output reg        paused    // paused state
);

    // internal next-state temporaries are not strictly necessary since
    // we update everything in one sequential block, but keep clarity.
    // Use non-blocking assignments for sequential logic.

    initial begin
        h2 = 4'd0; h1 = 4'd0; m2 = 4'd0; m1 = 4'd0; s2 = 4'd0; s1 = 4'd0;
        sel = 3'd0;
        paused = 1'b0;
    end

    always @(posedge clk) begin
        // Handle control pulses first (they only act when paused for shift/inc/dec)
        if (pause_pulse) begin
            paused <= ~paused;
            // When toggling to unpause, optionally reset selection to s1 (keep previous behaviour)
            if (!paused) sel <= 3'd0;
        end

        if (paused && shift_pulse) begin
            // shift left (from s1 -> s2 -> m1 -> m2 -> h1 -> h2 -> s1)
            if (sel == 3'd5) sel <= 3'd0;
            else sel <= sel + 1;
        end

        // Increment / Decrement only valid while paused (user requested)
        if (paused && inc_pulse) begin
            case (sel)
                3'd0: begin // s1 (0-9)
                    if (s1 == 4'd9) s1 <= 4'd0; else s1 <= s1 + 1;
                end
                3'd1: begin // s2 (0-5)
                    if (s2 == 4'd5) s2 <= 4'd0; else s2 <= s2 + 1;
                end
                3'd2: begin // m1 (0-9)
                    if (m1 == 4'd9) m1 <= 4'd0; else m1 <= m1 + 1;
                end
                3'd3: begin // m2 (0-5)
                    if (m2 == 4'd5) m2 <= 4'd0; else m2 <= m2 + 1;
                end
                3'd4: begin // h1 (depends on h2)
                    if (h2 == 4'd2) begin
                        if (h1 == 4'd3) h1 <= 4'd0; else h1 <= h1 + 1;
                    end else begin
                        if (h1 == 4'd9) h1 <= 4'd0; else h1 <= h1 + 1;
                    end
                end
                3'd5: begin // h2 (0-2)
                    if (h2 == 4'd2) h2 <= 4'd0; else h2 <= h2 + 1;
                    // After changing h2, ensure h1 remains valid:
                    if ( (h2 == 4'd1) && (h1 > 4'd9) ) h1 <= 4'd9; // safe guard (not usually needed)
                    if ( (h2 == 4'd2) && (h1 > 4'd3) ) h1 <= 4'd3;
                end
            endcase
        end

        if (paused && dec_pulse) begin
            case (sel)
                3'd0: begin // s1
                    if (s1 == 4'd0) s1 <= 4'd9; else s1 <= s1 - 1;
                end
                3'd1: begin // s2
                    if (s2 == 4'd0) s2 <= 4'd5; else s2 <= s2 - 1;
                end
                3'd2: begin // m1
                    if (m1 == 4'd0) m1 <= 4'd9; else m1 <= m1 - 1;
                end
                3'd3: begin // m2
                    if (m2 == 4'd0) m2 <= 4'd5; else m2 <= m2 - 1;
                end
                3'd4: begin // h1
                    if (h2 == 4'd2) begin
                        if (h1 == 4'd0) h1 <= 4'd3; else h1 <= h1 - 1;
                    end else begin
                        if (h1 == 4'd0) h1 <= 4'd9; else h1 <= h1 - 1;
                    end
                end
                3'd5: begin // h2
                    if (h2 == 4'd0) h2 <= 4'd2; else h2 <= h2 - 1;
                    if ( (h2 == 4'd2) && (h1 > 4'd3) ) h1 <= 4'd3;
                end
            endcase
        end

        // Time tick: advance time only when not paused
        if (tick && !paused) begin
            // increment seconds ones
            if (s1 == 4'd9) begin
                s1 <= 4'd0;
                // increment seconds tens
                if (s2 == 4'd5) begin
                    s2 <= 4'd0;
                    // increment minutes ones
                    if (m1 == 4'd9) begin
                        m1 <= 4'd0;
                        // increment minutes tens
                        if (m2 == 4'd5) begin
                            m2 <= 4'd0;
                            // increment hours ones
                            // complex wrap for 24-hour:
                            if ( (h2 == 4'd2 && h1 == 4'd3) ) begin
                                // currently 23 -> roll to 00
                                h1 <= 4'd0;
                                h2 <= 4'd0;
                            end else begin
                                if (h1 == 4'd9) begin
                                    h1 <= 4'd0;
                                    h2 <= h2 + 1;
                                end else begin
                                    h1 <= h1 + 1;
                                end
                            end
                        end else begin
                            m2 <= m2 + 1;
                        end
                    end else begin
                        m1 <= m1 + 1;
                    end
                end else begin
                    s2 <= s2 + 1;
                end
            end else begin
                s1 <= s1 + 1;
            end
        end
    end

endmodule



// ---------------- MAIN MODULE ----------------
module main(
    input b1, b2, b3, b4,
    output reg [5:0] power,
    output reg [3:0] disp
);
    // --- System clock input ---
    wire clk;
    qlal4s3b_cell_macro u_qlal4s3b_cell_macro (
        .Sys_Clk0 (clk)
    );

    // --- Parameters (adjust if your board freq differs) ---
    localparam integer SYS_FREQ = 24000000;     // 12 MHz (example)
    localparam integer ONEHZ_DIV = SYS_FREQ / 2; // produce ~1 Hz toggle (adjusted to previous code style)
    localparam integer MUX_DIV = SYS_FREQ / 1000;
    localparam integer BLINK_DIV = SYS_FREQ / 2; // 0.5 s toggle

    // --- Clock divider and pulse generation ---
    reg [25:0] clkdiv = 0;
    reg onehz_pulse = 0;

    always @(posedge clk) begin
        if (clkdiv >= ONEHZ_DIV - 1) begin
            clkdiv <= 0;
            onehz_pulse <= 1'b1;
        end else begin
            clkdiv <= clkdiv + 1;
            onehz_pulse <= 1'b0;
        end
    end

    // --- MUX switching (~1kHz) ---
    reg [15:0] muxdiv = 0;
    reg [2:0] mux_index = 0;
    always @(posedge clk) begin
        if (muxdiv >= (MUX_DIV / 6) - 1) begin
            muxdiv <= 0;
            if (mux_index == 5) mux_index <= 0; else mux_index <= mux_index + 1;
        end else begin
            muxdiv <= muxdiv + 1;
        end
    end

    // --- Blink timer (0.5s toggle) ---
    reg [25:0] blinkdiv = 0;
    reg blink_state = 1'b1;
    always @(posedge clk) begin
        if (blinkdiv >= BLINK_DIV - 1) begin
            blinkdiv <= 0;
            blink_state <= ~blink_state;
        end else begin
            blinkdiv <= blinkdiv + 1;
        end
    end

    // --- Button edge detection (falling edge pulses) ---
    reg b1_prev = 1'b1, b2_prev = 1'b1, b3_prev = 1'b1, b4_prev = 1'b1;
    wire b1_fall = b1_prev & ~b1;
    wire b2_fall = b2_prev & ~b2;
    wire b3_fall = b3_prev & ~b3;
    wire b4_fall = b4_prev & ~b4;

    always @(posedge clk) begin
        b1_prev <= b1;
        b2_prev <= b2;
        b3_prev <= b3;
        b4_prev <= b4;
    end

    // --- Clock core outputs ---
    wire [3:0] h2, h1, m2, m1, s2, s1;
    wire [2:0] sel;
    wire       paused;

    // instantiate clock core - it owns time registers and sel/paused
    clock mainclock(
        .clk(clk),
        .tick(onehz_pulse),
        .pause_pulse(b1_fall),
        .shift_pulse(b2_fall),
        .inc_pulse(b3_fall),
        .dec_pulse(b4_fall),
        .h2(h2), .h1(h1), .m2(m2), .m1(m1), .s2(s2), .s1(s1),
        .sel(sel),
        .paused(paused)
    );

    // --- Display multiplexer with true ON/OFF blink for selected digit ---
    always @(*) begin
        disp = 4'd0;
        power = 6'b000000;
        case (mux_index)
            0: begin
                disp = s1; power = 6'b000001;
                if (paused && sel == 3'd0 && ~blink_state) power = 6'b000000;
            end
            1: begin
                disp = s2; power = 6'b000010;
                if (paused && sel == 3'd1 && ~blink_state) power = 6'b000000;
            end
            2: begin
                disp = m1; power = 6'b000100;
                if (paused && sel == 3'd2 && ~blink_state) power = 6'b000000;
            end
            3: begin
                disp = m2; power = 6'b001000;
                if (paused && sel == 3'd3 && ~blink_state) power = 6'b000000;
            end
            4: begin
                disp = h1; power = 6'b010000;
                if (paused && sel == 3'd4 && ~blink_state) power = 6'b000000;
            end
            5: begin
                disp = h2; power = 6'b100000;
                if (paused && sel == 3'd5 && ~blink_state) power = 6'b000000;
            end
            default: begin
                disp = 4'd0; power = 6'b000000;
            end
        endcase
    end

endmodule

