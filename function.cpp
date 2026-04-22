#include <WiFiS3.h>
#include "UDP_plus.h"
#include "macro.h"
#include "PDC.h"
#include "function.h"

using namespace std;

PDC database[STRUCTURE_AREA];

uint8_t assenbleLAP(uint8_t com, uint8_t data) {
  uint8_t message;
  uint8_t start_f = 0b1;
  uint8_t command_f;
  switch (com) {
    case 1: command_f = 0b00; break;
    case 2: command_f = 0b01; break;
    case 3: command_f = 0b10; break;
    case 4: command_f = 0b11; break;
  }
  uint8_t size_f = 0b10;
  uint8_t data_f;
  switch (data) {
    case 1: data_f = 0b000; break;
    case 2: data_f = 0b001; break;
    case 3: data_f = 0b010; break;
  }

  message = (start_f << 7) + (command_f << 5) + (size_f << 3) + data_f;
  return message;
}

uint8_t CRC_8(const uint8_t subject) {
  uint8_t crc = 0x00;
  uint8_t poly = 0x07;

    crc ^= subject;
    for (int i = 0; i < 8; i++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ poly;
      } 
      else {
        crc <<= 1;
      }
    }
    return crc;
}

uint8_t* buildPayload(uint8_t Id, uint8_t AorN, uint8_t com, uint8_t data) {
  static uint8_t* payLoad = new uint8_t[PAYLOAD_SIZE];
  uint8_t udp_plus_header;
  uint8_t checksum_UDPp;
  uint8_t LAP_message;
  uint8_t checksum_LAP;
  udp_plus_header = Udpp.makeUdpPlusHeader(Id, AorN);
  checksum_UDPp = CRC_8(udp_plus_header);
  LAP_message = assenbleLAP(com, data);
  checksum_LAP = CRC_8(LAP_message);
  *payLoad = udp_plus_header;
  *(payLoad + 1) = checksum_UDPp;
  *(payLoad + 2) = LAP_message;
  *(payLoad + 3) = checksum_LAP;
  return payLoad;
}

bool MessageIntegrityChecker(uint8_t* lap) {
  uint8_t message = *lap;
  uint8_t checksum = *(lap + 1);
  if (checksum == CRC_8(message))
    return true;
  else
    return false;
}

uint8_t* parseMessage(uint8_t mes) {
  static uint8_t* convertedData = new uint8_t[4];
  uint8_t start_f, command_f, size_f, data_f;
  uint8_t p_com, p_data;
  start_f = mes >> 7;
  command_f = (mes >> 5) & 0b11;
  size_f = (mes >> 3) & 0b11;
  data_f = mes & 0b111;
  switch (command_f) {
    case 0: p_com = LED_SEND; break;
    case 1: p_com = REQUEST; break;
    case 2: p_com = RESPONSE; break;
    case 3: p_com = ACKNOWLEDGEMENT; break;
  }
  switch (data_f) {
    case 0: p_data = LED_ON; break;
    case 1: p_data = LED_OFF; break;
    case 2: p_data = LED_DEMAND; break;
  }
  
  *convertedData = start_f;
  *(convertedData + 1) = p_com;
  *(convertedData + 2) = size_f;
  *(convertedData + 3) = p_data;

  return convertedData;
}

void move_NextPhase(uint8_t Id, uint32_t LastTime) {
  database[Id].resend_count = 0;
  database[Id].status++;
  database[Id].LastTime = LastTime;
}

void resendReport(uint8_t Id, uint32_t LastTime) {
  database[Id].resend_count++;
  database[Id].LastTime = LastTime;
}

uint8_t* getPayload(uint8_t Id) {
  static uint8_t* Payload = new uint8_t[PAYLOAD_SIZE];
  *Payload = (database[Id].sent_Id << 1) + 1;
  *(Payload + 1) = database[Id].checksum_Udpp;
  *(Payload + 2) = database[Id].sent_LAP;
  *(Payload + 3) = database[Id].checksum_LAP;

  return Payload; 
}

void savetoPDC(uint8_t* payload, uint8_t status, uint32_t LastTime) {
  uint8_t Id = (*payload >> 1);
  database[Id].sent_Id = Id;
  database[Id].checksum_Udpp = *(payload + 1);
  database[Id].sent_LAP = *(payload + 2);
  database[Id].checksum_LAP = *(payload + 3);
  database[Id].resend_count = 0;
  database[Id].status = status;
  database[Id].LastTime = LastTime;
}

void deleteData(uint8_t Id) {
  database[Id].checksum_Udpp = 0;
  database[Id].sent_LAP = 0;
  database[Id].checksum_LAP = 0;
  database[Id].resend_count = 0;
  database[Id].status = 0;
  database[Id].LastTime = 0;
}

uint8_t* waitTimeout(uint32_t CurrentTime) {
  static uint8_t* IDs = new uint8_t[LIMIT_AREA];
  int j = 0;
  for(int i = 0; i < LIMIT_AREA; i++) {
    if (database[i].status == 1) {
      if ((CurrentTime - database[i].LastTime) > RESEND_TIMEOUT) {
        *(IDs + j) = database[i].sent_Id;
        j++;
      }
    }
  }
  *(IDs + j) = 255;
  return IDs;
}

uint8_t Length(uint8_t* Ids) {
  int j = 0;
  while (*(Ids + j) != 255)
    j++;
    
  return j;
}
