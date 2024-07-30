/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#include "kpn532_spi.h"
const uint8_t PN532_ACK[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };
const uint8_t PN532_NACK[] = { 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };

#define PACKET_DATA_IN (this->Buffer + 7)
#define PACKET_DATA_OUT (this->Buffer + 8)

PN532_SPI::PN532_SPI(const uint8_t ss_pin, const uint8_t irq_pin, PISR_PN532_SPI_ROUTINE Routine)
  : _ss(ss_pin), _irq(irq_pin), IrqState(HIGH) {
  pinMode(_ss, OUTPUT);
  digitalWrite(this->_ss, HIGH);

  pinMode(this->_irq, INPUT);

  attachInterrupt(digitalPinToInterrupt(this->_irq), Routine, FALLING);
}

PN532_SPI::~PN532_SPI() {
  detachInterrupt(digitalPinToInterrupt(this->_irq));
}

void PN532_SPI::begin() {
  uint8_t status;
#if (KPN532_OUTPUT_LEVEL >= KPN532_OUTPUT_LEVEL_INFO)
  uint8_t IC, Ver, Rev, Support;
#endif

  while (true) {
    digitalWrite(this->_ss, LOW);
    delay(PN532_T_osc_start);
    SPI.transfer(PN532_SPI_Status_Reading);
    status = SPI.transfer(0x42);
    digitalWrite(this->_ss, HIGH);

    if ((status != 0xff) && (status != 0xaa)) {  // seen, 0xff (init ?), 0xaa (default char ?), 0x08 (?), last bit - & 0x01 (ready or not ready)
      if (status & 0x01) {
        memcpy(this->Buffer, PN532_ACK, sizeof(PN532_ACK));
        digitalWrite(this->_ss, LOW);
        SPI.transfer(this->Buffer, sizeof(PN532_ACK));
        digitalWrite(this->_ss, HIGH);
      } else {
        break;
      }
    }
    // TODO add delay interframe?
  }

  if (!SAMConfiguration(0x01)) {
#if (KPN532_OUTPUT_LEVEL >= KPN532_OUTPUT_LEVEL_ERROR)
    Serial.println("Bad SAMConfiguration :(");
#endif
    while (1)
      ;
  }

#if (KPN532_OUTPUT_LEVEL >= KPN532_OUTPUT_LEVEL_INFO)
  if (GetFirmwareVersion(&IC, &Ver, &Rev, &Support)) {
    Serial.print("|   PN5");
    Serial.print(IC, HEX);
    Serial.print(" version ");
    Serial.print(Ver, DEC);
    Serial.print(".");
    Serial.print(Rev, DEC);
    Serial.print(".");
    Serial.println(Support, DEC);
  }

  if (ReadRegister(PN53X_REG_CIU_Version, &Ver)) {
    Serial.print("|   CIU Version 0x");
    Serial.println(Ver, HEX);
  }
#endif
}

// Miscellaneous

uint8_t PN532_SPI::GetFirmwareVersion(uint8_t *pIC, uint8_t *pVer, uint8_t *pRev, uint8_t *pSupport) {
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = PN532_CMD_GetFirmwareVersion;
  this->cbData = 1;

  if (Information_Frame_Exchange() && (this->cbData == 4)) {

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

    ret = 1;
  }

  return ret;
}

uint8_t PN532_SPI::ReadRegister(uint16_t Register, uint8_t *pValue) {
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = PN532_CMD_ReadRegister;
  *(uint16_t *)(PACKET_DATA_IN + 1) = __bswap_16(Register);
  this->cbData = 3;

  if (Information_Frame_Exchange() && (this->cbData == 1)) {
    *pValue = PACKET_DATA_OUT[0];
    ret = 1;
  }

  return ret;
}

uint8_t PN532_SPI::WriteRegister(uint16_t Register, uint8_t Value) {
  PACKET_DATA_IN[0] = PN532_CMD_WriteRegister;
  *(uint16_t *)(PACKET_DATA_IN + 1) = __bswap_16(Register);
  PACKET_DATA_IN[3] = Value;
  this->cbData = 4;

  return Information_Frame_Exchange();
}

uint8_t PN532_SPI::WriteRegister(const PN53X_REGISTER_VALUE *pRV, uint8_t szData) {
  PACKET_DATA_IN[0] = PN532_CMD_WriteRegister;
  memcpy(PACKET_DATA_IN + 1, pRV, szData);
  this->cbData = 1 + szData;

  return Information_Frame_Exchange();
}

