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

void ISR_NFC0() {
  pST25TB->pNFC->IrqState = 0;
}

void ReadAndPrintBlock(const uint8_t no);

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

  pOutput->println("- kpn532 st25tb-");

  pST25TB = new ST25TB(KPN532_0_CS, KPN532_0_IRQ, ISR_NFC0);

  pOutput->print("0|");
  pST25TB->begin();
}

void loop(void) {
  uint8_t ui8ChipId, UID[8], i;

  pST25TB->pNFC->RfConfiguration__RF_field(0x01);
  delay(10);

  pOutput->print("| Initiate...");
  if (pST25TB->Initiate(&ui8ChipId, 0x01)) {
    OUTPUT_NEWLINE_SAME();
    pOutput->print("ChipId: ");
    pOutput->println(ui8ChipId, HEX);

    pOutput->print("| Select...");
    if (pST25TB->Select(ui8ChipId)) {
      OUTPUT_NEWLINE_SAME();
      pOutput->print("| Get_Uid...");
      if (pST25TB->Get_Uid(UID)) {
        OUTPUT_NEWLINE_SAME();
        PN532::PrintHex(UID, sizeof(UID), PN532_PRINTHEX_REV);
      } else {
        pOutput->println("KO");
      }

      ReadAndPrintBlock(0x05);
      ReadAndPrintBlock(0x06);

      pOutput->print("| Completion...");
      if(pST25TB->Completion()) {
        OUTPUT_NEWLINE_SAME();
      } else {
        pOutput->println("KO");
      }
    } else {
      pOutput->println("KO");
    }
  } else {
    pOutput->println("KO");
  }

  delay(5);
  pST25TB->pNFC->RfConfiguration__RF_field(0x00);

  pOutput->println("END");

  while (1)
    ;
}

void ReadAndPrintBlock(const uint8_t no) {
  uint8_t data[4];

  if (no < 0x10) {
    pOutput->print('0');
  }
  pOutput->print(no, HEX);
  pOutput->print('|');

  if (pST25TB->Read_Block(no, data)) {
    PN532::PrintHex(data, sizeof(data), PN532_PRINTHEX_REV);
  } else {
    pOutput->println("error!");
  }
}