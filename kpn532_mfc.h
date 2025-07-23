/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#ifndef _KPN532_MFC_H_INCLUDED
#define _KPN532_MFC_H_INCLUDED
#include <kpn532.h>

#define MIFARE_CLASSIC_KEY_SIZE						6
#define MIFARE_CLASSIC_SECTORS						16
#define MIFARE_CLASSIC_BLOCKS_PER_SECTOR			4
#define MIFARE_CLASSIC_BLOCK_SIZE					16
#define MIFARE_CLASSIC_UID_SIZE						4	// ok, I know about 7 or 10 too...

#define MIFARE_CLASSIC_ACL_NONE						0x00
#define MIFARE_CLASSIC_ACL_KEY_A					0x01
#define MIFARE_CLASSIC_ACL_KEY_B					0x02
#define MIFARE_CLASSIC_ACL_KEY_AB					MIFARE_CLASSIC_ACL_KEY_A | MIFARE_CLASSIC_ACL_KEY_B

#define MIFARE_CLASSIC_CMD_Halt						0x50 // 0x00
#define MIFARE_CLASSIC_CMD_AUTHENTICATION_KEY_A		0x60
#define MIFARE_CLASSIC_CMD_AUTHENTICATION_KEY_B		0x61
#define MIFARE_CLASSIC_CMD_PERSONALIZE_UID_USAGE	0x40
#define MIFARE_CLASSIC_CMD_SET_MOD_TYPE				0x43
#define MIFARE_CLASSIC_CMD_READ						0x30
#define MIFARE_CLASSIC_CMD_WRITE					0xa0

#define MIFARE_CLASSIC_GEN1A_OPERATION_UNLOCK       MIFARE_CLASSIC_CMD_SET_MOD_TYPE
#define MIFARE_CLASSIC_GEN1A_OPERATION_WIPE         0x41

#define MIFARE_CLASSIC_ACK                          (0xA)
#define MIFARE_CLASSIC_INVALID_OPERATION            (0x0)
#define MIFARE_CLASSIC_PARITY_CRC                   (0x1)
#define MIFARE_CLASSIC_INVALID_OPERATION_NB         (0x4 | MIFARE_CLASSIC_INVALID_OPERATION)
#define MIFARE_CLASSIC_PARITY_CRC_NB                (0x4 | MIFARE_CLASSIC_PARITY_CRC)

class MFC {
private:
  uint8_t MFC_BUFFER[1 + 1 + 16];
  
  uint8_t Special_Gen1a_generic(const uint8_t operation, const uint8_t bNoHalt = 0x00);
  uint8_t Special_Gen1a_generic_instruction(const uint8_t Instruction, uint8_t SpecificBits);
  
public:
  PN532 *pNFC;
  uint8_t UID[MIFARE_CLASSIC_UID_SIZE], cbUID, SENS_RES[2], SEL_RES;

  void begin();

  MFC(const uint8_t ss_pin, const uint8_t irq_pin, PISR_PN532_ROUTINE Routine);  // Only IRQ
  ~MFC();

  uint8_t Select();
  uint8_t Release();
  uint8_t Authenticate(const uint8_t cmdAuthAB, const uint8_t blockId, const uint8_t key[MIFARE_CLASSIC_KEY_SIZE]);
  uint8_t Read(const uint8_t blockId, uint8_t **ppReceived);
  uint8_t Write(const uint8_t blockId, const uint8_t *pcbData);
  
  uint8_t Special_Gen1a_unlock();
  //uint8_t Special_Gen1a_wipe();
};

#endif