uint8_t PN532_SPI::SAMConfiguration(uint8_t IRQ) {
  PACKET_DATA_IN[0] = PN532_CMD_SAMConfiguration;
  PACKET_DATA_IN[1] = 0x01;
  PACKET_DATA_IN[2] = 0x00;
  PACKET_DATA_IN[3] = IRQ;
  this->cbData = 4;

  return Information_Frame_Exchange();
}

// RFcommunication

uint8_t PN532_SPI::RfConfiguration__RF_field(uint8_t ConfigurationData) {
  PACKET_DATA_IN[0] = PN532_CMD_RFConfiguration;
  PACKET_DATA_IN[1] = 0x01;
  PACKET_DATA_IN[2] = ConfigurationData;
  this->cbData = 3;

  return Information_Frame_Exchange();
}

uint8_t PN532_SPI::RfConfiguration__Various_timings(uint8_t fATR_RES_Timeout, uint8_t fRetryTimeout) {
  PACKET_DATA_IN[0] = PN532_CMD_RFConfiguration;
  PACKET_DATA_IN[1] = 0x02;
  PACKET_DATA_IN[2] = 0x00;  // RFU
  PACKET_DATA_IN[3] = fATR_RES_Timeout;
  PACKET_DATA_IN[4] = fRetryTimeout;
  this->cbData = 5;

  return Information_Frame_Exchange();
}

uint8_t PN532_SPI::RfConfiguration__MaxRtyCOM(uint8_t MaxRtyCOM) {
  PACKET_DATA_IN[0] = PN532_CMD_RFConfiguration;
  PACKET_DATA_IN[1] = 0x04;
  PACKET_DATA_IN[2] = MaxRtyCOM;
  this->cbData = 3;

  return Information_Frame_Exchange();
}

uint8_t PN532_SPI::RfConfiguration__MaxRetries(uint8_t MxRtyPassiveActivation) {
  PACKET_DATA_IN[0] = PN532_CMD_RFConfiguration;
  PACKET_DATA_IN[1] = 0x05;
  PACKET_DATA_IN[2] = 0xff;
  PACKET_DATA_IN[3] = 0x01;
  PACKET_DATA_IN[4] = MxRtyPassiveActivation;
  this->cbData = 5;

  return Information_Frame_Exchange();
}

// Initiator

uint8_t PN532_SPI::InListPassiveTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t *pcbUID, uint8_t SENS_RES[2], uint8_t *pSEL_RES)  // NFC-A, 106kbps, 1 target, ATS not here
{
  uint8_t ret = 0;

  PACKET_DATA_IN[0] = PN532_CMD_InListPassiveTarget;
  PACKET_DATA_IN[1] = 0x01;
  PACKET_DATA_IN[2] = 0x00;
  this->cbData = 3;

  if (Information_Frame_Exchange() && (this->cbData > 1) && (PACKET_DATA_OUT[0] == 0x01)) {

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

    ret = 1;
  }

  return ret;
}

uint8_t PN532_SPI::InDataExchange(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t **ppReceived, uint8_t *pcbReceived, uint8_t *pErrorCode) {
  uint8_t ret = 0, errorCode;

  PACKET_DATA_IN[0] = PN532_CMD_InDataExchange;
  PACKET_DATA_IN[1] = 0x01;
  memcpy(PACKET_DATA_IN + 2, pcbInData, cbInData);
  this->cbData = 2 + cbInData;

  if (ppReceived && pcbReceived) {
    if (Information_Frame_Exchange() && this->cbData) {
      errorCode = PACKET_DATA_OUT[0] & 0x3f;
      if (!errorCode) {
        *ppReceived = PACKET_DATA_OUT + 1;
        *pcbReceived = this->cbData - 1;
        ret = 1;
      } else if (pErrorCode) {
        *pErrorCode = errorCode;
      }
    }

    if (!ret) {
      *ppReceived = NULL;
      *pcbReceived = 0;
    }
  } else {
    ret = Information_Frame_Exchange(0x01);
  }

  return ret;
}

uint8_t PN532_SPI::InCommunicateThru(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t **ppReceived, uint8_t *pcbReceived, uint8_t *pErrorCode) {
  uint8_t ret = 0, errorCode;

  PACKET_DATA_IN[0] = PN532_CMD_InCommunicateThru;
  memcpy(PACKET_DATA_IN + 1, pcbInData, cbInData);
  this->cbData = 1 + cbInData;

  if (ppReceived && pcbReceived) {
    if (Information_Frame_Exchange() && this->cbData) {
      errorCode = PACKET_DATA_OUT[0] & 0x3f;
      if (!errorCode) {
        *ppReceived = PACKET_DATA_OUT + 1;
        *pcbReceived = this->cbData - 1;
        ret = 1;
      } else if (pErrorCode) {
        *pErrorCode = errorCode;
      }
    }

    if (!ret) {
      *ppReceived = NULL;
      *pcbReceived = 0;
    }
  } else {
    ret = Information_Frame_Exchange(0x01);
  }

  return ret;
}

