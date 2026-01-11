#!/usr/bin/python
#
# FILE:
#   interrupt_gen.py
#
#

# * This example block has two interrupt outputs. Interrupts can be time-based
#    or be triggered by writing to a register. 
# * Each interrupt is associated with a period counter, driven by the master clock.
#    If the value of this register is zero, the counter is reset and no time-based 
#    interrupts occur on that output.
# * A write-only trigger register may be used to force an interrupt by writing a 1
#    to the appropriate bit.
# * We have an interrupt enable register (IER) and an interrupt status register (ISR).
# * When each period counter expires, the corresponding bit in the ISR is set.
# * An interrupt output goes high if the associated bit in the ISR is high
#    and the associated bit in the IER is also high.
# * A bit in the ISR is cleared by writing a 1 to that bit position.



from myhdl import (
    always_comb,
    always_seq,
    block,
    instances,
    intbv,
    modbv,
    ResetSignal,
    Signal,
)
from PL.MyHDL.src.interfaces.axi_local import AxiLocal

PL_ADDR_WIDTH = 12
PL_DATA_WIDTH = 32
PL_REG_WIDTH = 32

# Register indices
INTERRUPT_GEN_PERIOD1 = 0
INTERRUPT_GEN_PERIOD2 = 1
INTERRUPT_GEN_ISR = 2
INTERRUPT_GEN_IER = 3
INTERRUPT_GEN_TRIGGER = 4
# Register bit fields
INTERRUPT_GEN_ISR_INTERRUPT1_B = 0
INTERRUPT_GEN_ISR_INTERRUPT1_W = 1
INTERRUPT_GEN_ISR_INTERRUPT2_B = 1
INTERRUPT_GEN_ISR_INTERRUPT2_W = 1
INTERRUPT_GEN_IER_INTERRUPT1_B = 0
INTERRUPT_GEN_IER_INTERRUPT1_W = 1
INTERRUPT_GEN_IER_INTERRUPT2_B = 1
INTERRUPT_GEN_IER_INTERRUPT2_W = 1
INTERRUPT_GEN_TRIGGER_INTERRUPT1_B = 0
INTERRUPT_GEN_TRIGGER_INTERRUPT1_W = 1
INTERRUPT_GEN_TRIGGER_INTERRUPT2_B = 1
INTERRUPT_GEN_TRIGGER_INTERRUPT2_W = 1

LOW, HIGH = bool(0), bool(1)


