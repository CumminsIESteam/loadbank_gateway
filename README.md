# loadbank_gateway
An application built for Arduino with Etehrnet shield.   Accepts Modbus TCP register writes and translates them to commands for the Simplex loadbanks with AutomationDirect PLCs with ECOM Ethernet modules.

This application uses the MgsModbus library for Modbus TCP slave functionality, which uses a single block of memory for all modbus data (mbData[] array).  The Modbus commands are relayed to the loadbank PLC using commands of unknown protocol (the command messages were deciphered by monitoring traffic from the Simplex touchscreen).  The loadbank communications are over UDP.  Writes to multi-register values must be carried out in a single multiple-register write (Function Code 16).

The command can be written using either the float32 or uint16 formats - see the register definitions below:

40001:  float32 kW command lower 16bits - units: kW               \
40002:  float32 kW command upper 16bits                           \
40003:  float32 PF command lower 16bits - units: normalized 0-1   \
40004:  float32 PF command upper 16bits                           \
40201:  uint16  kW command - units: kW                            \
40202:  uint16  PF command - units: % (range 0-100)               
    
Author:  Mike Scheuerell
written and tested with Arduino 1.8.5
  
V-0.1.0 2018-08-30
initinal version
