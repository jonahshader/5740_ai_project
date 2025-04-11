# Minimal constraints file for KV260 UART design

# Map UART rx and tx to PMOD pins
# Using PMOD[0] for rx
set_property PACKAGE_PIN H12      [get_ports "i_rx_serial"]
set_property IOSTANDARD LVCMOS33  [get_ports "i_rx_serial"]

# Using PMOD[1] for tx
set_property PACKAGE_PIN E10      [get_ports "o_tx_serial"]
set_property IOSTANDARD LVCMOS33  [get_ports "o_tx_serial"]