uint8_t PN532_SPI::InRelease(uint8_t *pErrorCode) {
  uint8_t ret = 0, errorCode;

  PACKET_DATA_IN[0] = PN532_CMD_InRelease;
  PACKET_DATA_IN[1] = 0x00;
  this->cbData = 2;

  if (Information_Frame_Exchange() && this->cbData) {
    errorCode = PACKET_DATA_OUT[0] & 0x3f;
    if (!errorCode) {
      ret = 1;
    } else if (pErrorCode) {
      *pErrorCode = errorCode;
    }
  }

  return ret;
}

// Target

uint8_t PN532_SPI::TgInitAsTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t SENS_RES[2], uint8_t SEL_RES) {
  uint8_t ret = 0, mode;

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
    this->cbData = 39;

    if (Information_Frame_Exchange() && this->cbData) {
      mode = PACKET_DATA_OUT[0] & 0x7f;
      if ((mode & 0x78) == 0x08) {
        ret = 1;
      }
    }
  }

  return ret;
}

uint8_t PN532_SPI::TgGetData(uint8_t **ppReceived, uint8_t *pcbReceived, uint8_t *pErrorCode) {
  uint8_t ret = 0, errorCode;

  PACKET_DATA_IN[0] = PN532_CMD_TgGetData;
  this->cbData = 1;

  if (Information_Frame_Exchange() && this->cbData) {
    errorCode = PACKET_DATA_OUT[0] & 0x3f;
    if (!errorCode) {
      *ppReceived = PACKET_DATA_OUT + 1;
      *pcbReceived = this->cbData - 1;
      ret = 1;
    } else if (pErrorCode) {
      *pErrorCode = errorCode;
    }
  }

  if (!ret) {
    *ppReceived = NULL;
    *pcbReceived = 0;
  }

  return ret;
}

uint8_t PN532_SPI::TgSetData(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t *pErrorCode) {
  uint8_t ret = 0, errorCode;

  PACKET_DATA_IN[0] = PN532_CMD_TgSetData;
  memcpy(PACKET_DATA_IN + 1, pcbInData, cbInData);
  this->cbData = 1 + cbInData;

  if (Information_Frame_Exchange() && this->cbData) {
    errorCode = PACKET_DATA_OUT[0] & 0x3f;
    if (!errorCode) {
      ret = 1;
    } else if (pErrorCode) {
      *pErrorCode = errorCode;
    }
  }

  return ret;
}

uint8_t PN532_SPI::TgGetInitiatorCommand(uint8_t **ppReceived, uint8_t *pcbReceived, uint8_t *pErrorCode) {
  uint8_t ret = 0, errorCode;

  PACKET_DATA_IN[0] = PN532_CMD_TgGetInitiatorCommand;
  this->cbData = 1;

  if (Information_Frame_Exchange() && this->cbData) {
    errorCode = PACKET_DATA_OUT[0] & 0x3f;
    if (!errorCode) {
      *ppReceived = PACKET_DATA_OUT + 1;
      *pcbReceived = this->cbData - 1;
      ret = 1;
    } else if (pErrorCode) {
      *pErrorCode = errorCode;
    }
  }

  if (!ret) {
    *ppReceived = NULL;
    *pcbReceived = 0;
  }

  return ret;
}

uint8_t PN532_SPI::TgResponseToInitiator(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t *pErrorCode) {
  uint8_t ret = 0, errorCode;

  PACKET_DATA_IN[0] = PN532_CMD_TgRespondToInitiator;
  memcpy(PACKET_DATA_IN + 1, pcbInData, cbInData);
  this->cbData = 1 + cbInData;

  if (Information_Frame_Exchange() && this->cbData) {
    errorCode = PACKET_DATA_OUT[0] & 0x3f;
    if (!errorCode) {
      ret = 1;
    } else if (pErrorCode) {
      *pErrorCode = errorCode;
    }
  }

  return ret;
}

