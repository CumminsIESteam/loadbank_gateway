#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include "loadbank_control.h"


/*  TCP Client not used
void send_tcp_packet(EthernetClient client, IPAddress ServerIp, uint16 Port, uint8 * packet_ptr, uint16 packet_size)
{
  uint8 packet_array[packet_size];
  if (client.connect(ServerIp,Port)) {
    packet_array[0] = packet_ptr;
    #ifdef DEBUG
      Serial.println("connected with slave");
      Serial.print("Master request: ");
      for(int i=0;i<packet_size;i++) {
        if(packet_array[i] < 16){Serial.print("0");} // pad hex values with leading zero
        Serial.print(packet_array[i],HEX);
        if (i != packet_size - 1) {Serial.print(".");} else {Serial.println();}
      }
    #endif    
    for(int i=0;i<packet_size;i++) {
      client.write(packet_array[i]);
    }
  } else {
    #ifdef DEBUG
      Serial.println("connection with slave failed");
    #endif    
    client.stop();
  }
}
*/

void send_udp_packet(EthernetUDP ethernet_udp, IPAddress ServerIp, uint16 Port, uint8 packet_array[], uint16 packet_size)
{
  //uint8 packet_array[packet_size];
  //packet_array = packet_ptr;
  
  ethernet_udp.beginPacket(ServerIp, Port);
  for(int i=0;i<packet_size;i++) {
#ifdef DEBUG
    if(packet_array[i] < 16){Serial.print("0");} // pad hex values with leading zero
    Serial.print(packet_array[i],HEX);
    if (i != packet_size - 1) {Serial.print(".");} else {Serial.println();}
#endif
    ethernet_udp.write(packet_array[i]);
  }
  ethernet_udp.endPacket();
  delay(20);
}


void build_packet(uint8 command, float32 input_value, uint8 ** packet_ptr, uint16 * packet_size) 
{
   const uint8 pre_crc[5] = // undescript bytes needed before CRC
   {
     0x48, 0x41, 0x50, 0x2b, 0x54 
   };
   const uint8 post_crc[] = { 0x0c };                                   // undescript byte needed after CRC
   const uint8 pre_address[] = { 0x00, 0x19, 0x00, 0x01, 0x20, 0x04 };  // undescript bytes needed before address bytes
   const uint8 address_arr[2][3] = { {0x8d, 0x02, 0x31} ,               /*  kw address - bit pattern identified for setting kw in loadbank PLC */
                                     {0x87, 0x02, 0x31} };              /*  pf address - bit pattern identified for setting power factor in loadbank PLC */
   uint8 input_byte_array[4];
   uint8 offset;
   union crc_union { uint16 u16;  uint8 u8arr[2]; };  // union used to conver between uint16 array and uint8 array (uint8 needed for sending data over Ethernet)
   crc_union crc_union;
   uint8 crc_bytes[2];
   uint8 crc_in[sizeof(pre_address) + sizeof(address_arr[1]) + sizeof(input_value)];   
   uint8 packet[sizeof(pre_crc) + sizeof(crc_bytes) + sizeof(post_crc)  + sizeof(crc_in)];
   const uint8 sizes[] = {sizeof(pre_crc), sizeof(crc_bytes), sizeof(post_crc), sizeof(pre_address), sizeof(address_arr[1]), sizeof(input_byte_array)};
   uint8* addresses[] = {pre_crc, crc_bytes, post_crc, pre_address, address_arr[1], input_byte_array};;

    //convert float value to byte array
    float_to_bytes_swapped(input_value, input_byte_array);
    //prepare crc input
    offset = 0;
    memcpy(crc_in + offset, pre_address, sizeof(pre_address));
    offset = offset + sizeof(pre_address);
    memcpy(crc_in + offset, address_arr[command], sizeof(address_arr[command]));
    offset = offset + sizeof(address_arr[command]);
    memcpy(crc_in + offset, &input_value, sizeof(input_value)); 
    //calculate crc
    crc_union.u16 = calc_crc_ccitt_xmodem(crc_in, sizeof(crc_in));;
    // swap bytes
    crc_bytes[1]=crc_union.u8arr[1];
    crc_bytes[0]=crc_union.u8arr[0];
    //build packet
    offset = 0;
    addresses[4] = address_arr[command];
    for (uint8 j = 0; j < 6; j++)
    {
      memcpy(packet + offset, addresses[j], sizes[j]);
      offset = offset + sizes[j];
    }   
   *packet_ptr = packet;  // pass the address to the first element of packet array
   *packet_size = sizeof(packet);  
}

void float_to_bytes_swapped(float32 float_val, uint8 byte_array[])
{
  union float_bytes { float32 float_rep;  uint8 byte_rep[4]; };
  float_bytes u1;
  u1.float_rep = float_val;
  byte_array[0]=u1.byte_rep[0];
  byte_array[1]=u1.byte_rep[1];
  byte_array[2]=u1.byte_rep[2];
  byte_array[3]=u1.byte_rep[3];
}

