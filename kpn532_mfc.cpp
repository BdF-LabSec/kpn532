/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#include "kpn532_mfc.h"

MFC::MFC(const uint8_t ss_pin, const uint8_t irq_pin, PISR_PN532_ROUTINE Routine) {
  pNFC = new PN532(ss_pin, irq_pin, Routine);
}

MFC::~MFC() {
  delete pNFC;
}

void MFC::begin() {
  pNFC->begin();
}

uint8_t MFC::Select() {
    uint8_t ret = 0, cbUID;
    
    if (pNFC->InListPassiveTarget(UID, sizeof(UID), &cbUID, SENS_RES, &SEL_RES)) {
        
        if((cbUID == MIFARE_CLASSIC_UID_SIZE) && (SENS_RES[0] == 0x00) && (SENS_RES[1] == 0x04) && ((SEL_RES == 0x08) || (SEL_RES == 0x88)))
        {
            ret = 1;
        }
        else
        {
            pNFC->InRelease();
        }
    }
    
    return ret;
}

uint8_t MFC::Release() {
    return pNFC->InRelease();
}

uint8_t MFC::Authenticate(const uint8_t cmdAuthAB, const uint8_t blockId, const uint8_t key[MIFARE_CLASSIC_KEY_SIZE]) {
    uint8_t ret = 0, *pReceived, cbReceived, errorCode;
    
    MFC_BUFFER[0] = cmdAuthAB;
    MFC_BUFFER[1] = blockId;
    memcpy(MFC_BUFFER + 2, key, MIFARE_CLASSIC_KEY_SIZE);
    memcpy(MFC_BUFFER + 2 + MIFARE_CLASSIC_KEY_SIZE, UID, MIFARE_CLASSIC_UID_SIZE);
    
    if(pNFC->InDataExchange(MFC_BUFFER, 12, &pReceived, &cbReceived, &errorCode)) {
        if(errorCode == 0x00) {
            ret = 1;
        }
    }
   
    return ret;
}

uint8_t MFC::Read(const uint8_t blockId, uint8_t **ppReceived) {
    uint8_t ret = 0, cbReceived, errorCode;
    MFC_BUFFER[0] = MIFARE_CLASSIC_CMD_READ;
    MFC_BUFFER[1] = blockId;
    
    if(pNFC->InDataExchange(MFC_BUFFER, 2, ppReceived, &cbReceived, &errorCode))
    {
        if((errorCode == 0x00) && (cbReceived == MIFARE_CLASSIC_BLOCK_SIZE)) {
            ret = 1;
        }
    }
    
    return ret;
}

uint8_t MFC::Write(const uint8_t blockId, const uint8_t *pcbData) {
    uint8_t ret = 0, *pReceived, cbReceived, errorCode;
    MFC_BUFFER[0] = MIFARE_CLASSIC_CMD_WRITE;
    MFC_BUFFER[1] = blockId;
    memcpy(MFC_BUFFER + 2, pcbData, MIFARE_CLASSIC_BLOCK_SIZE);
    
    if(pNFC->InDataExchange(MFC_BUFFER, 2 + MIFARE_CLASSIC_BLOCK_SIZE, &pReceived, &cbReceived, &errorCode)) {
        if(errorCode == 0x00) {
            ret = 1;
        }
    }    
    
    return ret;
}

uint8_t MFC::Special_Gen1a_unlock() {
    return Special_Gen1a_generic(MIFARE_CLASSIC_GEN1A_OPERATION_UNLOCK);
}

//uint8_t MFC::Special_Gen1a_wipe() {
//    return Special_Gen1a_generic(MIFARE_CLASSIC_GEN1A_OPERATION_WIPE);
//}

const uint8_t HALTA_data[] = { MIFARE_CLASSIC_CMD_Halt, 0x00 };
const PN53X_REGISTER_VALUE Registers_A_DisableCRC[] = {
  { __builtin_bswap16(PN53X_REG_CIU_TxMode), 0x00 },
  { __builtin_bswap16(PN53X_REG_CIU_RxMode), 0x00 },
};

const PN53X_REGISTER_VALUE Registers_A_EnableCRC[] = {
  { __builtin_bswap16(PN53X_REG_CIU_TxMode), 0x80 },
  { __builtin_bswap16(PN53X_REG_CIU_RxMode), 0x80 },
};

uint8_t MFC::Special_Gen1a_generic(const uint8_t operation, const uint8_t bNoHalt) {
  uint8_t ret = 0, errorCode;

  if(!bNoHalt)
  {
      if (!pNFC->InCommunicateThru(HALTA_data, sizeof(HALTA_data), NULL, NULL, &errorCode) && (errorCode == 0x01)) {  // timeout
        ret = 1;
      }
  }
  else
  {
    ret = 1;
  }


  if (ret) {
    ret = 0;
    if (pNFC->WriteRegister(Registers_A_DisableCRC, sizeof(Registers_A_DisableCRC))) {

      if (Special_Gen1a_generic_instruction(MIFARE_CLASSIC_CMD_PERSONALIZE_UID_USAGE, 7)) {
        if (Special_Gen1a_generic_instruction(operation, 0)) {
          ret = 1;
        }
      }

      pNFC->WriteRegister(Registers_A_EnableCRC, sizeof(Registers_A_EnableCRC));
    }
  }
  return ret;
}

uint8_t MFC::Special_Gen1a_generic_instruction(const uint8_t Instruction, uint8_t SpecificBits) {
  uint8_t ret = 0, *pResult, cbResult, errorCode, validBits;

  SpecificBits &= 0x07;
  if (SpecificBits) {
    pNFC->WriteRegister(PN53X_REG_CIU_BitFraming, SpecificBits);
  }

  if (pNFC->InCommunicateThru(&Instruction, sizeof(Instruction), &pResult, &cbResult, &errorCode)) {
    if (!errorCode && (cbResult == 1)) {
      if ((*pResult & 0x0f) == MIFARE_CLASSIC_ACK) {
        if (pNFC->ReadRegister(PN53X_REG_CIU_Control, &validBits)) {
          validBits &= 0x07;
          if (validBits == 4) {
            ret = 1;
          }
        }
      }
    }
  }

  if (SpecificBits) {
    pNFC->WriteRegister(PN53X_REG_CIU_BitFraming, 0x00);
  }

  return ret;
}
