#!/usr/bin/python
#
# FILE:
#   AXI_support.py
#
# DESCRIPTION:
#   Support block which connects programmable logic to an AXI Slave interface
#
#
from myhdl import (always_comb, always_seq, block,
                   instances, intbv, Signal)
LOW, HIGH = bool(0), bool(1)


@block
def axi_support(
    S_AXI_ACLK,     # Global Clock Signal
    S_AXI_ARESETN,  # Global Reset Signal. This Signal is Active LOW
    S_AXI_AWADDR,   # Write address (issued by master, accepted by Slave)
    # Write channel Protection type. This signal indicates the privilege and security
    # level of the transaction, and whether the transaction is a data access or an 
    # instruction access
    S_AXI_AWPROT,
    # Write address valid. This signal indicates that the master signaling
    # valid write address and control information.
    S_AXI_AWVALID,
    # Write address ready. This signal indicates that the slave is ready to
    # accept an address and associated control signals.
    axi_awready,
    S_AXI_WDATA,    # Write data (issued by master, accepted by Slave)
    # Write strobes. This signal indicates which byte lanes hold valid data.
    # There is one write strobe bit for each eight bits of the write data bus.
    S_AXI_WSTRB,
    # Write valid. This signal indicates that valid write data and strobes are
    # available.
    S_AXI_WVALID,
    # Write ready. This signal indicates that the slave can accept the write
    # data.
    axi_wready,
    # Write response. This signal indicates the status of the write
    # transaction.
    S_AXI_BRESP,
    # Write response valid. This signal indicates that the channel is
    # signaling a valid write response.
    S_AXI_BVALID,
    # Response ready. This signal indicates that the master can accept a write
    # response.
    S_AXI_BREADY,
    S_AXI_ARADDR,   # Read address (issued by master, accepted by Slave)
    # Protection type. This signal indicates the privilege and security level
    # of the transaction, and whether the transaction is a data access or an
    # instruction access.
    S_AXI_ARPROT,
    # Read address valid. This signal indicates that the channel is signaling
    # valid read address and control information.
    S_AXI_ARVALID,
    # Read address ready. This signal indicates that the slave is ready to
    # accept an address and associated control signals.
    S_AXI_ARREADY,
    S_AXI_RDATA,    # Read data (issued by slave)
    # Read response. This signal indicates the status of the read transfer.
    S_AXI_RRESP,
    # Read valid. This signal indicates that the channel is signaling the
    # required read data.
    S_AXI_RVALID,
    # Read ready. This signal indicates that the master can accept the read
    # data and response information.
    S_AXI_RREADY,
    axi_awaddr,     # Latched write address
    axi_araddr,     # Latched read address
    reg_data,       # Data to be written back to PS
    C_S_AXI_DATA_WIDTH,
    C_S_AXI_ADDR_WIDTH
):

    axi_bresp = Signal(intbv(0)[2:0])
    axi_bvalid = Signal(False)
    axi_arready = Signal(False)
    axi_rdata = Signal(intbv(0)[C_S_AXI_DATA_WIDTH:0])
    axi_rresp = Signal(intbv(0)[2:0])
    axi_rvalid = Signal(False)

    # Connect output lines to registers
    @always_comb
    def output_lines():
        S_AXI_BRESP.next = axi_bresp
        S_AXI_BVALID.next = axi_bvalid
        S_AXI_ARREADY.next = axi_arready
        S_AXI_RDATA.next = axi_rdata
        S_AXI_RRESP.next = axi_rresp
        S_AXI_RVALID.next = axi_rvalid

    # Implement axi_awready generation
    # axi_awready is asserted for one S_AXI_ACLK clock cycle when both
    # S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_awready is
    # de-asserted when reset is low.

    @always_seq(S_AXI_ACLK.posedge, reset=S_AXI_ARESETN)
    def axi_awready_generation():
        if (not axi_awready) and S_AXI_AWVALID and S_AXI_WVALID:
            # slave is ready to accept write address when there is a valid write address and write data
            # on the write address and data bus. This design expects no
            # outstanding transactions.
            axi_awready.next = True
        else:
            axi_awready.next = False

    # Implement axi_awaddr latching
    # This process is used to latch the address when both
    # S_AXI_AWVALID and S_AXI_WVALID are valid.
    @always_seq(S_AXI_ACLK.posedge, reset=S_AXI_ARESETN)
    def axi_awaddr_latching():
        if (not axi_awready) and S_AXI_AWVALID and S_AXI_WVALID:
            axi_awaddr.next = S_AXI_AWADDR

    # Implement axi_wready generation
    # axi_wready is asserted for one S_AXI_ACLK clock cycle when both
    # S_AXI_AWVALID and S_AXI_WVALID are asserted. axi_wready is
    # de-asserted when reset is low.
    @always_seq(S_AXI_ACLK.posedge, reset=S_AXI_ARESETN)
    def axi_wready_generation():
        if (not axi_awready) and S_AXI_AWVALID and S_AXI_WVALID:
            # slave is ready to accept write data when there is a valid
            # write address and write data on the write address and data
            # bus. This design expects no outstanding transactions.
            axi_wready.next = True
        else:
            axi_wready.next = False

    # Implement write response logic generation
    # The write response and response valid signals are asserted by the slave
    # when axi_wready, S_AXI_WVALID, axi_wready and S_AXI_WVALID are asserted.
    # This marks the acceptance of address and indicates the status of write
    # transaction.

    @always_seq(S_AXI_ACLK.posedge, reset=S_AXI_ARESETN)
    def write_response_logic_generation():
        if axi_awready and S_AXI_AWVALID and (not axi_bvalid) and axi_wready and S_AXI_WVALID:
            # indicates a valid write response is available
            axi_bvalid.next = True
            axi_bresp.next = 0  # 'OKAY' response
        else:
            if S_AXI_BREADY and axi_bvalid:
                # check if bready is asserted while bvalid is high)
                # (there is a possibility that bready is always asserted high)
                axi_bvalid.next = False

    # Implement axi_arready generation
    # axi_arready is asserted for one S_AXI_ACLK clock cycle when
    # S_AXI_ARVALID is asserted. axi_arready is
    # de-asserted when reset (active low) is asserted.
    # The read address is also latched when S_AXI_ARVALID is
    # asserted. axi_araddr is reset to zero on reset assertion.

    @always_seq(S_AXI_ACLK.posedge, reset=S_AXI_ARESETN)
    def axi_arready_generation():
        if (not axi_arready) and S_AXI_ARVALID:
            # indicates that the slave has accepted the valid read address
            axi_arready.next = True
            # Read address latching
            axi_araddr.next = S_AXI_ARADDR
        else:
            axi_arready.next = False

    # Implement axi_arvalid generation
    # axi_rvalid is asserted for one S_AXI_ACLK clock cycle when both
    # S_AXI_ARVALID and axi_arready are asserted. The slave registers
    # data are available on the axi_rdata bus at this instance. The
    # assertion of axi_rvalid marks the validity of read data on the
    # bus and axi_rresp indicates the status of read transaction.axi_rvalid
    # is deasserted on reset (active low). axi_rresp and axi_rdata are
    # cleared to zero on reset (active low).
    @always_seq(S_AXI_ACLK.posedge, reset=S_AXI_ARESETN)
    def axi_arvalid_generation():
        if axi_arready and S_AXI_ARVALID and (not axi_rvalid):
            # Valid read data is available at the read data bus
            axi_rvalid.next = True
            axi_rresp.next = 0  # 'OKAY' response
        elif axi_rvalid and S_AXI_RREADY:
            # Read data is accepted by the master
            axi_rvalid.next = False

    # Output register or memory read data
    @always_seq(S_AXI_ACLK.posedge, reset=S_AXI_ARESETN)
    def memory_read_data():
        # When there is a valid read address (axi_arvalid) with
        # acceptance of read address by the slave (axi_arready),
        # output the read data
        slv_reg_rden = axi_arready and S_AXI_ARVALID and not axi_rvalid
        if slv_reg_rden:
            axi_rdata.next = reg_data

    return instances()


