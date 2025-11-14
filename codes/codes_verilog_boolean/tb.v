`timescale 1ns/1ps

module tb_main;

    // Parameters
    localparam CLK_PERIOD_NS = 2;       // 20MHz -> 50ns period
    localparam integer MUX_DIV = 33; // For ~166.7 ms multiplex

    // DUT I/O
    reg clk;
    reg b1, b2, b3, b4;
    wire [3:0] disp;
    wire [5:0] power;

    // Instantiate DUT
    main uut (
        .clk(clk),
        .b1(b1), .b2(b2), .b3(b3), .b4(b4),
        .power(power),
        .disp(disp)
    );

    // 20MHz clock
    initial clk = 0;
    always #(CLK_PERIOD_NS/2) clk = ~clk;

    // Internal (optional): expose pause, aclk, etc. for waveform viewing
    wire paused = uut.paused;
    // If your main exposes multiplexer index, internal clocks, etc., also wire those up for analysis

    // Stimulus sequence (no printing, just for VCD timing)
    initial begin
        b1 = 0; b2 = 0; b3 = 0; b4 = 0;

        // -------- Dump all relevant variables for GTKWave/ModelSim --------
        $dumpfile("main.vcd");
        $dumpvars(0, tb_main);      // dumps all signals below this instance

        // Wait for a little bit (~500ns initial settle)
        #(10*CLK_PERIOD_NS);

        // Normal operation, see several multiplex cycles
        repeat (100*MUX_DIV) @(posedge clk);  // Run for a while

        // Pause (simulate freeze)
        b1 = 1; #10; b1 = 0;  // May miss entirely if no clk edge between
        // Hold paused for several multiplex periods
        repeat (10*6) @(posedge clk);

        b2 = 1; #10; b2 = 0;  // May miss entirely if no clk edge between
        repeat (10*1000) @(posedge clk);

        b3 = 1; #10; b3 = 0;  // May miss entirely if no clk edge between
        repeat (10*1000) @(posedge clk);
        b4 = 1; #10; b4 = 0;  // May miss entirely if no clk edge between
        repeat (10*1000) @(posedge clk);
        // Resume
        b1 = 1; #10; b1 = 0;  // May miss entirely if no clk edge between
        repeat (10*1000) @(posedge clk);

        // Done
        $finish;
    end

endmodule
