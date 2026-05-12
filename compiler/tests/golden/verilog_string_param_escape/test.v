// This Verilog was transpiled from JZ-HDL.
// jz-hdl version: Version 0.1.8 (928ef2b)
// Intended for use with yosys.

`default_nettype none

module vendor_ip (
    clk,
    q
);
    // Ports
    input clk;
    output q;

    // Signals

endmodule

module TopMod (
    clk,
    q
);
    // Ports
    input clk;
    output q;

    // Signals


    vendor_ip #(
        .DATA_FILE("rom\\shared\\payload.mem")
    ) inst_vendor (
        .clk(clk),
        .q(q)
    );
endmodule
