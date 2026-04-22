#include <WiFiS3.h>
#include "macro.h"
#include "UDP_plus.h"
#include "function.h"

using namespace std;

char ssid[] = "hackathon006-WPA2";
char pass[] = "hackathon006";
int status = WL_IDLE_STATUS;//接続状態を示す定数

IPAddress serverip(192,168,11,96);//サーバ側のIPアドレス
unsigned int serverPort = 8000;//サーバが使うポート
unsigned int localPort = 8080;//クライアントが使うポート

const int buttonPin = 11;
const int LEDPin = 8;
int lastState = HIGH;
int currentState = HIGH;
unsigned long lastDebounce = 0;
const unsigned long debounceDelay = 25;

unsigned long lastMillis = 0;
const uint8_t interval = 30;
unsigned long currentMillis;

uint8_t* reply;
uint8_t* Udpp_extraction = new uint8_t[UDPP_LENGTH];
uint8_t* LAP_extraction;
uint8_t* parsedUdpp;
uint8_t* parsedLAP;
uint8_t* payload;
uint8_t* Ids;
uint8_t Id;
uint8_t ack = 1;
bool Integrity;

UDPplus Udpp;

void setup() {
 Serial.begin(9600);

 while(status != WL_CONNECTED){
   status = WiFi.begin(ssid,pass);//WAP2のネットワークに接続
 }
 Serial.println("Connected to router!");
 IPAddress ip = WiFi.localIP();//DHCPによる割り当てIP
 pinMode(buttonPin, INPUT_PULLUP);
 pinMode(LEDPin, OUTPUT);
 digitalWrite(LEDPin, LOW);

 Udpp.begin(localPort);//localPortで受信を開始
}

void loop() {
  //サーバからの応答を待つ
  int packetSize = Udpp.parsePacket();
  if(packetSize){
    reply = new uint8_t[8];
    int len = Udpp.read(reply,8);
    for (int i = 0; i < len; i++) {
      // Serial.print(*(reply + i), HEX);
      // Serial.print(" ");
    }
    Serial.println("");
  }
  else{
    Serial.println("No data received...");
  }
  /*ここからUDP+とLAPの解析*/
  if (packetSize) {
    for (int i = 0; i < UDPP_LENGTH; i++)
      *(Udpp_extraction + i) = reply[i];
    if (Udpp.avoidError(Udpp_extraction) == true) {
      parsedUdpp = Udpp.parseHeader(*Udpp_extraction);
      if (*(parsedUdpp + 1) == 1 && packetSize > UDPP_LENGTH) {
        LAP_extraction = &reply[2];
        Integrity = MessageIntegrityChecker(LAP_extraction);
        if (Integrity == true) {
          parsedLAP = parseMessage(LAP_extraction[0]);
          if (*(parsedLAP + COMMAND_F) == RESPONSE) {
            switch(*(parsedLAP + DATA_F)) {
              case LED_ON:  digitalWrite(LEDPin, HIGH);
                            Serial.println("LED ON");
                            break;
              case LED_OFF: digitalWrite(LEDPin, LOW);
                            Serial.println("LED LOW");
                            break;
              default:      Udpp.resendData(*parsedUdpp, serverip, serverPort);
                            break;
            }
          }
          Udpp.sendACK(*parsedUdpp, serverip, serverPort);
        }
        else {
          Udpp.resendDemand(*parsedUdpp, serverip, serverPort);
        }
      }
      else if (*(parsedUdpp + 1) == 1) {
        //UDP+ヘッダのみが来た場合(ACK)
        //完了 -> その通信のIdが格納されているデータベースの削除
        deleteData(*parsedUdpp);
      }
      else {
        //前回送った内容をもう一度
        Udpp.resendData(*parsedUdpp, serverip, serverPort);
      }
    }
    else {
      Udpp.resendDemand(*parsedUdpp, serverip, serverPort);
    }
  }
  delete reply;
  //タクトスイッチ読み取り
  int reading = digitalRead(buttonPin);
  if (reading != lastState) 
    lastDebounce = millis();

  //データ送信
  if ((millis() - lastDebounce) > debounceDelay) {
    if (reading == LOW && currentState == HIGH) {
      Id = Udpp.getId();
      payload = buildPayload(Id, ack, LED_SEND, digitalRead(LEDPin) == HIGH ? LED_OFF : LED_ON);

      Udpp.beginPacket(serverip, serverPort);//サーバのIPアドレスとポート番号
      Udpp.write(payload, PAYLOAD_SIZE);//書き込み
      Udpp.endPacket();//サーバ送信
      Serial.println("Sent LED status to server");

      //クライアントデータベースに記録
      savetoPDC(payload, WAIT_ACK, millis());
    }
    currentState = reading;
  }
  lastState = reading;

  //リクエスト送信(必要に応じて周期を設定)
  if ((currentMillis = millis()) - lastMillis > interval) {
    Id = Udpp.getId();
    payload = buildPayload(Id, ack, REQUEST, LED_DEMAND);

    Udpp.beginPacket(serverip, serverPort);
    Udpp.write(payload, PAYLOAD_SIZE);
    Udpp.endPacket();
    Serial.println("Sent LED request to server");
    lastMillis = currentMillis;

    //クライアントデータベースに記録
    savetoPDC(payload, WAIT_RESPONSE, millis());
  }

  //応答待ちタイムアウトを超過したデータの再送
  if (Ids = waitTimeout(millis())) {
    for (int i = 0; i < Length(Ids); i++) {
      Udpp.resendData(*(Ids + i), serverip, serverPort);
      uint32_t sentTime = millis();
      while ((millis() - sentTime) < 10); //送信周期
    }
  }

  delay(50);
}
