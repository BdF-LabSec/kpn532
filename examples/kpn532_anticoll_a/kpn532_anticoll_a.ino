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

#define KPN532_USE_KSSD1309_CONSOLE 0  // or 1 to enable output to SSD1309 console

#if !KPN532_USE_KSSD1309_CONSOLE
#define OUTPUT_NEWLINE_SAME(o) Serial.println()
#else
#include <ssd1309.h>
#define SSD1309_CS 8
#define SSD1309_DC 4
#define SSD1309_RST 5
SSD1309 *pDisplay;

#include <console.h>
Console *pConsole;
#define OUTPUT_NEWLINE_SAME(o) pConsole->RestartCurrentLine(o)
#endif

Print *pOutput;

void ISR_NFCReader() {
  pNFCReader->IrqState = 0;
}

void setup() {
#if !KPN532_USE_KSSD1309_CONSOLE
  Serial.begin(115200);
  while (!Serial)
    ;
  PN532::InitGlobalSPI();  // this example will use SPI only for PN532(s)

  pOutput = &Serial;
#else
  SPI.begin();  // Using SSD1309 will make the SPI bus to be shared with PN532(s)

  pDisplay = new SSD1309(SSD1309_CS, SSD1309_DC, SSD1309_RST);
  pDisplay->begin();
  pConsole = new Console(pDisplay);
  pConsole->ClearScreen();
  pOutput = pConsole;
  PN532::_pOutput = pOutput;
#endif

  pOutput->println("-anticoll A-");

  pNFCReader = new PN532(KPN532_0_CS, KPN532_0_IRQ, ISR_NFCReader);

  pOutput->print("0|");
  pNFCReader->begin();
}

void loop() {
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
    if (pNFCReader->InRelease()) {
      pOutput->println("|Target released");
    }
  }

  while(1);
}
