# --- XDC for KR260 LED Demo ---

# LED0 (DS8) - Pin E8 (HP Bank, using compatible LVCMOS18 standard)
set_property PACKAGE_PIN E8 [get_ports {led_output_tri_o[0]}]
set_property IOSTANDARD LVCMOS18 [get_ports {led_output_tri_o[0]}]

# LED1 (DS7) - Pin F8 (HP Bank, using compatible LVCMOS18 standard)
set_property PACKAGE_PIN F8 [get_ports {led_output_tri_o[1]}]
set_property IOSTANDARD LVCMOS18 [get_ports {led_output_tri_o[1]}]