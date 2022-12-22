/*	Benjamin DELPY `gentilkiwi`
	LabSec - DGSI DIT ARCOS
	benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
	Licence : https://creativecommons.org/licenses/by/4.0/
*/
#pragma GCC optimize("-Ofast")
#include "kpn532_spi.h"

void SetupChip(PN532_SPI *pNFC);
uint8_t IsVerbose();

PN532_SPI *pNFCReader, *pNFCEmulator;

void setup(void) {
  uint8_t bIsVerbose = 0;
  uint8_t UID[10], cbUID, SENS_RES[2], SEL_RES;

  Serial.begin(115200);
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(LSBFIRST);
  SPI.setClockDivider(SPI_CLOCK_DIV4);

  bIsVerbose = IsVerbose();

  pNFCReader = new PN532_SPI(10, 9);
  pNFCEmulator = new PN532_SPI(7, 6);

  Serial.println();
  Serial.println("  .#####.         mimicard 0.1 (Arduino - single board)");
  Serial.println(" .## ^ ##.__ _    \"A La Vie, A L\'Amour\" - (oe.eo)");
  Serial.println(" ## / \\ /   ('>-");
  Serial.println(" ## \\ / | K  |    /*** Benjamin DELPY `gentilkiwi`");
  Serial.println(" '## v #\\____/         benjamin.delpy@banque-france.fr");
  Serial.println("  '#####' L\\_          Kiwi PN532                      ***/\r\n");

  Serial.print("| Verbosity: ");
  Serial.println(bIsVerbose ? "full" : "minimal");

  pNFCReader->setVerbose(bIsVerbose);
  pNFCEmulator->setVerbose(bIsVerbose);

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
          if (pNFCReader->InDataExchange(pResult, cbResult, &pResult, &cbResult)) {
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

uint8_t IsVerbose() {
  pinMode(A4, OUTPUT);
  pinMode(A5, INPUT_PULLUP);
  digitalWrite(A4, LOW);

  return digitalRead(A5);
}

void SetupChip(PN532_SPI *pNFC) {
  uint8_t IC, Ver, Rev, Support;

  if (!pNFC->begin()) {
    Serial.println("Not detected :(");
    while (1)
      ;
  }

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