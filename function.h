#ifndef FUNCTION_H_
#define FUNCTION_H_

uint8_t assenbleLAP(uint8_t com, uint8_t data);
uint8_t CRC_8(const uint8_t subject);
uint8_t* buildPayload(uint8_t Id, uint8_t AorN, uint8_t com, uint8_t data);
bool MessageIntegrityChecker(uint8_t* lap);
uint8_t* parseMessage(uint8_t mes);
void move_NextPhase(uint8_t Id, uint32_t LastTime);
void resendReport(uint8_t Id, uint32_t LastTime);
uint8_t* getPayload(uint8_t Id);
void savetoPDC(uint8_t* payload, uint8_t status, uint32_t LastTime);
void deleteData(uint8_t Id);
uint8_t* waitTimeout(uint32_t CurrentTime);
uint8_t Length(uint8_t* Ids);

#endif