@block
def axi_connect(clk, resetn, axi_lite, axi_local, ADDR_WIDTH=12, DATA_WIDTH=32):
    ADDR_LSB = (DATA_WIDTH // 32) + 1
    OPT_MEM_ADDR_BITS = ADDR_WIDTH - ADDR_LSB  # log2 of number of registers

    axi_awready = Signal(LOW)
    axi_wready = Signal(LOW)
    axi_awaddr = Signal(intbv(0)[ADDR_WIDTH:0])
    axi_araddr = Signal(intbv(0)[ADDR_WIDTH:0])
    reg_data = Signal(intbv(0)[DATA_WIDTH:0])

    supp = axi_support(clk, resetn, axi_lite.awaddr, axi_lite.awprot, axi_lite.awvalid, axi_awready,
                       axi_lite.wdata, axi_lite.wstrb, axi_lite.wvalid, axi_wready, axi_lite.bresp, axi_lite.bvalid,
                       axi_lite.bready, axi_lite.araddr, axi_lite.arprot, axi_lite.arvalid, axi_lite.arready, axi_lite.rdata,
                       axi_lite.rresp, axi_lite.rvalid, axi_lite.rready, axi_awaddr, axi_araddr, reg_data,
                       DATA_WIDTH, ADDR_WIDTH)

    @always_comb
    def axi_helpers():
        axi_lite.awready.next = axi_awready
        axi_lite.wready.next = axi_wready
        axi_local.wen.next = axi_wready and axi_lite.awvalid and axi_awready and axi_lite.wvalid
        axi_local.raddr.next = axi_araddr[
            ADDR_LSB + OPT_MEM_ADDR_BITS:ADDR_LSB]
        axi_local.waddr.next = axi_awaddr[
            ADDR_LSB + OPT_MEM_ADDR_BITS:ADDR_LSB]
        axi_local.wdata.next = axi_lite.wdata
        axi_local.wstrobe.next = axi_lite.wstrb
        reg_data.next = axi_local.rdata

    return instances()