uint16 calc_crc_ccitt_xmodem(const uint8 byte_array[], const uint16 array_size)
{
  uint16 lookup_index;
  uint16 crc = 0;
  uint16 crc_ui16_lookup_table [] = { 0,4129,8258,12387,16516,20645,24774,28903,33032,37161,41290,45419,49548,\
          53677,57806,61935,4657,528,12915,8786,21173,17044,29431,25302,37689,33560,45947,41818,54205,\
          50076,62463,58334,9314,13379,1056,5121,25830,29895,17572,21637,42346,46411,34088,38153,58862,\
          62927,50604,54669,13907,9842,5649,1584,30423,26358,22165,18100,46939,42874,38681,34616,63455,\
          59390,55197,51132,18628,22757,26758,30887,2112,6241,10242,14371,51660,55789,59790,63919,35144,\
          39273,43274,47403,23285,19156,31415,27286,6769,2640,14899,10770,56317,52188,64447,60318,39801,\
          35672,47931,43802,27814,31879,19684,23749,11298,15363,3168,7233,60846,64911,52716,56781,44330,\
          48395,36200,40265,32407,28342,24277,20212,15891,11826,7761,3696,65439,61374,57309,53244,48923,\
          44858,40793,36728,37256,33193,45514,41451,53516,49453,61774,57711,4224,161,12482,8419,20484,\
          16421,28742,24679,33721,37784,41979,46042,49981,54044,58239,62302,689,4752,8947,13010,16949,\
          21012,25207,29270,46570,42443,38312,34185,62830,58703,54572,50445,13538,9411,5280,1153,29798,\
          25671,21540,17413,42971,47098,34713,38840,59231,63358,50973,55100,9939,14066,1681,5808,26199,\
          30326,17941,22068,55628,51565,63758,59695,39368,35305,47498,43435,22596,18533,30726,26663,6336,\
          2273,14466,10403,52093,56156,60223,64286,35833,39896,43963,48026,19061,23124,27191,31254,2801,6864,\
          10931,14994,64814,60687,56684,52557,48554,44427,40424,36297,31782,27655,23652,19525,15522,11395,\
          7392,3265,61215,65342,53085,57212,44955,49082,36825,40952,28183,32310,20053,24180,11923,16050,3793,7920};

  for  (uint16 i = 0; i < array_size; i++)
  {
    lookup_index = byte_array[i] ^ ((uint8)(crc >> 8));
    crc = crc_ui16_lookup_table[lookup_index] ^ ((crc << 8) % 65536);
    //Serial.println(crc);
  }
  return crc;
}

#ifdef DEBUG
void print_bytes(uint8 byte_array[], uint16 array_size)
{
    for  (uint8 j = 0; j < array_size; j++)
    {
      Serial.print(byte_array[j],HEX);
    }
    Serial.println();
}
#endif

void loadbank_apply_change_numeric(EthernetUDP ethernet_udp, IPAddress loadbank_ip_address, uint16 l_loadbank_port)
{
  // nondescript bytes needed to command loadbank to apply the commanded load value
  const uint8 arr18[4][18] = {{0x48, 0x41, 0x50, 0x44, 0xbf, 0x29, 0x95, 0x08, 0x00, 0x19, 0x00, 0x01, 0x1e, 0x02, 0xb3, 0x01, 0x33, 0x00},
                              {0x48, 0x41, 0x50, 0x48, 0xbf, 0x29, 0x95, 0x08, 0x00, 0x19, 0x00, 0x01, 0x1e, 0x02, 0xb3, 0x01, 0x33, 0x00},
                              {0x48, 0x41, 0x50, 0x4b, 0xbf, 0x29, 0x95, 0x08, 0x00, 0x19, 0x00, 0x01, 0x1e, 0x02, 0xb3, 0x01, 0x33, 0x00},
                              {0x48, 0x41, 0x50, 0x4e, 0xbf, 0x29, 0x95, 0x08, 0x00, 0x19, 0x00, 0x01, 0x1e, 0x02, 0xb3, 0x01, 0x33, 0x00}};
  const uint8 arr19[2][19] = {{0x48, 0x41, 0x50, 0x46, 0xbf, 0x96, 0x68, 0x0a, 0x00, 0x19, 0x00, 0x01, 0x20, 0x02, 0xb3, 0x01, 0x33, 0x10, 0x14},
                              {0x48, 0x41, 0x50, 0x4c, 0xbf, 0xe5, 0x6b, 0x0a, 0x00, 0x19, 0x00, 0x01, 0x20, 0x02, 0xb3, 0x01, 0x33, 0x00, 0x14}};
   
    // send "apply/change numeric" command sequence
    send_udp_packet(ethernet_udp, loadbank_ip_address, l_loadbank_port, arr18[0], 18);
    send_udp_packet(ethernet_udp, loadbank_ip_address, l_loadbank_port, arr19[0], 19);
    send_udp_packet(ethernet_udp, loadbank_ip_address, l_loadbank_port, arr18[1], 18);
    send_udp_packet(ethernet_udp, loadbank_ip_address, l_loadbank_port, arr18[2], 18);
    send_udp_packet(ethernet_udp, loadbank_ip_address, l_loadbank_port, arr19[1], 19);
    send_udp_packet(ethernet_udp, loadbank_ip_address, l_loadbank_port, arr18[3], 18);
}



