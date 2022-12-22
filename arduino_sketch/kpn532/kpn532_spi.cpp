/*	Benjamin DELPY `gentilkiwi`
	LabSec - DGSI DIT ARCOS
	benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
	Licence : https://creativecommons.org/licenses/by/4.0/
*/
#include "kpn532_spi.h"

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

// PN532_SPI::PN532_SPI(const uint8_t ss_pin)
//   : _ss(ss_pin), _irq(0xff), _timeout_loops(25500 / 100), _bIsVerbose(false) {
//   pinMode(_ss, OUTPUT);
//   digitalWrite(_ss, HIGH);
// }

PN532_SPI::PN532_SPI(const uint8_t ss_pin, const uint8_t irq_pin)
  : _ss(ss_pin), _irq(irq_pin), _bIsVerbose(false) {
  pinMode(_ss, OUTPUT);
  digitalWrite(_ss, HIGH);

  pinMode(_irq, INPUT);
}

PN532_SPI::~PN532_SPI() {
}

const uint8_t PN532_ACK[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };
const uint8_t PN532_NACK[] = { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };

uint8_t PN532_SPI::begin() {
  uint8_t ret = 0, status;  //, i;

  while (true) /*for (i = 0; i < _timeout_loops; i++)*/ {
    status = Status_Frame();

    if ((status != 0xff) && (status != 0xaa)) {
      if (status & 0x01) {
        Generic_Frame_Host_To_PN532(PN532_ACK, sizeof(PN532_ACK));
        delayMicroseconds(PN532_INTERFRAME_DELAY_US);
      } else {
        ret = 1;

        break;
      }
    } else {
      delayMicroseconds(PN532_INTERFRAME_DELAY_US);
    }
  }

  return ret;
}

void PN532_SPI::Information_Frame_Host_To_PN532(const uint8_t cbData) {
  uint8_t DCS, i;

  if (cbData) {
    pbBuffer[0] = 0x01;

    pbBuffer[1] = 0x00;              // PREAMBLE - Preamble
    pbBuffer[2] = 0x00;              // START CODE - Start of Packet Code
    pbBuffer[3] = 0xff;              // ...
    pbBuffer[4] = cbData + 1;        // LEN - Packet Length
    pbBuffer[5] = ~pbBuffer[4] + 1;  // LCS - Packet Length Checksum
    pbBuffer[6] = 0xd4;              // TFI - Specific PN532 Frame Identifier

    DCS = pbBuffer[6];
    for (i = 0; i < cbData; i++) {
      DCS += pbBuffer[7 + i];
    }

    pbBuffer[7 + cbData] = ~DCS + 1;
    pbBuffer[7 + cbData + 1] = 0x00;

    cbBuffer = 7 + cbData + 1 + 1;
    Generic_Frame_Host_To_PN532(pbBuffer, cbBuffer);
  }
}

void PN532_SPI::Generic_Frame_Host_To_PN532(const uint8_t *pbData, const uint8_t cbData) {
  uint8_t i;

  digitalWrite(_ss, LOW);
  for (i = 0; i < cbData; i++) {
    SPI.transfer(pbData[i]);
  }
  digitalWrite(_ss, HIGH);
}

uint8_t PN532_SPI::Generic_Frame_PN532_To_Host() {
  uint8_t ret = 0;
  uint8_t DCS, i;

  pbBuffer[0] = 0x03;
  cbBuffer = 7;

  digitalWrite(_ss, LOW);

  for (i = 0; i < cbBuffer; i++) {
    pbBuffer[i] = SPI.transfer(pbBuffer[i]);
  }

  if ((pbBuffer[0] == 0x01) || (pbBuffer[0] == 0xff) || (pbBuffer[0] == 0xaa))  // ? :')
  {
    if ((pbBuffer[1] == 0x00) && (pbBuffer[2] == 0x00) && (pbBuffer[3] == 0xff)) {
      if ((pbBuffer[4] == 0x00) && (pbBuffer[5] == 0xff) && (pbBuffer[6] == 0x00)) {
        ret = 3;  // 6.2.1.3 ACK frame
      } else if ((pbBuffer[4] == 0xff) && (pbBuffer[5] == 0x00) && (pbBuffer[6] == 0x00)) {
        ret = 4;  // 6.2.1.4 NACK frame
      } else if (pbBuffer[5] == (uint8_t)(~pbBuffer[4] + 1)) {
        DCS = pbBuffer[6];
        for (i = 1; i < pbBuffer[4]; i++)  // TFI already here, + DCS & 0x00
        {
          pbBuffer[6 + i] = SPI.transfer(0x42);
          DCS += pbBuffer[6 + i];
        }

        pbBuffer[6 + pbBuffer[4]] = SPI.transfer(0x42);
        pbBuffer[6 + pbBuffer[4] + 1] = SPI.transfer(0x42);
        cbBuffer += pbBuffer[4] - 1 + 2;

        if (pbBuffer[6 + pbBuffer[4]] == (uint8_t)~DCS + 1) {
          if (pbBuffer[6] == 0xd5) {
            ret = 1;  // 6.2.1.1 Normal information frame
          } else if (pbBuffer[4] == 0x01) {
            ret = 5;  // 6.2.1.5 Error frame
          }
        }
      }
    }
  }

  digitalWrite(_ss, HIGH);

  return ret;
}

