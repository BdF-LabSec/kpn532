/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#pragma GCC optimize("-Ofast")
#include <kpn532_mfc.h>
#define KPN532_0_CS 10
#define KPN532_0_IRQ 3

MFC *pMFC;

void ISR_NFCReader() {
  pMFC->pNFC->IrqState = 0;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;
  PN532::InitGlobalSPI();  // this example will use SPI only for PN532(s)

  Serial.println("-* Simple MFC *-");

  pMFC = new MFC(KPN532_0_CS, KPN532_0_IRQ, ISR_NFCReader);

  Serial.print("0|");
  pMFC->begin();
}

const uint8_t MIFARE_EMPTY_KEY[MIFARE_CLASSIC_KEY_SIZE] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

void loop() {
  uint8_t i, *pResult;

  Serial.println("~Waiting target~");
  if (pMFC->Select()) {
    Serial.print("SENS_RES: ");
    PN532::PrintHex(pMFC->SENS_RES, sizeof(pMFC->SENS_RES));
    Serial.print("SEL_RES : ");
    PN532::PrintHex(&pMFC->SEL_RES, sizeof(pMFC->SEL_RES));
    Serial.print("UID     : ");
    PN532::PrintHex(pMFC->UID, MIFARE_CLASSIC_UID_SIZE);

    for (i = 0; i < (MIFARE_CLASSIC_SECTORS * MIFARE_CLASSIC_BLOCKS_PER_SECTOR); i++) {

      if (!(i % MIFARE_CLASSIC_BLOCKS_PER_SECTOR)) {
        Serial.println("--------|---------------------------------");
      }

      Serial.print(i);
      Serial.print("\t| ");

      if (!(i % MIFARE_CLASSIC_BLOCKS_PER_SECTOR)) {
        if (!pMFC->Authenticate(MIFARE_CLASSIC_CMD_AUTHENTICATION_KEY_A, i, MIFARE_EMPTY_KEY)) {
          Serial.print("AUTH Error on block: ");
          Serial.println(i);
          i += (MIFARE_CLASSIC_BLOCKS_PER_SECTOR - 1);
          continue;
        }
      }

      if (pMFC->Read(i, &pResult)) {
        PN532::PrintHex(pResult, MIFARE_CLASSIC_BLOCK_SIZE);
      } else {
        Serial.print("READ Error on block: ");
        Serial.println(i);
      }

    }

    if (pMFC->Release()) {
      Serial.println("|Target released");
    }
  }

  while (1)
    ;
}
