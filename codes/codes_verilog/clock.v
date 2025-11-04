// ---------------- CLOCK CORE ----------------
module clock(
    input clk,
    output reg [3:0] h2, h1, m2, m1, s2, s1
);
    reg temp;
    reg [3:0] e = 4'd0;
    reg [3:0] f = 4'd0;
    reg [3:0] d = 4'd0;
    reg [3:0] c = 4'd0;
    reg [3:0] a = 4'd0;
    reg [3:0] b = 4'd0;

    always @(posedge clk) begin
        // --- Seconds ones ---
        s1 = {(~f[0]&f[3])|(f[0]&f[1]&f[2]),
              (~f[1]&f[2])|(~f[0]&f[2])|(f[0]&f[1]&~f[2]),
              (f[0]&~f[1]&~f[3])|(~f[0]&f[1]),
              (~f[0])};
        temp = f[0] & ~f[1] & ~f[2] & f[3];

        // --- Seconds tens ---
        s2 = {1'd0, (~e[0]&e[2])|(e[1]&e[0]),
              (~e[2]&~e[1]&e[0])|(~e[0]&e[1]),
              (~e[0])};
        s2 = ({4{temp}} & s2) | ({4{~temp}} & e);
        temp = temp & e[0] & ~e[1] & e[2] & ~e[3];

        // --- Minutes ones ---
        m1 = {(~d[0]&d[3])|(d[0]&d[1]&d[2]),
              (~d[1]&d[2])|(~d[0]&d[2])|(d[0]&d[1]&~d[2]),
              (d[0]&~d[1]&~d[3])|(~d[0]&d[1]),
              (~d[0])};
        m1 = ({4{temp}} & m1) | ({4{~temp}} & d);
        temp = temp & d[0] & ~d[1] & ~d[2] & d[3];

        // --- Minutes tens ---
        m2 = {1'd0, (~c[0]&c[2])|(c[1]&c[0]),
              (~c[2]&~c[1]&c[0])|(~c[0]&c[1]),
              (~c[0])};
        m2 = ({4{temp}} & m2) | ({4{~temp}} & c);
        temp = temp & c[0] & ~c[1] & c[2] & ~c[3];

        // --- Hours ones ---
        h1 = {(~b[0]&b[3])|(b[0]&b[1]&b[2]),
              (~b[1]&b[2])|(~b[0]&b[2])|(b[0]&b[1]&~b[2]),
              (b[0]&~b[1]&~b[3])|(~b[0]&b[1]),
              (~b[0])};
        h1 = ({4{temp}} & h1) | ({4{~temp}} & b);
        temp = temp & b[0] & ~b[1] & ~b[2] & b[3];

        // --- Hours tens ---
        h2 = {(~a[0]&a[3])|(a[0]&a[1]&a[2]),
              (~a[1]&a[2])|(~a[0]&a[2])|(a[0]&a[1]&~a[2]),
              (a[0]&~a[1]&~a[3])|(~a[0]&a[1]),
              (~a[0])};
        h2 = ({4{temp}} & h2) | ({4{~temp}} & a);
        temp = ~s1[0] & ~s1[1] & ~s1[2] & ~s1[3] &
                ~s2[0] & ~s2[1] & ~s2[3] & ~s2[2] &
                ~a[0] & a[1] & ~a[2] & ~a[3] &
                 b[0] & b[1] & ~b[2] & ~b[3] &
                 c[0] & ~c[1] & c[2] & ~c[3] &
                 d[0] & ~d[1] & ~d[2] & d[3];

        s1 = {4{~temp}} & s1;
        s2 = {4{~temp}} & s2;
        m1 = {4{~temp}} & m1;
        m2 = {4{~temp}} & m2;
        h1 = {4{~temp}} & h1;
        h2 = {4{~temp}} & h2;

        f = s1;
        e = s2;
        d = m1;
        c = m2;
        b = h1;
        a = h2;
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

    // --- Clock core outputs ---
    wire [3:0] h2, h1, m2, m1, s2, s1;
    clock mainclock(.clk(aclk),
                    .h2(h2), .h1(h1),
                    .m2(m2), .m1(m1),
                    .s2(s2), .s1(s1));

    // --- Parameters ---
    localparam integer SYS_FREQ = 12000000;     // 12 MHz
    localparam integer ONEHZ_DIV = SYS_FREQ / 2; // 1 Hz toggle
    localparam integer MUX_DIV = SYS_FREQ / 1000;
    localparam integer BLINK_DIV = SYS_FREQ / 2; // 0.5 s blink toggle

    // --- Internal registers ---
    reg [25:0] clkdiv = 0;
    reg [15:0] muxdiv = 0;
    reg [25:0] blinkdiv = 0;
    reg aclk = 0;
    reg blink_state = 1'b1;

    // --- MUX index ---
    reg [2:0] mux_index = 0;

    // --- Pause and buttons ---
    reg paused = 0;
    reg [2:0] selectedDigit = 3'd0;

    reg b1_prev = 1, b2_prev = 1;
    wire b1_fall = b1_prev & ~b1;
    wire b2_fall = b2_prev & ~b2;

    // --- Clock divider for 1 Hz time increment ---
    always @(posedge clk) begin
        clkdiv <= clkdiv + 1;
        if (clkdiv >= ONEHZ_DIV - 1) begin
            clkdiv <= 0;
            aclk <= ~aclk;
        end
    end

    // --- MUX switching (~1kHz) ---
    always @(posedge clk) begin
        muxdiv <= muxdiv + 1;
        if (muxdiv >= MUX_DIV / 6) begin
            muxdiv <= 0;
            mux_index <= (mux_index == 5) ? 0 : mux_index + 1;
        end
    end

    // --- Blink timer (0.5s toggle) ---
    always @(posedge clk) begin
        blinkdiv <= blinkdiv + 1;
        if (blinkdiv >= BLINK_DIV - 1) begin
            blinkdiv <= 0;
            blink_state <= ~blink_state;
        end
    end

    // --- Button sampling and logic ---
    always @(posedge clk) begin
        b1_prev <= b1;
        b2_prev <= b2;

        // Pause toggle
        if (b1_fall) begin
            paused <= ~paused;
            if (~paused)
                selectedDigit <= 3'd0;  // reset to first digit when entering pause
        end

        // Next digit select
        if (paused && b2_fall) begin
            if (selectedDigit == 3'd5)
                selectedDigit <= 3'd0;
            else
                selectedDigit <= selectedDigit + 1;
        end
    end

    // --- Display multiplexer with blink ---
    always @(*) begin
        case (mux_index)
            0: begin
                disp = s1; power = 6'b000001;
                if (paused && selectedDigit == 3'd0 && ~blink_state) disp = 4'd0;
            end
            1: begin
                disp = s2; power = 6'b000010;
                if (paused && selectedDigit == 3'd1 && ~blink_state) disp = 4'd0;
            end
            2: begin
                disp = m1; power = 6'b000100;
                if (paused && selectedDigit == 3'd2 && ~blink_state) disp = 4'd0;
            end
            3: begin
                disp = m2; power = 6'b001000;
                if (paused && selectedDigit == 3'd3 && ~blink_state) disp = 4'd0;
            end
            4: begin
                disp = h1; power = 6'b010000;
                if (paused && selectedDigit == 3'd4 && ~blink_state) disp = 4'd0;
            end
            5: begin
                disp = h2; power = 6'b100000;
                if (paused && selectedDigit == 3'd5 && ~blink_state) disp = 4'd0;
            end
            default: begin disp = 4'd0; power = 6'b000000; end
        endcase
    end
endmodule

