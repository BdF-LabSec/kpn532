/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#pragma GCC optimize("-Ofast")
#include <kpn532.h>
#define KPN532_0_CS 10
#define KPN532_0_IRQ 3
#define KPN532_1_CS 9
#define KPN532_1_IRQ 2

#define KPN532_RELAY_VERBOSE 0  // or 1, be careful, verbose slow down the relay
#define KPN532_RELAY_RAW 0      // 0 = TgGetData, InDataExchange, TgSetData (WTX, slower, overall faster because ISO/DEP and no real deselect/hlt), 1 = TgGetInitiatorCommand, InCommunicateThru, TgResponseToInitiator (no WTX, faster, overall slower because of real deselect/hlt),

#if KPN532_RELAY_RAW == 0
#define KPN532_RELAY_GET TgGetData
#define KPN532_RELAX_EXC InDataExchange
#define KPN532_RELAY_SET TgSetData
#else
#define KPN532_RELAY_GET TgGetInitiatorCommand
#define KPN532_RELAX_EXC InCommunicateThru
#define KPN532_RELAY_SET TgResponseToInitiator
#endif

PN532 *pNFCReader, *pNFCEmulator;

#define KPN532_USE_KSSD1309_CONSOLE 0 // or 1 to enable output to SSD1309 console

#if !KPN532_USE_KSSD1309_CONSOLE
#define OUTPUT_NEWLINE_SAME() Serial.println()
#else
#include <ssd1309.h>
#define SSD1309_CS 8
#define SSD1309_DC 4
#define SSD1309_RST 5
SSD1309 *pDisplay;

#include <console.h>
Console *pConsole;
#define OUTPUT_NEWLINE_SAME() pConsole->RestartCurrentLine()
#endif

Print *pOutput;

void ISR_NFCReader() {
  pNFCReader->IrqState = 0;
}

void ISR_NFCEmulator() {
  pNFCEmulator->IrqState = 0;
}

void setup(void) {
  
#if !KPN532_USE_KSSD1309_CONSOLE
  Serial.begin(115200);
  PN532::InitGlobalSPI(); // this example will use SPI only for PN532(s)

  pOutput = &Serial;
#else
  SPI.begin(); // Using SSD1309 will make the SPI bus to be shared with PN532(s)

  pDisplay = new SSD1309(SSD1309_CS, SSD1309_DC, SSD1309_RST);
  pDisplay->begin();
  pConsole = new Console(pDisplay);
  pConsole->ClearScreen();
  pOutput = pConsole;
  PN532::_pOutput = pOutput;
#endif

  pOutput->println("- kpn532 relay -");
  pOutput->print("V:");
  pOutput->print(KPN532_RELAY_VERBOSE ? "full" : "min");
  pOutput->print(" M:");
  pOutput->println(KPN532_RELAY_RAW ? "RAW" : "ISO/DEP");

  pNFCReader = new PN532(KPN532_0_CS, KPN532_0_IRQ, ISR_NFCReader);
  pNFCEmulator = new PN532(KPN532_1_CS, KPN532_1_IRQ, ISR_NFCEmulator);

  pOutput->print("0|");
  pNFCReader->begin();
  pOutput->print("1|");
  pNFCEmulator->begin();
}

void loop(void) {

  uint8_t *pResult, cbResult, bSuccess;
  uint8_t UID[10], cbUID = 0, SENS_RES[2], SEL_RES;

  pOutput->print("~Waiting target~");
  if (pNFCReader->InListPassiveTarget(UID, sizeof(UID), &cbUID, SENS_RES, &SEL_RES)) {
    OUTPUT_NEWLINE_SAME();
    pOutput->print("SENS_RES: ");
    PN532::PrintHex(SENS_RES, sizeof(SENS_RES));
    pOutput->print("SEL_RES : ");
    PN532::PrintHex(&SEL_RES, sizeof(SEL_RES));
    pOutput->print(' ');
    PN532::PrintHex(UID, cbUID);

    pOutput->print("~Waiting reader~");
    if (pNFCEmulator->TgInitAsTarget(UID, cbUID, SENS_RES, SEL_RES)) {
      OUTPUT_NEWLINE_SAME();
      pOutput->println("|Reader detected");
      do {
        bSuccess = 0x00;

        if (pNFCEmulator->KPN532_RELAY_GET(&pResult, &cbResult)) {
#if KPN532_RELAY_VERBOSE
          pOutput->print("< ");
          PN532::PrintHex(pResult, cbResult);
#endif
          if (pNFCReader->KPN532_RELAX_EXC(pResult, cbResult, &pResult, &cbResult)) {
#if KPN532_RELAY_VERBOSE
            pOutput->print("> ");
            PN532::PrintHex(pResult, cbResult);
#endif
            bSuccess = pNFCEmulator->KPN532_RELAY_SET(pResult, cbResult);
          }
        }
      } while (bSuccess);

      if (pNFCReader->InRelease()) {
        pOutput->println("|Target released");
      }
    }
  }
}