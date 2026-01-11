#!/usr/bin/python
#
# FILE:
#   axi_local.py
#
# DESCRIPTION: Defines interface for AXI bus
#
from myhdl import (modbv, Signal)

LOW, HIGH = bool(0), bool(1)


class AxiLocal(object):

    def __init__(self, ADDR_WIDTH=12, DATA_WIDTH=32):
        self.waddr = Signal(modbv(0)[ADDR_WIDTH:0])
        self.wdata = Signal(modbv(0)[DATA_WIDTH:0])
        self.wstrobe = Signal(modbv(0)[(DATA_WIDTH // 8):])
        self.wen = Signal(LOW)
        self.raddr = Signal(modbv(0)[ADDR_WIDTH:0])
        self.rdata = Signal(modbv(0)[DATA_WIDTH:0])
