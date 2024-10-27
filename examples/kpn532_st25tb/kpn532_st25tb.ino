/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#pragma GCC optimize("-Ofast")
#include <kpn532_st25tb.h>

#define KPN532_ST25TB_VERBOSE 0

void ReadAndPrintBlock(const uint8_t no);

ST25TB *pST25TB;

void ISR_NFC0() {
  pST25TB->pNFC->IrqState = 0;
}

void setup(void) {
  Serial.begin(115200);
  PN532::InitGlobalSPI();

  pST25TB = new ST25TB(10, 3, ISR_NFC0);

  Serial.println("\x1b[2J\x1b[3J\x1b[H");
  Serial.println("  .#####.         st25tb 0.1 (Arduino with kpn532)");
  Serial.println(" .## ^ ##.__ _    \"A La Vie, A L\'Amour\" - (oe.eo)");
  Serial.println(" ## / \\ /   ('>-");
  Serial.println(" ## \\ / | K  |    /*** Benjamin DELPY `gentilkiwi`");
  Serial.println(" '## v #\\____/         benjamin.delpy@banque-france.fr");
  Serial.println("  '#####' L\\_          benjamin@gentilkiwi.com         ***/");
  Serial.println();

  Serial.print("| Verbosity: ");
  Serial.println(KPN532_ST25TB_VERBOSE ? "full" : "minimal");

  Serial.println("| PN532 #0 - Init for ST25TB...");
  pST25TB->begin();
  Serial.println("| Initialization OK!");
}

void loop(void) {
  uint8_t ui8ChipId, UID[8], i;

  pST25TB->pNFC->RfConfiguration__RF_field(0x01);
  delay(10);

  Serial.print("| Initiate: ... ");
  if (pST25TB->Initiate(&ui8ChipId, 0x01)) {
    Serial.println();
    Serial.print("    ChipId is 0x");
    Serial.println(ui8ChipId, HEX);

    Serial.println("| Select  : ... ");
    if (pST25TB->Select(ui8ChipId)) {
      Serial.print("| Get_Uid : ...");
      if (pST25TB->Get_Uid(UID)) {
        Serial.println();
        Serial.print("    UID is ");
        PN532::PrintHex(UID, sizeof(UID));
      } else {
        Serial.println("KO");
      }

      for (i = 0; i < 16; i++) {
        ReadAndPrintBlock(i);
      }
      ReadAndPrintBlock(0xff);

      Serial.print("| Completion: ... ");
      if(!pST25TB->Completion())
      {
        Serial.println("KO");
      }
    } else {
      Serial.println("KO");
    }
  } else {
    Serial.println("KO");
  }

  delay(10);
  pST25TB->pNFC->RfConfiguration__RF_field(0x00);

  Serial.println("END");

  while (1)
    ;
}

void ReadAndPrintBlock(const uint8_t no) {
  uint8_t data[4];

  Serial.print("| Block 0x");
  if (no < 0x10) {
    Serial.print("0");
  }
  Serial.print(no, HEX);
  Serial.print(": ");

  if (pST25TB->Read_Block(no, data)) {
    PN532::PrintHex(data, sizeof(data));
  } else {
    Serial.println("error!");
  }
}