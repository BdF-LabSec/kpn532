/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#pragma GCC optimize("-Ofast")
#include <kpn532.h>
#define KPN532_0_CS 10
#define KPN532_0_IRQ 3

PN532 *pNFCReader;

void ISR_NFCReader() {
  pNFCReader->IrqState = 0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  PN532::InitGlobalSPI();

  Serial.println("-anticoll A-");

  pNFCReader = new PN532(KPN532_0_CS, KPN532_0_IRQ, ISR_NFCReader);
  pNFCReader->begin();
}

void loop() {
  uint8_t *pResult, cbResult, bSuccess;
  uint8_t UID[10], cbUID = 0, SENS_RES[2], SEL_RES;

  Serial.println("~Waiting target~");
  if (pNFCReader->InListPassiveTarget(UID, sizeof(UID), &cbUID, SENS_RES, &SEL_RES)) {
    Serial.print("SENS_RES: ");
    PN532::PrintHex(SENS_RES, sizeof(SENS_RES));
    Serial.print("SEL_RES : ");
    PN532::PrintHex(&SEL_RES, sizeof(SEL_RES));
    Serial.print(' ');
    PN532::PrintHex(UID, cbUID);
    if (pNFCReader->InRelease()) {
      Serial.println("|Target released");
    }
  }

  while (1)
    ;
}