uint8_t PN532_SPI::Information_Frame_Exchange(uint8_t bNoAnswer) {
  uint8_t ret = 0, cmd = PACKET_DATA_IN[0] + 1;

  Information_Frame_Host_To_PN532();
  if (Wait_Ready_IRQ()) {
    if (Generic_Frame_PN532_To_Host() == PN532_ACK_FRAME) {
      if (bNoAnswer) {
        memcpy(this->Buffer, PN532_ACK, sizeof(PN532_ACK));
        digitalWrite(this->_ss, LOW);
        SPI.transfer(this->Buffer, sizeof(PN532_ACK));
        digitalWrite(this->_ss, HIGH);
        ret = 1;
      } else {
        if (Wait_Ready_IRQ()) {
          if (Generic_Frame_PN532_To_Host() == PN532_NORMAL_INFORMATION_FRAME) {
            if (cmd == PACKET_DATA_OUT[-1]) {
              this->cbData = PACKET_DATA_OUT[-4] - 2;
              ret = 1;
            }
          }
        }
      }
    }
  }
  return ret;
}

uint8_t PN532_SPI::Wait_Ready_IRQ() {
  while (this->IrqState != LOW)
    ;
  this->IrqState = HIGH;

  return 1;
}

void PN532_SPI::Information_Frame_Host_To_PN532() {
  uint8_t DCS = 0, i;

  if (this->cbData) {
    this->Buffer[0] = PN532_SPI_Data_Writing;

    this->Buffer[1] = 0x00;                     // PREAMBLE - Preamble
    this->Buffer[2] = 0x00;                     // START CODE - Start of Packet Code
    this->Buffer[3] = 0xff;                     // ...
    this->Buffer[4] = this->cbData + 1;         // LEN - Packet Length
    this->Buffer[5] = ~Buffer[4] + 1;           // LCS - Packet Length Checksum
    this->Buffer[6] = PN532_TFI_Host_to_PN532;  // TFI - Specific PN532 Frame Identifier
    for (i = 0; i < (this->cbData + 1); i++) {
      DCS += this->Buffer[6 + i];
    }
    this->Buffer[7 + this->cbData] = ~DCS + 1;
    this->Buffer[7 + this->cbData + 1] = 0x00;

    digitalWrite(_ss, LOW);
    SPI.transfer(this->Buffer, 7 + this->cbData + 1 + 1);
    digitalWrite(_ss, HIGH);

    this->IrqState = HIGH;
  }
}

PN532_FRAME_TYPE PN532_SPI::Generic_Frame_PN532_To_Host() {
  PN532_FRAME_TYPE ret = PN532_UNKNOWN;
  uint8_t DCS = 0, i, cbDataIn;

  this->Buffer[0] = PN532_SPI_Data_Reading;
  this->cbData = 7;

  digitalWrite(_ss, LOW);
  SPI.transfer(this->Buffer, this->cbData);  // we want the return value here :)
  cbDataIn = this->Buffer[4];                // often used

  if ((this->Buffer[0] == 0x01) || (this->Buffer[0] == 0xff) || (this->Buffer[0] == 0xaa))  // ? :')
  {
    if ((this->Buffer[1] == 0x00) && (this->Buffer[2] == 0x00) && (this->Buffer[3] == 0xff)) {
      if ((cbDataIn == 0x00) && (this->Buffer[5] == 0xff) && (this->Buffer[6] == 0x00)) {
        ret = PN532_ACK_FRAME;
      } else if ((cbDataIn == 0xff) && (this->Buffer[5] == 0x00) && (this->Buffer[6] == 0x00)) {
        ret = PN532_NACK_FRAME;
      } else if (this->Buffer[5] == (uint8_t)(~cbDataIn + 1)) {
        SPI.transfer(this->Buffer + 7, cbDataIn - 1 + 2);
        this->cbData += cbDataIn - 1 + 2;
        for (i = 0; i < cbDataIn; i++) {
          DCS += this->Buffer[6 + i];
        }

        if (this->Buffer[6 + cbDataIn] == (uint8_t)~DCS + 1) {
          if (this->Buffer[6] == PN532_TFI_PN532_to_Host) {
            ret = PN532_NORMAL_INFORMATION_FRAME;
          } else if (cbDataIn == 0x01) {
            ret = PN532_ERROR_FRAME;
          }
        }
      }
    }
  }

  digitalWrite(_ss, HIGH);

  return ret;
}

void PN532_SPI::InitGlobalSPI() {
  SPI.begin();
  SPI.beginTransaction(SPISettings(PN532_SPI_SPEED, LSBFIRST, SPI_MODE0));  // we start it globally because we do not use SPI for other operations
}

void PN532_SPI::PrintHex(const byte *pcbData, const size_t cbData) {
  size_t i;

  for (i = 0; i < cbData; i++) {
    if (pcbData[i] < 0x10) {
      Serial.print("0");
    }
    Serial.print(pcbData[i] & 0xff, HEX);
  }
  Serial.println();
}
