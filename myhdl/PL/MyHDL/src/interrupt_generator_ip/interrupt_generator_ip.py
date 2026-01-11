#!/usr/bin/python

from myhdl import (
    always_comb,
    block,
    instances,
    ResetSignal,
    Signal,
)

from PL.MyHDL.src.interfaces.axi_lite import AxiLite
from PL.MyHDL.src.interfaces.axi_local import AxiLocal
from PL.MyHDL.src.axi_support.axi_support import axi_connect
from PL.MyHDL.src.interrupt_gen.interrupt_gen import interrupt_gen

LOW, HIGH = bool(0), bool(1)

PL_ADDR_WIDTH = 12
PL_DATA_WIDTH = 32
PL_INTERRUPT_GENERATOR = 0

@block
def interrupt_generator_ip(
    clk,
    resetn,
    s00_axi,
    interrupt1_out,
    interrupt2_out,
):
    """
    Parameters:
    clk             System clock (50MHz)
    resetn          System reset
    s00_axi         AXI4 slave interface
    interrupt1_out  Interrupt 1 output
    interrupt2_out  Interrupt 2 output
    """
    # Wrapped interface signal definitions
    _s00_axi = AxiLite(PL_ADDR_WIDTH, PL_DATA_WIDTH)

    @always_comb
    def wrap_interfaces():
        _s00_axi.awaddr.next = s00_axi.awaddr
        _s00_axi.awprot.next = s00_axi.awprot
        _s00_axi.awvalid.next = s00_axi.awvalid
        s00_axi.awready.next = _s00_axi.awready
        _s00_axi.wdata.next = s00_axi.wdata
        _s00_axi.wstrb.next = s00_axi.wstrb
        _s00_axi.wvalid.next = s00_axi.wvalid
        s00_axi.wready.next = _s00_axi.wready
        s00_axi.bresp.next = _s00_axi.bresp
        s00_axi.bvalid.next = _s00_axi.bvalid
        _s00_axi.bready.next = s00_axi.bready
        _s00_axi.araddr.next = s00_axi.araddr
        _s00_axi.arprot.next = s00_axi.arprot
        _s00_axi.arvalid.next = s00_axi.arvalid
        s00_axi.arready.next = _s00_axi.arready
        s00_axi.rdata.next = _s00_axi.rdata
        s00_axi.rresp.next = _s00_axi.rresp
        s00_axi.rvalid.next = _s00_axi.rvalid
        _s00_axi.rready.next = s00_axi.rready
        return

    # Define AxiLocal daisy-chain
    axi_local1 = AxiLocal(PL_ADDR_WIDTH, PL_DATA_WIDTH)

    axi_connect_inst = axi_connect(clk, resetn, _s00_axi, axi_local1)

    interrupt_gen_inst = interrupt_gen(
        clk=clk,
        resetn=resetn,
        axi_s=axi_local1,
        axi_m=None,
        interrupt1_out=interrupt1_out,
        interrupt2_out=interrupt2_out,
        map_base=PL_INTERRUPT_GENERATOR,
    )

    return instances()

if __name__ == "__main__":
    # Set parameters to defaults

    # Define signals for block ports
    clk = Signal(LOW)
    resetn = ResetSignal(0, active=0, isasync=True)
    s00_axi = AxiLite(PL_ADDR_WIDTH, PL_DATA_WIDTH)
    interrupt1_out = Signal(LOW)
    interrupt2_out = Signal(LOW)

    interrupt_generator_ip(
        clk=clk,
        resetn=resetn,
        s00_axi=s00_axi,
        interrupt1_out=interrupt1_out,
        interrupt2_out=interrupt2_out,
    ).convert(hdl="Verilog", testbench=False, timescale="1ns/1ps")
