#!/usr/bin/python
#
# FILE:
#   axi_lite.py
#
# DESCRIPTION: Defines signals for AXI interface
#

from myhdl import (Signal, modbv)
LOW, HIGH = bool(0), bool(1)


class AxiLite(object):

    def __init__(self, ADDR_WIDTH=12, DATA_WIDTH=32, REG_WIDTH=32):
        self.awaddr = Signal(modbv(0)[ADDR_WIDTH:])
        self.awprot = Signal(modbv(0)[3:])
        self.awvalid = Signal(LOW)
        self.awready = Signal(LOW)
        self.wdata = Signal(modbv(0)[DATA_WIDTH:])
        self.wstrb = Signal(modbv(0)[(DATA_WIDTH // 8):])
        self.wvalid = Signal(LOW)
        self.wready = Signal(LOW)
        self.bresp = Signal(modbv(0)[2:])
        self.bvalid = Signal(LOW)
        self.bready = Signal(LOW)
        self.araddr = Signal(modbv(0)[ADDR_WIDTH:])
        self.arprot = Signal(modbv(0)[3:])
        self.arvalid = Signal(LOW)
        self.arready = Signal(LOW)
        self.rdata = Signal(modbv(0)[DATA_WIDTH:])
        self.rresp = Signal(modbv(0)[2:])
        self.rvalid = Signal(LOW)
        self.rready = Signal(LOW)
