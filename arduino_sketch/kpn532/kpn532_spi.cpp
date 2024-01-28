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

const uint8_t PN532_ACK[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };
const uint8_t PN532_NACK[] = { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };

#define PACKET_DATA_IN (Buffer + 7)
#define PACKET_DATA_OUT (Buffer + 8)

PN532_SPI::PN532_SPI(const uint8_t ss_pin, const uint8_t irq_pin)
  : _ss(ss_pin), _irq(irq_pin), _bIsVerbose(false) {
  pinMode(_ss, OUTPUT);
  digitalWrite(_ss, HIGH);

  pinMode(_irq, INPUT);
}

PN532_SPI::~PN532_SPI() {
}

void PN532_SPI::begin() {
  uint8_t status;

  while (true) {
    digitalWrite(_ss, LOW);
    delay(PN532_T_osc_start);
    SPI.transfer(PN532_SPI_Status_Reading);
    status = SPI.transfer(0x42);
    digitalWrite(_ss, HIGH);

    if ((status != 0xff) && (status != 0xaa)) { // seen, 0xff (init ?), 0xaa (default char ?), 0x08 (?), last bit - & 0x01 (ready or not ready)
      if (status & 0x01) {
        memcpy(Buffer, PN532_ACK, sizeof(PN532_ACK));
        digitalWrite(_ss, LOW);
        SPI.transfer(Buffer, sizeof(PN532_ACK));
        digitalWrite(_ss, HIGH);
      } else {
        break;
      }
    }
    // TODO add delay interframe?
  }
}

void PN532_SPI::Information_Frame_Host_To_PN532() {
  uint8_t DCS = 0, i;

  if (cbData) {
    Buffer[0] = PN532_SPI_Data_Writing;

    Buffer[1] = 0x00;                     // PREAMBLE - Preamble
    Buffer[2] = 0x00;                     // START CODE - Start of Packet Code
    Buffer[3] = 0xff;                     // ...
    Buffer[4] = cbData + 1;               // LEN - Packet Length
    Buffer[5] = ~Buffer[4] + 1;         // LCS - Packet Length Checksum
    Buffer[6] = PN532_TFI_Host_to_PN532;  // TFI - Specific PN532 Frame Identifier
    for (i = 0; i < (cbData + 1); i++) {
      DCS += Buffer[6 + i];
    }
    Buffer[7 + cbData] = ~DCS + 1;
    Buffer[7 + cbData + 1] = 0x00;

    digitalWrite(_ss, LOW);
    SPI.transfer(Buffer, 7 + cbData + 1 + 1);
    digitalWrite(_ss, HIGH);
  }
}

