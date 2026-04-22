#ifndef PDS_H_
#define PDS_H_
#include "macro.h"

typedef struct Progress_Database_S {
  uint8_t sent_Id = 0; //送ったId
  uint8_t sent_LAP = 0; //送ったメッセージ
  uint8_t status = 0; //同一Idの通信フェーズ
  uint32_t LastTime = 0; //前回送った時間
} PDS;

typedef struct IP_Manager {
  IPAddress clientIP;
  unsigned int clientPort = 0;
  uint32_t LastCommunication = 0;
  PDS database[STRUCTURE_AREA];
} IM;

#endif
