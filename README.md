# loadbank_gateway
An application built for Arduino with Ethernet shield.   Accepts Modbus TCP register writes and translates them to commands for the Simplex loadbank with AutomationDirect PLCs with ECOM Ethernet modules.

This application uses the MgsModbus library for Modbus TCP slave functionality, which uses a single block of memory for all modbus data (mbData[] array).  The Modbus commands are relayed to the loadbank PLC using commands of unknown protocol (the command messages were deciphered by monitoring traffic from the Simplex touchscreen).  The loadbank communications are over UDP.  Writes to multi-register values must be carried out in a single multiple-register write (Function Code 16).

There is a holdoff of 500ms on sending commands to the loadbank to prevent excessive wear on the contactors.

The command can be written using either the float32 or uint16 formats - see the register definitions below:

| Register	| Data Type	| R/W	| Description   			| Units 			|
| :---:		|:---:		| :---:	|---						|:---:				|
| 40001 	| float32 	| R/W  	|kW command lower 16bits	| kW            	|	
| 40002 	| " "    	| " "  	|kW command upper 16bits	|  " "             	|	
| 40003 	| float32 	| R/W  	|PF command lower 16bits	| normalized 0-1	|	
| 40004		| " "   	| " "   |PF command upper 16bits	| " "              	|	
| 40201		| uint16  	| R/W   |kW command             	| kW            	|	
| 40202		| uint16  	| R/W   |PF command             	| % (range 0-100)	|	
    
Author:  Mike Scheuerell
written and tested with Arduino 1.8.5
  
V-0.1.0 2018-08-30
initinal version
