/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#ifndef _KPN532_ST25TB_H_INCLUDED
#define _KPN532_ST25TB_H_INCLUDED
#include <kpn532.h>

#define ST25TB_IDX_COUNTER1 0x05
#define ST25TB_IDX_COUNTER2 0x06
#define ST25TB_IDX_SYSTEM 0xff

#define ST25TB_CMD_INITIATE 0x06  // 0x00
#define ST25TB_CMD_PCALL16 0x06   // 0x04
#define ST25TB_CMD_SLOT_MARKER_MASK 0x06
#define ST25TB_CMD_READ_BLOCK 0x08
#define ST25TB_CMD_WRITE_BLOCK 0x09
#define ST25TB_CMD_AUTHENTICATE 0x0a  // SRIX4K - France Telecom proprietary anti-clone function - Authenticate(RND)
#define ST25TB_CMD_GET_UID 0x0b
#define ST25TB_CMD_RESET_TO_INVENTORY 0x0c
#define ST25TB_CMD_SELECT 0x0e
#define ST25TB_CMD_COMPLETION 0x0f

#define ST25TB_INITIATOR_TIMEOUT_INITIATE 20
#define ST25TB_INITIATOR_TIMEOUT_SELECT 10
#define ST25TB_INITIATOR_TIMEOUT_GENERIC 5
#define ST25TB_INITIATOR_DELAY_BEFORE_RETRY 250
#define ST25TB_INITIATOR_DELAY_WRITE_TIME_OTP 3
#define ST25TB_INITIATOR_DELAY_WRITE_TIME_EEPROM 5
#define ST25TB_INITIATOR_DELAY_WRITE_TIME_COUNTER 7

class ST25TB {
private:
  uint8_t ui8ChipId;

public:
  PN532 *pNFC;
  void begin();

  ST25TB(const uint8_t ss_pin, const uint8_t irq_pin, PISR_PN532_ROUTINE Routine);  // Only IRQ
  ~ST25TB();

  uint8_t Initiate(uint8_t *pui8ChipId = NULL, uint8_t force = 0x00);
  uint8_t Select(const uint8_t ui8ChipId);
  uint8_t Select();
  uint8_t Get_Uid(uint8_t pui8Data[8]);
  uint8_t Completion();
  uint8_t Reset_to_Inventory();
  uint8_t Read_Block(const uint8_t ui8BlockIdx, uint8_t pui8Data[4]);
  uint8_t Write_Block(const uint8_t ui8BlockIdx, const uint8_t pui8Data[4]);

  uint8_t Initiator_Initiate_Select_UID_C1_C2(uint8_t UID[8], uint8_t C1[4], uint8_t C2[4]);
  uint8_t Initiator_Initiate_Select_Read_Block(const uint8_t ui8BlockIdx, uint8_t pui8Data[4]);
  uint8_t Write_Block_noflush_notimer(const uint8_t ui8BlockIdx, const uint8_t pui8Data[4]);
};

#endif
