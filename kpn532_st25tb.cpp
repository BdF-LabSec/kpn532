/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#include "kpn532_st25tb.h"

ST25TB::ST25TB(const uint8_t ss_pin, const uint8_t irq_pin, PISR_PN532_ROUTINE Routine) : bIgnoreTiming(0x00) {
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
uint8_t ST25TB::Initiate(uint8_t *pui8ChipId, uint8_t bForce) {
  uint8_t ret = 0;
  uint8_t *pReceived, cbReceived;

  if (bForce) {
    pNFC->RfConfiguration__MaxRtyCOM(0xff);
  }

  if(!bIgnoreTiming) {
    pNFC->RfConfiguration__Various_timings(0x00, 0x09);  // ~20 ms - ST25TB_INITIATOR_TIMEOUT_INITIATE
  }

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
  } while (bForce && !ret);

  if (bForce) {
    pNFC->RfConfiguration__MaxRtyCOM(0x00);
  }

  return ret;
}

uint8_t ST25TB::Select(const uint8_t ui8ChipId) {
  uint8_t ret = 0, ST25TB_Initiator_CMD_Select_data[] = { ST25TB_CMD_SELECT, ui8ChipId };
  uint8_t *pReceived, cbReceived;

  if(!bIgnoreTiming) {
    pNFC->RfConfiguration__Various_timings(0x00, 0x08);  // ~10 ms - ST25TB_INITIATOR_TIMEOUT_SELECT
  }

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
  uint8_t ret = 0, *pReceived, cbReceived;

  if(!bIgnoreTiming) {
    pNFC->RfConfiguration__Various_timings(0x00, 0x07);  // ~5 ms - ST25TB_INITIATOR_TIMEOUT_GENERIC
  }

  if (pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Get_Uid_data, sizeof(ST25TB_Initiator_CMD_Get_Uid_data), &pReceived, &cbReceived)) {
    if (cbReceived == sizeof(uint64_t)) {
      memcpy(pui8Data, pReceived, sizeof(uint64_t));
      ret = 1;
    }
  }

  return ret;
}

const uint8_t ST25TB_Initiator_CMD_Completion_data[] = { ST25TB_CMD_COMPLETION };
uint8_t ST25TB::Completion() {
  uint8_t ret, errorCode;

  if(!bIgnoreTiming) {
    pNFC->RfConfiguration__Various_timings(0x00, 0x01);  // 100 µs
  }
  ret = pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Completion_data, sizeof(ST25TB_Initiator_CMD_Completion_data), NULL, NULL, &errorCode);
  ret = (ret == 0x00) && (errorCode == 0x01);  // timeout

  return ret;
}

const uint8_t ST25TB_Initiator_CMD_Reset_to_inventory_data[] = { ST25TB_CMD_RESET_TO_INVENTORY };
uint8_t ST25TB::Reset_to_Inventory() {
  uint8_t ret, errorCode;

  if(!bIgnoreTiming) {
    pNFC->RfConfiguration__Various_timings(0x00, 0x01);  // 100 µs
  }
  ret = pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Reset_to_inventory_data, sizeof(ST25TB_Initiator_CMD_Reset_to_inventory_data), NULL, NULL, &errorCode);
  ret = (ret == 0x00) && (errorCode == 0x01);  // timeout

  return ret;
}

uint8_t ST25TB::Read_Block(const uint8_t ui8BlockIdx, uint8_t pui8Data[4]) {
  uint8_t ret = 0, ST25TB_Initiator_CMD_Read_Block_data[] = { ST25TB_CMD_READ_BLOCK, ui8BlockIdx };
  uint8_t *pReceived, cbReceived;

  if(!bIgnoreTiming) {
    pNFC->RfConfiguration__Various_timings(0x00, 0x07);  // ~5 ms - ST25TB_INITIATOR_TIMEOUT_GENERIC
  }
  if (pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Read_Block_data, sizeof(ST25TB_Initiator_CMD_Read_Block_data), &pReceived, &cbReceived)) {
    if (cbReceived == sizeof(uint32_t)) {
      *(uint32_t *)pui8Data = *(uint32_t *)pReceived;
      ret = 1;
    }
  }

  return ret;
}

uint8_t ST25TB::Write_Block(const uint8_t ui8BlockIdx, const uint8_t pui8Data[4]) {
  uint8_t ret, errorCode, ST25TB_Initiator_CMD_Write_Block_data[2 + 4] = { ST25TB_CMD_WRITE_BLOCK, ui8BlockIdx };
  *(uint32_t *)(ST25TB_Initiator_CMD_Write_Block_data + 2) = *((uint32_t *)pui8Data);

  if(!bIgnoreTiming) {
    pNFC->RfConfiguration__Various_timings(0x00, 0x07);  // ~ ST25TB_INITIATOR_DELAY_WRITE_TIME_COUNTER / ST25TB_INITIATOR_DELAY_WRITE_TIME_EEPROM
  }
  ret = pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Write_Block_data, sizeof(ST25TB_Initiator_CMD_Write_Block_data), NULL, NULL, &errorCode);
  ret = (ret == 0x00) && (errorCode == 0x01);  // timeout

  return ret;
}

uint8_t ST25TB::Initiator_Initiate_Select_UID_C1_C2(uint8_t UID[8], uint8_t C1[4], uint8_t C2[4]) {
  uint8_t ret = 0;

  if (Initiate(NULL, 0x01)) {
    ret = Select();
    if (UID && ret) {
      ret = Get_Uid(UID);
    }
    if (C1 && ret) {
      ret = Read_Block(ST25TB_IDX_COUNTER1, C1);
    }
    if (C2 && ret) {
      ret = Read_Block(ST25TB_IDX_COUNTER2, C2);
    }
  }

  return ret;
}

uint8_t ST25TB::Write_Block_noflush_notimer(const uint8_t ui8BlockIdx, const uint8_t pui8Data[4]) {
  uint8_t ST25TB_Initiator_CMD_Write_Block_data[2 + 4] = { ST25TB_CMD_WRITE_BLOCK, ui8BlockIdx };
  *(uint32_t *)(ST25TB_Initiator_CMD_Write_Block_data + 2) = *((uint32_t *)pui8Data);
  return pNFC->InCommunicateThru(ST25TB_Initiator_CMD_Write_Block_data, sizeof(ST25TB_Initiator_CMD_Write_Block_data));
}

uint8_t ST25TB::Initiator_Initiate_Select_Read_Block(const uint8_t ui8BlockIdx, uint8_t pui8Data[4]) {
  uint8_t ret = 0;

  if (Initiate()) {
    if (Select()) {
      ret = Read_Block(ui8BlockIdx, pui8Data);
    }
  }

  return ret;
}