PN532_FRAME_TYPE PN532_SPI::Generic_Frame_PN532_To_Host() {
  PN532_FRAME_TYPE ret = PN532_UNKNOWN;
  uint8_t DCS = 0, i;

  Buffer[0] = PN532_SPI_Data_Reading;
  cbData = 7;

  digitalWrite(_ss, LOW);
  SPI.transfer(Buffer, cbData);  // we want the return value here :)

  if ((Buffer[0] == 0x01) || (Buffer[0] == 0xff) || (Buffer[0] == 0xaa))  // ? :')
  {
    if ((Buffer[1] == 0x00) && (Buffer[2] == 0x00) && (Buffer[3] == 0xff)) {
      if ((Buffer[4] == 0x00) && (Buffer[5] == 0xff) && (Buffer[6] == 0x00)) {
        ret = PN532_ACK_FRAME;
      } else if ((Buffer[4] == 0xff) && (Buffer[5] == 0x00) && (Buffer[6] == 0x00)) {
        ret = PN532_NACK_FRAME;
      } else if (Buffer[5] == (uint8_t)(~Buffer[4] + 1)) {
        SPI.transfer(Buffer + 7, Buffer[4] - 1 + 2);
        cbData += Buffer[4] - 1 + 2;
        for (i = 0; i < Buffer[4]; i++) {
          DCS += Buffer[6 + i];
        }

        if (Buffer[6 + Buffer[4]] == (uint8_t)~DCS + 1) {
          if (Buffer[6] == PN532_TFI_PN532_to_Host) {
            ret = PN532_NORMAL_INFORMATION_FRAME;
          } else if (Buffer[4] == 0x01) {
            ret = PN532_ERROR_FRAME;
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
  while (digitalRead(_irq) == HIGH)  // not really real IRQ, but as we wait with nothing in // ...
    ;

  return 1;
}

uint8_t PN532_SPI::Information_Frame_Exchange() {
  uint8_t ret = 0, cmd = PACKET_DATA_IN[0] + 1;

  Information_Frame_Host_To_PN532();
  if (Wait_Ready()) {
    if (Generic_Frame_PN532_To_Host() == PN532_ACK_FRAME) {
      if (Wait_Ready()) {
        if (Generic_Frame_PN532_To_Host() == PN532_NORMAL_INFORMATION_FRAME) {
          if (cmd == PACKET_DATA_OUT[-1]) {
            cbData = PACKET_DATA_OUT[-4] - 2;
            ret = 1;
          }
        }
      }
    }
  }
  return ret;
}

uint8_t PN532_SPI::GetFirmwareVersion(uint8_t *pIC, uint8_t *pVer, uint8_t *pRev, uint8_t *pSupport) {
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = PN532_CMD_GetFirmwareVersion;
  cbData = 1;

  if (Information_Frame_Exchange() && (cbData == 4)) {
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

  PACKET_DATA_IN[0] = PN532_CMD_InListPassiveTarget;
  PACKET_DATA_IN[1] = 0x01;
  PACKET_DATA_IN[2] = 0x00;
  cbData = 3;

  if (Information_Frame_Exchange() && (cbData > 1) && (PACKET_DATA_OUT[0] == 0x01)) {
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

  PACKET_DATA_IN[0] = PN532_CMD_InRelease;
  PACKET_DATA_IN[1] = 0x00;
  cbData = 2;

  if (Information_Frame_Exchange() && cbData) {
    ret = !(PACKET_DATA_OUT[0] & 0x3f);
  }

  return ret;
}

uint8_t PN532_SPI::InDataExchange(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t **ppReceived, uint8_t *pcbReceived) {
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = PN532_CMD_InDataExchange;
  PACKET_DATA_IN[1] = 0x01;
  cbData = 2 + cbInData;

  if (_bIsVerbose) {
    Serial.print(">C ");
    PrintHex(pcbInData, cbInData);
  }

  memcpy(PACKET_DATA_IN + 2, pcbInData, cbInData);

  if (Information_Frame_Exchange() && cbData) {
    ret = !(PACKET_DATA_OUT[0] & 0x3f);
    if (ret) {
      if (_bIsVerbose) {
        Serial.print("<C ");
        PrintHex(PACKET_DATA_OUT + 1, cbData - 1);
      }

      *ppReceived = PACKET_DATA_OUT + 1;
      *pcbReceived = cbData - 1;
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
    PACKET_DATA_IN[0] = PN532_CMD_TgInitAsTarget;
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
    cbData = 39;

    if (Information_Frame_Exchange() && cbData) {
      if ((PACKET_DATA_OUT[0] & 0x78) == 0x08) {
        ret = 1;
      }


      if (_bIsVerbose) {
        Serial.print("We got a reader! : ");
        PrintHex(PACKET_DATA_OUT, cbData);
      }
    }
  }

  return ret;
}

uint8_t PN532_SPI::TgGetData(uint8_t **ppReceived, uint8_t *pcbReceived) {
  uint8_t errorCode, ret = 0;

  PACKET_DATA_IN[0] = PN532_CMD_TgGetData;
  cbData = 1;

  if (Information_Frame_Exchange() && cbData) {
    errorCode = PACKET_DATA_OUT[0] & 0x3f;
    ret = !errorCode;
    if (ret) {
      if (_bIsVerbose) {
        Serial.print("<R ");
        PrintHex(PACKET_DATA_OUT + 1, cbData - 1);
      }

      *ppReceived = PACKET_DATA_OUT + 1;
      *pcbReceived = cbData - 1;
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

uint8_t PN532_SPI::TgSetData(const uint8_t *pcbInData, const uint8_t cbInData) {
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = PN532_CMD_TgSetData;

  if (_bIsVerbose) {
    Serial.print(">R ");
    PrintHex(pcbInData, cbInData);
  }

  memcpy(PACKET_DATA_IN + 1, pcbInData, cbInData);
  cbData = 1 + cbInData;

  if (Information_Frame_Exchange() && cbData) {
    if(!(PACKET_DATA_OUT[0] & 0x3f))
    {
      ret = 1;
    }
  }

  return ret;
}

uint8_t PN532_SPI::RfConfiguration(uint8_t MxRtyPassiveActivation)  // MaxRetries
{
  PACKET_DATA_IN[0] = PN532_CMD_RFConfiguration;
  PACKET_DATA_IN[1] = 0x05;
  PACKET_DATA_IN[2] = 0xff;
  PACKET_DATA_IN[3] = 0x01;
  PACKET_DATA_IN[4] = MxRtyPassiveActivation;
  cbData = 5;

  return Information_Frame_Exchange();
}

uint8_t PN532_SPI::SAMConfiguration() {
  PACKET_DATA_IN[0] = PN532_CMD_SAMConfiguration;
  PACKET_DATA_IN[1] = 0x01;
  PACKET_DATA_IN[2] = 0x00;
  PACKET_DATA_IN[3] = 0x01;
  cbData = 4;

  return Information_Frame_Exchange();
}