uint8_t PN532_SPI::Wait_Ready() {
  return /*(_irq != 0xff) ? */ Wait_Ready_IRQ() /* : Wait_Ready_Status()*/;
}

uint8_t PN532_SPI::Status_Frame()  // seen, 0xff (init ?), 0xaa (default char ?), 0x08 (?), last bit - & 0x01 (ready or not ready)
{
  uint8_t ret;

  digitalWrite(_ss, LOW);
  SPI.transfer(0x02);
  ret = SPI.transfer(0x42);
  digitalWrite(_ss, HIGH);

  return ret;
}

// uint8_t PN532_SPI::Wait_Ready_Status() {
//   uint8_t status = 0, i;

//   for (i = 0; i < _timeout_loops; i++) {
//     if (Status_Frame() & 0x01) {
//       status = 1;
//       break;
//     } else {
//       delayMicroseconds(PN532_INTERFRAME_DELAY_US);
//     }
//   }

//   return status;
// }

uint8_t PN532_SPI::Wait_Ready_IRQ() {
  while (digitalRead(_irq)) // not really real IRQ, but as we wait with nothing in // ...
    ;

  return 1;
}

uint8_t PN532_SPI::Information_Frame_Exchange(const uint8_t cbData) {
  uint8_t ret = 0, cmd = pbBuffer[7] + 1;

  cbResult = 0;

  Information_Frame_Host_To_PN532(cbData);
  if (Wait_Ready()) {
    if (Generic_Frame_PN532_To_Host() == 3) {
      if (Wait_Ready()) {
        if (Generic_Frame_PN532_To_Host() == 1) {
          if (cmd == pbBuffer[7]) {
            ret = 1;
            cbResult = pbBuffer[4] - 2;
          }
        }
      }
    }
  }
  return ret;
}

uint8_t PN532_SPI::GetFirmwareVersion(uint8_t *pIC, uint8_t *pVer, uint8_t *pRev, uint8_t *pSupport) {
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = 0x02;

  if (Information_Frame_Exchange(1) && (cbResult == 4)) {
    ret = 1;

    if (pIC) {
      *pIC = PACKET_DATA_OUT[0];
    }

    if (pVer) {
      *pVer = PACKET_DATA_OUT[1];
    }

    if (pRev) {
      *pRev = PACKET_DATA_OUT[2];
    }

    if (pSupport) {
      *pSupport = PACKET_DATA_OUT[3];
    }
  }

  return ret;
}

uint8_t PN532_SPI::InListPassiveTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t *pcbUID, uint8_t SENS_RES[2], uint8_t *pSEL_RES)  // NFC-A, 106kbps, 1 target, ATS not here
{
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = 0x4a;
  PACKET_DATA_IN[1] = 0x01;
  PACKET_DATA_IN[2] = 0x00;

  if (Information_Frame_Exchange(3) && (cbResult > 1) && (PACKET_DATA_OUT[0] == 0x01)) {
    ret = 1;

    if (SENS_RES) {
      *(uint16_t *)SENS_RES = *(uint16_t *)(PACKET_DATA_OUT + 2);
    }

    if (pSEL_RES) {
      *pSEL_RES = PACKET_DATA_OUT[4];
    }

    if (pbUID && pcbUID) {
      if (cbUID >= PACKET_DATA_OUT[5]) {
        memcpy(pbUID, PACKET_DATA_OUT + 6, PACKET_DATA_OUT[5]);
        *pcbUID = PACKET_DATA_OUT[5];
      }
    }
  }

  return ret;
}

uint8_t PN532_SPI::InRelease() {
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = 0x52;
  PACKET_DATA_IN[1] = 0x00;

  if (Information_Frame_Exchange(2) && cbResult) {
    ret = !(PACKET_DATA_OUT[0] & 0x3f);
  }

  return ret;
}

