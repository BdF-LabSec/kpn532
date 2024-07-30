/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#pragma GCC optimize("-Ofast")
#include <kpn532.h>
#define KPN532_RELAY_VERBOSE 0  // or 1, be careful, verbose slow down the relay
#define KPN532_RELAY_RAW 0      // 0 = TgGetData, InDataExchange, TgSetData (WTX, slower, overall faster because ISO/DEP and no real deslect/hlt), 1 = TgGetInitiatorCommand, InCommunicateThru, TgResponseToInitiator (no WTX, faster, overall slower because of real deselect/hlt),

PN532 *pNFCReader, *pNFCEmulator;

void ISR_NFCReader() {
  pNFCReader->IrqState = 0;
}

void ISR_NFCEmulator() {
  pNFCEmulator->IrqState = 0;
}

void setup(void) {
  uint8_t UID[10], cbUID, SENS_RES[2], SEL_RES;

  Serial.begin(115200);
  PN532::InitGlobalSPI();

  pNFCReader = new PN532(10, 3, ISR_NFCReader);
  pNFCEmulator = new PN532(9, 2, ISR_NFCEmulator);

  Serial.println("\x1b[2J\x1b[3J\x1b[H");
  Serial.println("  .#####.         kirelay 0.2 (Arduino with kpn532 - single board)");
  Serial.println(" .## ^ ##.__ _    \"A La Vie, A L\'Amour\" - (oe.eo)");
  Serial.println(" ## / \\ /   ('>-");
  Serial.println(" ## \\ / | K  |    /*** Benjamin DELPY `gentilkiwi`");
  Serial.println(" '## v #\\____/         benjamin.delpy@banque-france.fr");
  Serial.println("  '#####' L\\_          benjamin@gentilkiwi.com         ***/");
  Serial.println();

  Serial.print("| Relay verbosity: ");
  Serial.println(KPN532_RELAY_VERBOSE ? "full" : "minimal");
  Serial.print("| Relay mode     : ");
  Serial.println(KPN532_RELAY_RAW ? "RAW" : "ISO/DEP");

  Serial.println("| PN532 #0 - Reader init...");
  pNFCReader->begin();
  Serial.println("| PN532 #1 - Emulator init...");
  pNFCEmulator->begin();
  Serial.println("| Initialization OK!");
}

#if KPN532_RELAY_RAW == 0
#define KPN532_RELAY_GET TgGetData
#define KPN532_RELAX_EXC InDataExchange
#define KPN532_RELAY_SET TgSetData
#else
#define KPN532_RELAY_GET TgGetInitiatorCommand
#define KPN532_RELAX_EXC InCommunicateThru
#define KPN532_RELAY_SET TgResponseToInitiator
#endif

void loop(void) {

  uint8_t *pResult, cbResult, bSuccess;
  uint8_t UID[10], cbUID, SENS_RES[2], SEL_RES;

  Serial.println("~ Waiting for target...");
  if (pNFCReader->InListPassiveTarget(UID, sizeof(UID), &cbUID, SENS_RES, &SEL_RES)) {
    Serial.print("| Detected target\r\n|   SENS_RES: ");
    PN532::PrintHex(SENS_RES, sizeof(SENS_RES));
    Serial.print("|   SEL_RES : ");
    PN532::PrintHex(&SEL_RES, 1);
    Serial.print("|   UID     : ");
    PN532::PrintHex(UID, cbUID);

    Serial.println("~ Waiting for reader on emulator...");
    if (pNFCEmulator->TgInitAsTarget(UID, cbUID, SENS_RES, SEL_RES)) {
      Serial.println("| Reader detected!");
      do {
        bSuccess = 0;

        if (pNFCEmulator->KPN532_RELAY_GET(&pResult, &cbResult)) {
#if KPN532_RELAY_VERBOSE
          Serial.print("< ");
          PN532::PrintHex(pResult, cbResult);
#endif
          if (pNFCReader->KPN532_RELAX_EXC(pResult, cbResult, &pResult, &cbResult)) {
#if KPN532_RELAY_VERBOSE
            Serial.print("> ");
            PN532::PrintHex(pResult, cbResult);
#endif
            bSuccess = pNFCEmulator->KPN532_RELAY_SET(pResult, cbResult);
          }
        }
      } while (bSuccess);

      if (pNFCReader->InRelease()) {
        Serial.println("| Target released");
      }
    }
  }
}