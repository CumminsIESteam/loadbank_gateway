#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

//**************************************************************************************
// type definitions START
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;
typedef signed char     sint8;
typedef short           sint16;
typedef long            sint32;
typedef float           float32;
typedef double          float64;
// type definitions END
//**************************************************************************************

//**************************************************************************************
// macro definitions START
#define LOADBANK_PORT 28784
#define KW_COMMAND    0
#define PF_COMMAND    1
#define LOCAL_UDP_PORT 4631
#define FALSE           0
#define TRUE            1
//#define DEBUG

void send_tcp_packet(EthernetClient client, IPAddress ServerIp, uint16 Port, uint8 * packet_ptr, uint16 packet_size);
void send_udp_packet(EthernetUDP ethernet_udp, IPAddress ServerIp, uint16 Port, uint8 packet_array[], uint16 packet_size);
void build_packet(uint8 command, float32 input_value, uint8 ** packet_ptr, uint16 * packet_size); 
void float_to_bytes_swapped(float32 float_val, uint8 byte_array[]);
uint16 calc_crc_ccitt_xmodem(const uint8 byte_array[], const uint16 array_size);
void print_bytes(uint8 byte_array[], uint16 array_size);
void loadbank_apply_change_numeric(EthernetUDP ethernet_udp, IPAddress loadbank_ip_address, uint16 l_loadbank_port);




