// This Verilog was transpiled from JZ-HDL.
// jz-hdl version: jz-hdl 0.1 (prototype)
// Intended for use with yosys.

`default_nettype none

module vendor_mem_top (
    addr,
    mif_dout,
    coe_dout
);
    // Ports
    input [1:0] addr;
    output [7:0] mif_dout;
    output [7:0] coe_dout;

    // Signals

    // Memories
    (* ram_style = "distributed" *) reg [7:0] mif_mem[0:3];
    (* ram_style = "distributed" *) reg [7:0] coe_mem[0:3];

    initial begin
        $readmemh("jz_mem_init__vendor_mem_top__mif_mem.hex", mif_mem);
    end
    initial begin
        $readmemh("jz_mem_init__vendor_mem_top__coe_mem.hex", coe_mem);
    end

    assign mif_dout = mif_mem[addr];
    assign coe_dout = coe_mem[addr];

endmodule

module top (
    addr,
    mif_dout,
    coe_dout
);
    input [1:0] addr;
    output [7:0] mif_dout;
    output [7:0] coe_dout;

    // Top-level logical→physical pin mapping
    //   vendor_mem_top.addr[1] -> addr[1] (board 2)
    //   vendor_mem_top.addr[0] -> addr[0] (board 1)
    //   vendor_mem_top.mif_dout[7] -> mif_dout[7] (board 10)
    //   vendor_mem_top.mif_dout[6] -> mif_dout[6] (board 9)
    //   vendor_mem_top.mif_dout[5] -> mif_dout[5] (board 8)
    //   vendor_mem_top.mif_dout[4] -> mif_dout[4] (board 7)
    //   vendor_mem_top.mif_dout[3] -> mif_dout[3] (board 6)
    //   vendor_mem_top.mif_dout[2] -> mif_dout[2] (board 5)
    //   vendor_mem_top.mif_dout[1] -> mif_dout[1] (board 4)
    //   vendor_mem_top.mif_dout[0] -> mif_dout[0] (board 3)
    //   vendor_mem_top.coe_dout[7] -> coe_dout[7] (board 18)
    //   vendor_mem_top.coe_dout[6] -> coe_dout[6] (board 17)
    //   vendor_mem_top.coe_dout[5] -> coe_dout[5] (board 16)
    //   vendor_mem_top.coe_dout[4] -> coe_dout[4] (board 15)
    //   vendor_mem_top.coe_dout[3] -> coe_dout[3] (board 14)
    //   vendor_mem_top.coe_dout[2] -> coe_dout[2] (board 13)
    //   vendor_mem_top.coe_dout[1] -> coe_dout[1] (board 12)
    //   vendor_mem_top.coe_dout[0] -> coe_dout[0] (board 11)



    vendor_mem_top u_top (
        .addr({addr[1], addr[0]}),
        .mif_dout({mif_dout[7], mif_dout[6], mif_dout[5], mif_dout[4], mif_dout[3], mif_dout[2], mif_dout[1], mif_dout[0]}),
        .coe_dout({coe_dout[7], coe_dout[6], coe_dout[5], coe_dout[4], coe_dout[3], coe_dout[2], coe_dout[1], coe_dout[0]})
    );
endmodule
