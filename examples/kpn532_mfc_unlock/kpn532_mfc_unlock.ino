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

  Serial.println("Simple MFC Gen1a");

  pMFC = new MFC(KPN532_0_CS, KPN532_0_IRQ, ISR_NFCReader);

  Serial.print("0|");
  pMFC->begin();
}

void loop() {
  uint8_t *pResult, i;

  Serial.println("~Waiting target~");

  if (pMFC->Select()) {
    Serial.print("SENS_RES: ");
    PN532::PrintHex(pMFC->SENS_RES, sizeof(pMFC->SENS_RES));
    Serial.print("SEL_RES : ");
    PN532::PrintHex(&pMFC->SEL_RES, sizeof(pMFC->SEL_RES));
    Serial.print("UID     : ");
    PN532::PrintHex(pMFC->UID, MIFARE_CLASSIC_UID_SIZE);

    if (pMFC->Special_Gen1a_unlock()) {
      for (i = 0; i < 4; i++) {
        if (pMFC->Read(i, &pResult)) {
          Serial.print(i);
          Serial.print("\t| ");
          PN532::PrintHex(pResult, MIFARE_CLASSIC_BLOCK_SIZE);
        } else {
            Serial.print("READ Error on block: ");
            Serial.println(i);
        }
      }
    }

    if (pMFC->Release()) {
      Serial.println("|Target released");
    }
  }

  pMFC->pNFC->RfConfiguration__RF_field(0x00);
  while (1)
    ;
}
