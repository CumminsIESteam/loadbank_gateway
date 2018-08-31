/*
  loadbank_gateway - an applicatoin built for Arduino with Etehrnet shield.
  Accepts Modbus TCP register writes and translates them to commands for 
  the Simplex loadbanks with AutomationDirect PLCs with ECOM Ethernet modules.

  This application uses the MgsModbus library for Modbus TCP slave functionality, 
  which uses a single block of memory for all modbus data (mbData[] array).  The 
  Modbus commands are relayed to the loadbank PLC using commands of unknown protocol
  (the command messages were deciphered by monitoring traffic from the Simplex 
  touchscreen).  The loadbank communications are over UDP.  Writes to multi-register 
  values must be carried out in a single multiple-register write (Function Code 16).

  The command can be written using either the float32 or uint16 formats - see the
  register definitions below:

  40001:  float32 kW command - lower 16bits units: kW
  40002:  float32 kW command - upper 16bits 
  40003:  float32 PF command - lower 16bits units: normalized 0-1
  40004:  float32 PF command - upper 16bits
  40201:  uint16  kW command - units: kW
  40202:  uint16  PF command - units: % (range 0-100)
    
  Author:  Mike Scheuerell
  written and tested with Arduino 1.8.5
  
  V-0.1.1 XXXXXX
  bugfix
  
  V-0.1.0 2018-08-30
  initinal version
*/

#include <SPI.h>
#include <Ethernet.h>
#include "MgsModbus.h"
#include "loadbank_control.h"

MgsModbus Mb;   // Modbus object
int inByte = 0; // incoming serial byte

// Ethernet settings (depending on MAC and Local network)
byte mac[] = {0x90, 0xA2, 0xDA, 0x0E, 0x94, 0xB5 };
IPAddress ip(192, 168, 100, 10);
IPAddress gateway(192, 168, 0, 49);
IPAddress subnet(255, 255, 0, 0);

EthernetUDP ethernet_udp;
IPAddress loadbank_ip_address(192, 168, 100, 11);  // Simplex loadbank PLC address

/** beginning of setup code  **********************************************************************************/
void setup()
{
  // serial setup
  Serial.begin(9600);
  Serial.println("Serial interface started");

  // initialize Ethernet
  Ethernet.begin(mac, ip, gateway, subnet);   // start ethernet interface
  ethernet_udp.begin(LOCAL_UDP_PORT);         // open UPD port
  Serial.println("Ethernet interface started"); 


  // Fill MbData
//  Mb.SetBit(0,false);
  Mb.MbData[0] = 0;
  Mb.MbData[1] = 0;
  Mb.MbData[2] = 0;
  Mb.MbData[3] = 0;
  Mb.MbData[4] = 0;
  Mb.MbData[5] = 0;
  Mb.MbData[6] = 0;
  Mb.MbData[7] = 0;
  Mb.MbData[8] = 0;
  Mb.MbData[9] = 0;
  Mb.MbData[10] = 0;
  Mb.MbData[11] = 0;
  // print menu
  Serial.println("0 - print the first 12 words of the MbData space");
  Serial.println("1 - fill MbData with 0kW, 1.0PF");
  Serial.println("2 - fill MbData with 3000kW, 0.9PF");
}
/** end of setup code *************************************************************************************/

/** beginning of execution code ***************************************************************************/
void loop()
{
  uint8 * packet_ptr;
  uint16 packet_size;
  bool new_command_flag = FALSE;
  union float_bytes { float32 float_rep[2];  uint16 uint16_rep[4]; }; 
  static float_bytes float_data;

#ifdef DEBUG                            
  if (Serial.available() > 0) {
    // get incoming byte:
    inByte = Serial.read();
    if (inByte == '0') {                                          // print MbData
      for (int i=0;i<12;i++) {
        Serial.print("address: "); Serial.print(i); Serial.print("Data: "); Serial.println(Mb.MbData[i]);
      }
    }  
    if (inByte == '1') {Mb.MbData[0] = 0x0000; Mb.MbData[1] = 0x0000; Mb.MbData[2] = 0x0000; Mb.MbData[3] = 0x3f80;}; //0x3f80 = ‭16256‬ 
    if (inByte == '2') {Mb.MbData[0] = 0x8000; Mb.MbData[1] = 0x453b; Mb.MbData[2] = 0x6666; Mb.MbData[3] = 0x3f66;};  
  }
#endif

  if( Mb.MbsRun() )    // run Modbus slave to process received Modbus commands
  {
    //check for changed register 40001-40004
    if (      float_data.uint16_rep[0] != Mb.MbData[0] ||   
              float_data.uint16_rep[1] != Mb.MbData[1] ||
              float_data.uint16_rep[2] != Mb.MbData[2] ||
              float_data.uint16_rep[3] != Mb.MbData[3] )
    {
              float_data.uint16_rep[0] = Mb.MbData[0]; 
              float_data.uint16_rep[1] = Mb.MbData[1]; 
              float_data.uint16_rep[2] = Mb.MbData[2];
              float_data.uint16_rep[3] = Mb.MbData[3]; 
              new_command_flag = TRUE;
    }
    //check for changed register 40201-40202
    else if ( (uint16)(float_data.float_rep[0])     != Mb.MbData[200] ||  
              (uint16)(float_data.float_rep[1]*100) != Mb.MbData[201] )
    {
              float_data.float_rep[0] = (float32)(Mb.MbData[200]);
              float_data.float_rep[1] = ((float32)(Mb.MbData[201]))/100;
              new_command_flag = TRUE;
    }
    
    if ( new_command_flag == TRUE)
    {
      new_command_flag = FALSE;
      // synchronize the float32 and uint16 registers
      Mb.MbData[0] = float_data.uint16_rep[0];
      Mb.MbData[1] = float_data.uint16_rep[1];
      Mb.MbData[2] = float_data.uint16_rep[2];
      Mb.MbData[3] = float_data.uint16_rep[3];    
      Mb.MbData[200] = (uint16)(float_data.float_rep[0]);
      Mb.MbData[201] = (uint16)(float_data.float_rep[1]*100); 
      Serial.println(float_data.float_rep[0]);     
      
      // build and send kw and pf packets using modbus register data
      build_packet(KW_COMMAND, float_data.float_rep[0], &packet_ptr, &packet_size);
      send_udp_packet(ethernet_udp, loadbank_ip_address, LOADBANK_PORT, packet_ptr, packet_size);
      build_packet(PF_COMMAND, float_data.float_rep[1], &packet_ptr, &packet_size);
      send_udp_packet(ethernet_udp, loadbank_ip_address, LOADBANK_PORT, packet_ptr, packet_size);
      
      // send "apply/change numeric" command sequence
      loadbank_apply_change_numeric(ethernet_udp, loadbank_ip_address, LOADBANK_PORT);
    }
  }
}
/** end execution code *******************************************************************************/