uint8_t PN532_SPI::InDataExchange(const uint8_t *pbData, const uint8_t cbData, uint8_t **ppReceived, uint8_t *pcbReceived) {
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = 0x40;
  PACKET_DATA_IN[1] = 0x01;

  if (_bIsVerbose) {
    Serial.print(">C ");
    PrintHex(pbData, cbData);
  }

  memcpy(PACKET_DATA_IN + 2, pbData, cbData);

  if (Information_Frame_Exchange(2 + cbData) && cbResult) {
    ret = !(PACKET_DATA_OUT[0] & 0x3f);
    if (ret) {
      if (_bIsVerbose) {
        Serial.print("<C ");
        PrintHex(PACKET_DATA_OUT + 1, cbResult - 1);
      }

      *ppReceived = PACKET_DATA_OUT + 1;
      *pcbReceived = cbResult - 1;
    }
  }

  if (!ret) {
    *ppReceived = NULL;
    *pcbReceived = 0;
  }

  return ret;
}

uint8_t PN532_SPI::TgInitAsTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t SENS_RES[2], uint8_t SEL_RES) {
  uint8_t ret = 0;

  if (cbUID >= 4) {
    PACKET_DATA_IN[0] = 0x8c;
    PACKET_DATA_IN[1] = 0x05;  // PICC only & passive

    PACKET_DATA_IN[2] = SENS_RES[1];
    PACKET_DATA_IN[3] = SENS_RES[0];
    PACKET_DATA_IN[4] = pbUID[1];
    PACKET_DATA_IN[5] = pbUID[2];
    PACKET_DATA_IN[6] = pbUID[3];
    PACKET_DATA_IN[7] = SEL_RES;

    memset(PACKET_DATA_IN + 8, 0, 29);

    PACKET_DATA_IN[37] = 0x01;
    PACKET_DATA_IN[38] = 0x80;

    if (Information_Frame_Exchange(39) && cbResult) {
      if ((PACKET_DATA_OUT[0] & 0x78) == 0x08) {
        ret = 1;
      }


      if (_bIsVerbose) {
        Serial.print("We got a reader! : ");
        PrintHex(PACKET_DATA_OUT, cbResult);
      }
    }
  }

  return ret;
}

uint8_t PN532_SPI::TgGetData(uint8_t **ppReceived, uint8_t *pcbReceived) {
  uint8_t errorCode, ret = 0;

  PACKET_DATA_IN[0] = 0x86;

  if (Information_Frame_Exchange(1) && cbResult) {
    errorCode = PACKET_DATA_OUT[0] & 0x3f;
    ret = !errorCode;
    if (ret) {
      if (_bIsVerbose) {
        Serial.print("<R ");
        PrintHex(PACKET_DATA_OUT + 1, cbResult - 1);
      }

      *ppReceived = PACKET_DATA_OUT + 1;
      *pcbReceived = cbResult - 1;
    } else {
      if (_bIsVerbose) {
        Serial.print("<R Status code: 0x");
        Serial.println(errorCode, HEX);
      }
    }
  }

  if (!ret) {
    *ppReceived = NULL;
    *pcbReceived = 0;
  }

  return ret;
}

uint8_t PN532_SPI::TgSetData(const uint8_t *pbData, const uint8_t cbData) {
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = 0x8e;

  if (_bIsVerbose) {
    Serial.print(">R ");
    PrintHex(pbData, cbData);
  }

  memcpy(PACKET_DATA_IN + 1, pbData, cbData);

  if (Information_Frame_Exchange(1 + cbData) && cbResult) {
    ret = !(PACKET_DATA_OUT[0] & 0x3f);
  }

  return ret;
}

uint8_t PN532_SPI::RfConfiguration(uint8_t MxRtyPassiveActivation)  // MaxRetries
{
  PACKET_DATA_IN[0] = 0x32;
  PACKET_DATA_IN[1] = 0x05;
  PACKET_DATA_IN[2] = 0xff;
  PACKET_DATA_IN[3] = 0x01;
  PACKET_DATA_IN[4] = MxRtyPassiveActivation;

  return Information_Frame_Exchange(5);
}

uint8_t PN532_SPI::SAMConfiguration() {
  PACKET_DATA_IN[0] = 0x14;
  PACKET_DATA_IN[1] = 0x01;
  PACKET_DATA_IN[2] = 0x00;
  PACKET_DATA_IN[3] = (_irq != 0xff) ? 0x01 : 0x00;

  return Information_Frame_Exchange(4);
}