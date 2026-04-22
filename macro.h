#ifndef MACRO_H_
#define MACRO_H_

#define STRUCTURE_AREA 128
#define LIMIT_AREA 64
#define PAYLOAD_SIZE 4 // メモリ領域確保用
#define FIELD 4 //LAPフィールド数
#define UDPP_LENGTH 2 //UDP+データ長
#define LAP_LENGTH 2//LAPデータ長
#define RESEND_TIMEOUT 5000 // 再送タイムアウト
#define INVAIL_VALUE 255 //参照できないId
//----------LAPフィールドマクロ---------
//使用例 : *(parsedLAP + COMMAND) <- コマンドフィールドの値
#define START_F 0
#define COMMAND_F 1
#define DATA_LENGTH_F 2
#define DATA_F 3
//------------------------------------

//----------通信コマンドマクロ-----------
#define LED_SEND 1
#define REQUEST 2
#define RESPONSE 3
#define ACKNOWLEDGEMENT 4
//------------------------------------

//-----------データマクロ---------------
#define LED_ON 1
#define LED_OFF 2
#define LED_DEMAND 3
//------------------------------------

//-----------通信フェーズマクロ---------------
//-----------ポーリング時--------------------
#define WAIT_RESPONSE 1
#define SENT_ACK 2
//-----------------------------------------
//-----------クライアント状態送信時------------
#define WAIT_ACK 1
#define COMPLETE 2
//-----------------------------------------

#endif
