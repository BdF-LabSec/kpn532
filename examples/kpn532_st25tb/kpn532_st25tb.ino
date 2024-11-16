/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#pragma GCC optimize("-Ofast")
#include <kpn532_st25tb.h>
#define KPN532_0_CS 10
#define KPN532_0_IRQ 3

ST25TB *pST25TB;

void ISR_NFC0() {
  pST25TB->pNFC->IrqState = 0;
}

void ReadAndPrintBlock(const uint8_t no);

void setup(void) {
  Serial.begin(115200);
  PN532::InitGlobalSPI(); // this example will use SPI only for PN532(s)

  Serial.println("- kpn532 st25tb-");

  pST25TB = new ST25TB(KPN532_0_CS, KPN532_0_IRQ, ISR_NFC0);

  Serial.print("0|");
  pST25TB->begin();
}

void loop(void) {
  uint8_t ui8ChipId, UID[8], i;

  pST25TB->pNFC->RfConfiguration__RF_field(0x01);
  delay(10);

  Serial.print("| Initiate...");
  if (pST25TB->Initiate(&ui8ChipId, 0x01)) {
    Serial.println();
    Serial.print("ChipId: ");
    Serial.println(ui8ChipId, HEX);

    Serial.print("| Select...");
    if (pST25TB->Select(ui8ChipId)) {
      Serial.println();
      Serial.print("| Get_Uid...");
      if (pST25TB->Get_Uid(UID)) {
        Serial.println();
        PN532::PrintHex(UID, sizeof(UID), PN532_PRINTHEX_REV);
      } else {
        Serial.println("KO");
      }

      ReadAndPrintBlock(0x05);
      ReadAndPrintBlock(0x06);

      Serial.print("| Completion...");
      if(pST25TB->Completion()) {
        Serial.println();
      } else {
        Serial.println("KO");
      }
    } else {
      Serial.println("KO");
    }
  } else {
    Serial.println("KO");
  }

  delay(5);
  pST25TB->pNFC->RfConfiguration__RF_field(0x00);

  Serial.println("END");

  while (1)
    ;
}

void ReadAndPrintBlock(const uint8_t no) {
  uint8_t data[4];

  if (no < 0x10) {
    Serial.print('0');
  }
  Serial.print(no, HEX);
  Serial.print('|');

  if (pST25TB->Read_Block(no, data)) {
    PN532::PrintHex(data, sizeof(data), PN532_PRINTHEX_REV);
  } else {
    Serial.println("error!");
  }
}