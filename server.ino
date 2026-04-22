#include <WiFiS3.h>
#include "macro.h"
#include "UDP_plus.h"
#include "function.h"
#include "Arduino_LED_Matrix.h"

char ssid[] = "hackathon006-WPA2";//WiFiのSSIDとパスワードを定義
char pass[] = "hackathon006";
int status = WL_IDLE_STATUS;

IPAddress _sm(192, 168, 11, 96);
IPAddress SourceIP;
unsigned int serverPort = 8000;
unsigned int clientPort;

uint8_t state = LED_OFF;

uint32_t LastTime = 0;
uint32_t wifi_idletime = 0;

uint8_t reply[4];
uint8_t* parsedUdpp;
uint8_t* parsedLAP;
uint8_t* payload;
uint8_t* Index_Id;
uint8_t* II;
uint8_t Id;
uint8_t ack = 1;
uint32_t last_m = 0;

uint8_t ON[8][12] = {
  {0,0,0,0,0,0,  0,0,0,0,0,0},
  {0,1,1,1,1,0,  0,1,0,0,1,0},
  {0,1,0,0,1,0,  0,1,1,0,1,0},
  {0,1,0,0,1,0,  0,1,1,0,1,0},
  {0,1,0,0,1,0,  0,1,0,1,1,0},
  {0,1,0,0,1,0,  0,1,0,1,1,0},
  {0,1,1,1,1,0,  0,1,0,0,1,0},
  {0,0,0,0,0,0,  0,0,0,0,0,0}
};

uint8_t OFF[8][12] = {
  {0,0,0,0,  0,0,0,0,  0,0,0,0},
  {1,1,1,0,  1,1,1,0,  1,1,1,0},
  {1,0,1,0,  1,0,0,0,  1,0,0,0},
  {1,0,1,0,  1,1,1,0,  1,1,1,0},
  {1,0,1,0,  1,0,0,0,  1,0,0,0},
  {1,0,1,0,  1,0,0,0,  1,0,0,0},
  {1,1,1,0,  1,0,0,0,  1,0,0,0},
  {0,0,0,0,  0,0,0,0,  0,0,0,0}
};

ArduinoLEDMatrix matrix;
UDPplus Udpp;

void WiFiModuleReboot() {
  WiFi.end();
  delay(5);

  WiFi.config(_sm);
  while(status != WL_CONNECTED){
    status = WiFi.begin(ssid,pass);
  }
  Serial.println("Connected to router!");
  Udpp.begin(serverPort);
}

void setup() {
  Serial.begin(9600);

  WiFi.config(_sm);//デバイスのIPアドレスを手動で設定固定IP

 //statusがWL_CONNECTEDになるまでwhileを繰り返す
  while(status != WL_CONNECTED){
    //WAP2のネットワークに接続
    status = WiFi.begin(ssid,pass);
  }
  Serial.println("Connected to router!");
  Udpp.begin(serverPort);//サーバのポートで待ち受け
  matrix.begin();
  matrix.clear();
  matrix.renderBitmap(OFF, 8, 12);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if ((millis() - wifi_idletime) > 500) {
    WiFiModuleReboot();
    wifi_idletime = millis();
    }
  }
  else
    wifi_idletime = millis();
  
  int packetSize = Udpp.parsePacket();
  if(packetSize){
    SourceIP = Udpp.remoteIP();
    clientPort = Udpp.remotePort();
    int len = Udpp.read(reply,4);
    reportIP(SourceIP, clientPort);
    parsedUdpp = Udpp.parseHeader(reply[0]);
    II = getIndexID(SourceIP, *parsedUdpp);
    switch (packetSize) {
      case UDPP_ONLY: deleteData(II, SourceIP);
                      break;
      case UDPP_AND_LAP:  parsedLAP = parseMessage(reply[1]);
                          switch(*(parsedLAP + COMMAND_F)) {
                            case LED_SEND:  if (getStatus(II) != SENT_ACK) {
                                              if (state == LED_OFF) {
                                  
                                                state = LED_ON;
                                                
                                                matrix.renderBitmap(ON, 8, 12);
                                                Serial.println("a");
                                                //Serial.println("ON");
                                              }
                                              else {
                                                state = LED_OFF;
                                                
                                                matrix.renderBitmap(OFF, 8, 12);
                                                Serial.print("b");
                                                //Serial.println("OFF");
                                              }
                                              Udpp.sendACK(II, SourceIP, clientPort);
                                              yield();
                                              //Serial.println(state);
                                            }
                                            else
                                              Udpp.resendACK(II, SourceIP, clientPort);
                                            break;
                            case REQUEST: payload = buildPayload(*parsedUdpp, ack, RESPONSE, state);

                                          Udpp.beginPacket(SourceIP, clientPort);//サーバのIPアドレスとポート番号
                                          Udpp.write(payload, PAYLOAD_SIZE);//書き込み
                                          Udpp.endPacket();//サーバ送信
                                          //Serial.println("Sent request");
                                          yield();
                                          //クライアントデータベースに記録
                                          savetoPDC(II, payload, SourceIP, clientPort, WAIT_ACK, millis());
                                          break;
                          }
    }
    reply[1] = 0;
  }
  //Serial.println(millis() - last_m);
  last_m = millis();

  if ((last_m - LastTime) > 5000) {
    resetStatus();
    LastTime = millis();
  }
  yield();
}