@block
def interrupt_gen(clk, resetn, axi_s, axi_m, interrupt1_out, interrupt2_out, map_base):
    """
    Parameters:
    clk         Clock
    resetn      Reset
    axi_s       Connection to upstream blocks
    axi_m       Connection to downstream blocks
    interrupt1_out  Goes high if interrupt 1 is asserted and bit in IER is set
    interrupt2_out  Goes high if interrupt 2 is asserted and bit in IER is set
    map_base    Base address

    Registers:
    INTERRUPT_GEN_PERIOD1   Divisor from clk to generate periodic interrupt 1, set to 0 for no periodic interrupts
    INTERRUPT_GEN_PERIOD2   Divisor from clk to generate periodic interrupt 2, set to 0 for no periodic interrupts
    INTERRUPT_GEN_ISR   Interrupt status register whose bits indicate interrupt assertion
    INTERRUPT_GEN_IER   Interrupt enable register whose bits allow an interrupt assertion to lead
                          to a high level on the associated interrupt output port
    INTERRUPT_GEN_TRIGGER   Write-only register allowing interrupt to be triggered

    Fields in INTERRUPT_GEN_ISR:
    INTERRUPT_GEN_ISR_INTERRUPT1    Indicates interrupt 1 asserted
    INTERRUPT_GEN_ISR_INTERRUPT2    Indicates interrupt 2 asserted

    Fields in INTERRUPT_GEN_IER:
    INTERRUPT_GEN_IER_INTERRUPT1    Enables interrupt1_out
    INTERRUPT_GEN_IER_INTERRUPT2    Enables interrupt2_out

    Fields in INTERRUPT_GEN_TRIGGER:
    INTERRUPT_GEN_TRIGGER_INTERRUPT1 Triggers interrupt 1
    INTERRUPT_GEN_TRIGGER_INTERRUPT2 Triggers interrupt 2
    """
    # Addresses of registers in block (in units of 4 bytes)
    interrupt_gen_period1_addr = map_base + INTERRUPT_GEN_PERIOD1
    interrupt_gen_period2_addr = map_base + INTERRUPT_GEN_PERIOD2
    interrupt_gen_isr_addr = map_base + INTERRUPT_GEN_ISR
    interrupt_gen_ier_addr = map_base + INTERRUPT_GEN_IER
    interrupt_gen_trigger_addr = map_base + INTERRUPT_GEN_TRIGGER
    rdata = Signal(intbv(0)[32:])
    period1 = Signal(intbv(0)[PL_REG_WIDTH:])
    period2 = Signal(intbv(0)[PL_REG_WIDTH:])
    isr = Signal(intbv(0)[8:])
    ier = Signal(intbv(0)[8:])
    trigger = Signal(intbv(0)[8:])

    # Unpacking ier register

    ier_interrupt1 = Signal(LOW)
    ier_interrupt2 = Signal(LOW)

    @always_comb
    def unpack_ier():
        ier_interrupt1.next = ier[INTERRUPT_GEN_IER_INTERRUPT1_B]
        ier_interrupt2.next = ier[INTERRUPT_GEN_IER_INTERRUPT2_B]

    # Unpacking trigger register

    trigger_interrupt1 = Signal(LOW)
    trigger_interrupt2 = Signal(LOW)

    @always_comb
    def unpack_trigger():
        trigger_interrupt1.next = trigger[INTERRUPT_GEN_TRIGGER_INTERRUPT1_B]
        trigger_interrupt2.next = trigger[INTERRUPT_GEN_TRIGGER_INTERRUPT2_B]

    # User defined signals and variables
    # Counters for periodic interrupts
    period1_counter = Signal(modbv(0)[PL_REG_WIDTH:])
    period2_counter = Signal(modbv(0)[PL_REG_WIDTH:])

    # AxiLocal pass through logic
    if axi_m is not None:

        @always_comb
        def axi_passthrough():
            axi_m.raddr.next = axi_s.raddr
            axi_m.waddr.next = axi_s.waddr
            axi_m.wdata.next = axi_s.wdata
            axi_m.wstrobe.next = axi_s.wstrobe
            axi_m.wen.next = axi_s.wen
            axi_s.rdata.next = axi_m.rdata | rdata

    else:

        @always_comb
        def axi_passthrough():
            axi_s.rdata.next = rdata

    @always_comb
    def register_read():
        # Read access of registers
        rdata.next = 0
        if axi_s.raddr == interrupt_gen_period1_addr:
            rdata.next = period1
        elif axi_s.raddr == interrupt_gen_period2_addr:
            rdata.next = period2
        elif axi_s.raddr == interrupt_gen_isr_addr:
            rdata.next = isr
        elif axi_s.raddr == interrupt_gen_ier_addr:
            rdata.next = ier

    period1_write_decode = Signal(LOW)

    @always_comb
    def period1_write_decoder():
        period1_write_decode.next = (
            axi_s.wen and axi_s.waddr == interrupt_gen_period1_addr
        )


    @always_seq(clk.posedge, reset=resetn)
    def period1_write():
        if period1_write_decode:
            for byte_index in range((PL_REG_WIDTH + 7) // 8):
                if axi_s.wstrobe[byte_index]:
                    for bit_index in range(8):
                        bit = 8 * byte_index + bit_index
                        if bit < len(period1):
                            period1.next[bit] = axi_s.wdata[bit]

    period2_write_decode = Signal(LOW)

    @always_comb
    def period2_write_decoder():
        period2_write_decode.next = (
            axi_s.wen and axi_s.waddr == interrupt_gen_period2_addr
        )

    @always_seq(clk.posedge, reset=resetn)
    def period2_write():
        if period2_write_decode:
            for byte_index in range((PL_REG_WIDTH + 7) // 8):
                if axi_s.wstrobe[byte_index]:
                    for bit_index in range(8):
                        bit = 8 * byte_index + bit_index
                        if bit < len(period2):
                            period2.next[bit] = axi_s.wdata[bit]

    isr_write_decode = Signal(LOW)

    @always_comb
    def isr_write_decoder():
        isr_write_decode.next = axi_s.wen and axi_s.waddr == interrupt_gen_isr_addr

    @always_seq(clk.posedge, reset=resetn)
    def isr_write():
        if isr_write_decode:
            for byte_index in range((8 + 7) // 8):
                if axi_s.wstrobe[byte_index]:
                    for bit_index in range(8):
                        bit = 8 * byte_index + bit_index
                        if bit < len(isr):
                            isr.next[bit] = isr[bit] & (~axi_s.wdata[bit])

        if (period1 != 0 and period1_counter >= period1 - 1) or trigger_interrupt1:
            isr.next[INTERRUPT_GEN_ISR_INTERRUPT1_B] = HIGH

        if (period2 != 0 and period2_counter >= period2 - 1) or trigger_interrupt2:
            isr.next[INTERRUPT_GEN_ISR_INTERRUPT2_B] = HIGH

    ier_write_decode = Signal(LOW)

    @always_comb
    def ier_write_decoder():
        ier_write_decode.next = axi_s.wen and axi_s.waddr == interrupt_gen_ier_addr

    @always_seq(clk.posedge, reset=resetn)
    def ier_write():
        if ier_write_decode:
            for byte_index in range((8 + 7) // 8):
                if axi_s.wstrobe[byte_index]:
                    for bit_index in range(8):
                        bit = 8 * byte_index + bit_index
                        if bit < len(ier):
                            ier.next[bit] = axi_s.wdata[bit]

    trigger_write_decode = Signal(LOW)

    @always_comb
    def trigger_write_decoder():
        trigger_write_decode.next = (
            axi_s.wen and axi_s.waddr == interrupt_gen_trigger_addr
        )

    @always_seq(clk.posedge, reset=resetn)
    def trigger_write():
        if trigger_write_decode:
            for byte_index in range((8 + 7) // 8):
                if axi_s.wstrobe[byte_index]:
                    for bit_index in range(8):
                        bit = 8 * byte_index + bit_index
                        if bit < len(trigger):
                            trigger.next[bit] = axi_s.wdata[bit]
        else:
            if trigger_interrupt1:
                trigger.next[INTERRUPT_GEN_TRIGGER_INTERRUPT1_B] = LOW
            if trigger_interrupt2:
                trigger.next[INTERRUPT_GEN_TRIGGER_INTERRUPT2_B] = LOW

    @always_seq(clk.posedge, reset=resetn)
    def handle_period1_counter():
        if period1 == 0:
            period1_counter.next = 0
        elif period1_counter >= period1 - 1:
            period1_counter.next = 0
        else:
            period1_counter.next = period1_counter + 1

    @always_seq(clk.posedge, reset=resetn)
    def handle_period2_counter():
        if period2 == 0:
            period2_counter.next = 0
        elif period2_counter >= period2 - 1:
            period2_counter.next = 0
        else:
            period2_counter.next = period2_counter + 1


    @always_comb
    def assigninterrupt_out():
        interrupt1_out.next = ier_interrupt1 and isr[INTERRUPT_GEN_ISR_INTERRUPT1_B]
        interrupt2_out.next = ier_interrupt2 and isr[INTERRUPT_GEN_ISR_INTERRUPT2_B]

    return instances()


if __name__ == "__main__":
    # Define signals for block ports
    clk = Signal(LOW)
    resetn = ResetSignal(0, active=0, isasync=True)
    axi_s = AxiLocal(PL_ADDR_WIDTH, PL_DATA_WIDTH)
    axi_m = AxiLocal(PL_ADDR_WIDTH, PL_DATA_WIDTH)
    interrupt1_out = Signal(LOW)
    interrupt2_out = Signal(LOW)
    map_base = 0

    interrupt_gen(clk=clk, resetn=resetn, axi_s=axi_s, axi_m=axi_m,
                  interrupt1_out=interrupt1_out,
                  interrupt2_out=interrupt2_out, map_base=map_base).convert(hdl='Verilog')
