/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#include "kpn532_st25tb.h"

ST25TB::ST25TB(const uint8_t ss_pin, const uint8_t irq_pin, PISR_PN532_ROUTINE Routine) {
  pNFC = new PN532(ss_pin, irq_pin, Routine);
}

ST25TB::~ST25TB() {
  delete pNFC;
}

const PN53X_REGISTER_VALUE Registers_B_SR_ST25TB[] = {
  { __builtin_bswap16(PN53X_REG_CIU_Control), 0x10 },  // 0x10 = Initiator
  { __builtin_bswap16(PN53X_REG_CIU_TxMode), 0x83 },   // 0x80 = TxCRCEn, 106 kbit/s, 0x03 = ISO/IEC 14443B
  { __builtin_bswap16(PN53X_REG_CIU_RxMode), 0x83 },   // 0x80 = RxCRCEn, 106 kbit/s, 0x03 = ISO/IEC 14443B
  { __builtin_bswap16(PN53X_REG_CIU_CWGsP), 0x3f },    // conductance of the output P-driver during times of no modulation = 0x3f - MAX
  { __builtin_bswap16(PN53X_REG_CIU_ModGsP), 0x12 },   // conductance of the output P-driver for the time of modulation = 0x12 - (0x3f & 0x12 ~= MOD_ASK_10)
};

void ST25TB::begin() {
  pNFC->begin();
  pNFC->WriteRegister(Registers_B_SR_ST25TB, sizeof(Registers_B_SR_ST25TB));
}

const uint8_t ST25TB_Initiator_CMD_Initiate_data[] = { ST25TB_CMD_INITIATE, 0x00 };
uint8_t ST25TB::Initiate(uint8_t *pui8ChipId, uint8_t force) {
  uint8_t ret = 0;
  uint8_t *pReceived, cbReceived;

  if (force) {
    pNFC->RfConfiguration__MaxRtyCOM(0xff);
  }

  pNFC->RfConfiguration__Various_timings(0x00, 0x09);  // ~20 ms
  
  do {
    if (pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Initiate_data, sizeof(ST25TB_Initiator_CMD_Initiate_data), &pReceived, &cbReceived)) {
      if (cbReceived == 1) {
        ui8ChipId = *pReceived;
        if (pui8ChipId) {
          *pui8ChipId = *pReceived;
        }
        ret = 1;
      }
    }
  } while (force && !ret);

  if (force) {
    pNFC->RfConfiguration__MaxRtyCOM(0x00);
  }

  return ret;
}

uint8_t ST25TB::Select(const uint8_t ui8ChipId) {
  uint8_t ret = 0, ST25TB_Initiator_CMD_Select_data[] = { ST25TB_CMD_SELECT, ui8ChipId };
  uint8_t *pReceived, cbReceived;

  pNFC->RfConfiguration__Various_timings(0x00, 0x08);  // ~10 ms

  if (pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Select_data, sizeof(ST25TB_Initiator_CMD_Select_data), &pReceived, &cbReceived)) {
    if (cbReceived == 1) {
      if (ui8ChipId == *pReceived) {
        ret = 1;
      }
    }
  }

  return ret;
}

uint8_t ST25TB::Select() {
  return Select(ui8ChipId);
}

const uint8_t ST25TB_Initiator_CMD_Get_Uid_data[] = { ST25TB_CMD_GET_UID };
uint8_t ST25TB::Get_Uid(uint8_t pui8Data[8]) {
  uint8_t ret = 0;
  uint8_t *pReceived, cbReceived;

  pNFC->RfConfiguration__Various_timings(0x00, 0x07);  // ~5 ms

  if (pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Get_Uid_data, sizeof(ST25TB_Initiator_CMD_Get_Uid_data), &pReceived, &cbReceived)) {
    if (cbReceived == sizeof(uint64_t)) {
      *(uint64_t *)pui8Data = *(uint64_t *)pReceived;
      ret = 1;
    }
  }

  return ret;
}

const uint8_t ST25TB_Initiator_CMD_Completion_data[] = { ST25TB_CMD_COMPLETION };
uint8_t ST25TB::Completion() {
  return pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Completion_data, sizeof(ST25TB_Initiator_CMD_Completion_data));
}

uint8_t ST25TB::Read_Block(const uint8_t ui8BlockIdx, uint8_t pui8Data[4]) {
  uint8_t ret = 0, ST25TB_Initiator_CMD_Read_Block_data[] = { ST25TB_CMD_READ_BLOCK, ui8BlockIdx };
  uint8_t *pReceived, cbReceived;

  pNFC->RfConfiguration__Various_timings(0x00, 0x07);  // ~5 ms

  if (pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Read_Block_data, sizeof(ST25TB_Initiator_CMD_Read_Block_data), &pReceived, &cbReceived)) {
    if (cbReceived == sizeof(uint32_t)) {
      *(uint32_t *)pui8Data = *(uint32_t *)pReceived;
      ret = 1;
    }
  }

  return ret;
}