/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#pragma GCC optimize("-Ofast")
#include <kpn532.h>
#define KPN532_0_CS 10
#define KPN532_0_IRQ 3

PN532 *pNFC;

void ISR_NFCEmulator() {
  pNFC->IrqState = 0;
}

const uint8_t UID[7] = { 0x04, 0x95, 0x91, 0x0a, 0x5d, 0x6d, 0x80 }, SENS_RES[2] = { 0x03, 0x44 }, SEL_RES = 0x20;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  PN532::InitGlobalSPI();

  Serial.println("- emul T4T -");

  Serial.print("ATQA: 0x");
  PN532::PrintHex(SENS_RES, sizeof(SENS_RES), PN532_PRINTHEX_REV);  // SENS_RES is stored byte-swapped
  Serial.print("SAK : 0x");
  PN532::PrintHex(&SEL_RES, sizeof(SEL_RES));
  Serial.print("UID : 0x");
  PN532::PrintHex(UID, sizeof(UID));

  pNFC = new PN532(KPN532_0_CS, KPN532_0_IRQ, ISR_NFCEmulator);
  pNFC->begin();
}

void loop() {
  Serial.print("~Waiting reader~");
  if (pNFC->TgInitAsTarget(UID, sizeof(UID), SENS_RES, SEL_RES)) {
    Serial.println();
    Serial.println("|Reader detected");
  }
}
