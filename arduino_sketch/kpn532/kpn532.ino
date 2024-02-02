/*	Benjamin DELPY `gentilkiwi`
	LabSec - DGSI DIT ARCOS
	benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
	Licence : https://creativecommons.org/licenses/by/4.0/
*/
#pragma GCC optimize("-Ofast")
#include "kpn532_spi.h"

#define KPN532_VERBOSE 0  // or 1, be careful, verbose slow down the relay

void SetupChip(PN532_SPI *pNFC);
void PrintHex(const byte *pcbData, const uint32_t cbData);

PN532_SPI *pNFCReader, *pNFCEmulator;

void ISR_NFCReader() {
  pNFCReader->IrqState = 0;
}

void ISR_NFCEmulator() {
  pNFCEmulator->IrqState = 0;
}

void setup(void) {
  uint8_t UID[10], cbUID, SENS_RES[2], SEL_RES;

  Serial.begin(115200);
  SPI.begin();
  SPI.beginTransaction(SPISettings(PN532_SPI_SPEED, LSBFIRST, SPI_MODE0));  // we start it globally because we do not use SPI for other operations

  pNFCReader = new PN532_SPI(10, 3, ISR_NFCReader);
  pNFCEmulator = new PN532_SPI(9, 2, ISR_NFCEmulator);

  Serial.println();
  Serial.println("  .#####.         mimicard 0.1 (Arduino - single board)");
  Serial.println(" .## ^ ##.__ _    \"A La Vie, A L\'Amour\" - (oe.eo)");
  Serial.println(" ## / \\ /   ('>-");
  Serial.println(" ## \\ / | K  |    /*** Benjamin DELPY `gentilkiwi`");
  Serial.println(" '## v #\\____/         benjamin.delpy@banque-france.fr");
  Serial.println("  '#####' L\\_          Kiwi PN532                      ***/\r\n");

  Serial.print("| Verbosity: ");
  Serial.println(KPN532_VERBOSE ? "full" : "minimal");

  Serial.println("| Reader init...");
  SetupChip(pNFCReader);
  Serial.println("| Emulator init...");
  SetupChip(pNFCEmulator);
  Serial.println("| Initialization OK!");
}

void loop(void) {

  uint8_t *pResult, cbResult, bSuccess;
  uint8_t UID[10], cbUID, SENS_RES[2], SEL_RES;

  Serial.println("~ Waiting for target...");
  if (pNFCReader->InListPassiveTarget(UID, sizeof(UID), &cbUID, SENS_RES, &SEL_RES)) {
    Serial.print("| Detected target\r\n|   SENS_RES: ");
    PrintHex(SENS_RES, sizeof(SENS_RES));
    Serial.print("|   SEL_RES : ");
    PrintHex(&SEL_RES, 1);
    Serial.print("|   UID     : ");
    PrintHex(UID, cbUID);

    Serial.println("~ Waiting for reader on emulator...");
    if (pNFCEmulator->TgInitAsTarget(UID, cbUID, SENS_RES, SEL_RES)) {
      Serial.println("| Reader detected!");

      do {
        bSuccess = 0;

        if (pNFCEmulator->TgGetData(&pResult, &cbResult)) {
#if KPN532_VERBOSE
          Serial.print("< ");
          PrintHex(pResult, cbResult);
#endif
          if (pNFCReader->InDataExchange(pResult, cbResult, &pResult, &cbResult)) {
#if KPN532_VERBOSE
            Serial.print("> ");
            PrintHex(pResult, cbResult);
#endif
            bSuccess = pNFCEmulator->TgSetData(pResult, cbResult);
          }
        }
      } while (bSuccess);

      if (pNFCReader->InRelease()) {
        Serial.println("| Target released");
      }
    }
  }
}

void SetupChip(PN532_SPI *pNFC) {
  uint8_t IC, Ver, Rev, Support;

  pNFC->begin();

  if (!pNFC->RfConfiguration(0xff)) {
    Serial.println("Bad RfConfiguration :(");
    while (1)
      ;
  }

  if (!pNFC->SAMConfiguration()) {
    Serial.println("Bad SAMConfiguration :(");
    while (1)
      ;
  }

  if (pNFC->GetFirmwareVersion(&IC, &Ver, &Rev, &Support)) {
    Serial.print("|   PN5");
    Serial.print(IC, HEX);
    Serial.print(" version ");
    Serial.print(Ver, DEC);
    Serial.print(".");
    Serial.print(Rev, DEC);
    Serial.print(".");
    Serial.println(Support, DEC);
  }
}

void PrintHex(const byte *pcbData, const uint32_t cbData) {
  uint32_t i;

  for (i = 0; i < cbData; i++) {
    if (pcbData[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(pcbData[i] & 0xff, HEX);
  }
  Serial.println();
}