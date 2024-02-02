/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#ifndef _KPN532_SPI_H_INCLUDED
#define _KPN532_SPI_H_INCLUDED

#include <SPI.h>

/* A little note about SPI speed on NXP PN532:

   SPI maximum speed is officially @ 5 MHz (PN532_C1 - 8.3.5)
   **BUT**, in reality, [BYTE - with clock] + [interbytes - without clock] is sensitive
   All considered the clock must be around 3.5 MHz
   
   - As ARDUINO R3 has a little time interbytes, it can go to 4 MHz (maybe more, but limited by divider)
     - TgInitAsTarget (48 bytes): 134.2 µs
   - As ARDUINO R4 is more optimized, less interbytes time, we must go to 3 MHz
     - TgInitAsTarget (48 bytes): 141.4 µs - ironically, it can be interesting to add a few ns delay between bytes to go to 4 MHz...
*/

#if defined(ARDUINO_AVR_UNO)
#define PN532_SPI_SPEED 4000000
#elif defined(ARDUINO_UNOR4_MINIMA)
#define PN532_SPI_SPEED 3000000
#else
#error Platform not tested or supported
#endif

#define PN532_INTERFRAME_DELAY_US 10

#define PN532_T_osc_start 2  // in ms, see 7.2.11 PowerDown (SPI)

#define PN532_SPI_Data_Writing 0x01    // host controller to PN532
#define PN532_SPI_Status_Reading 0x02  // PN532 to host controller
#define PN532_SPI_Data_Reading 0x03    // PN532 to host controller

#define PN532_TFI_Host_to_PN532 0xd4
#define PN532_TFI_PN532_to_Host (PN532_TFI_Host_to_PN532 + 1)

#define PN532_CMD_Diagnose 0x00
#define PN532_CMD_GetFirmwareVersion 0x02
#define PN532_CMD_GetGeneralStatus 0x04
#define PN532_CMD_ReadRegister 0x06
#define PN532_CMD_WriteRegister 0x08
#define PN532_CMD_ReadGPIO 0x0c
#define PN532_CMD_WriteGPIO 0x0e
#define PN532_CMD_SetSerialBaudRate 0x10
#define PN532_CMD_SetParameters 0x12
#define PN532_CMD_SAMConfiguration 0x14
#define PN532_CMD_PowerDown 0x16

#define PN532_CMD_RFConfiguration 0x32
#define PN532_CMD_RFRegulationTest 0x58

#define PN532_CMD_InJumpForDEP 0x56
#define PN532_CMD_InJumpForPSL 0x46
#define PN532_CMD_InListPassiveTarget 0x4a
#define PN532_CMD_InATR 0x50
#define PN532_CMD_InPSL 0x4e
#define PN532_CMD_InDataExchange 0x40
#define PN532_CMD_InCommunicateThru 0x42
#define PN532_CMD_InDeselect 0x44
#define PN532_CMD_InRelease 0x52
#define PN532_CMD_InSelect 0x54
#define PN532_CMD_InAutoPoll 0x60

#define PN532_CMD_TgInitAsTarget 0x8c
#define PN532_CMD_TgSetGeneralBytes 0x92
#define PN532_CMD_TgGetData 0x86
#define PN532_CMD_TgSetData 0x8e
#define PN532_CMD_TgSetMetaData 0x94
#define PN532_CMD_TgGetInitiatorCommand 0x88
#define PN532_CMD_TgRespondToInitiator 0x90
#define PN532_CMD_TgGetTargetStatus 0x8a

typedef enum _PN532_FRAME_TYPE {
  PN532_UNKNOWN = 0,
  PN532_NORMAL_INFORMATION_FRAME = 1,    // 6.2.1.1 Normal information frame
  PN532_EXTENDED_INFORMATION_FRAME = 2,  // 6.2.1.2 Extended information frame (not used here)
  PN532_ACK_FRAME = 3,                   // 6.2.1.3 ACK frame
  PN532_NACK_FRAME = 4,                  // 6.2.1.4 NACK frame
  PN532_ERROR_FRAME = 5,                 // 6.2.1.5 Error frame
} PN532_FRAME_TYPE,
  *PPN532_FRAME_TYPE;

typedef void (*PISR_PN532_SPI_ROUTINE)();

class PN532_SPI {
public:
  PN532_SPI(const uint8_t ss_pin, const uint8_t irq_pin, PISR_PN532_SPI_ROUTINE Routine);  // Only IRQ
  ~PN532_SPI();

  void begin();
  volatile uint8_t IrqState;

  uint8_t RfConfiguration(uint8_t MxRtyPassiveActivation = 0xff);  // MaxRetries
  uint8_t SAMConfiguration();
  uint8_t GetFirmwareVersion(uint8_t *pIC = NULL, uint8_t *pVer = NULL, uint8_t *pRev = NULL, uint8_t *pSupport = NULL);
  uint8_t InListPassiveTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t *pcbUID, uint8_t SENS_RES[2], uint8_t *pSEL_RES);  // NFC-A, 106kbps, 1 target, ATS not here
  uint8_t InRelease();
  uint8_t InDataExchange(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t **ppReceived, uint8_t *pcbReceived);
  uint8_t TgInitAsTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t SENS_RES[2], uint8_t SEL_RES);
  uint8_t TgGetData(uint8_t **ppReceived, uint8_t *pcbReceived, uint8_t *pErrorCode = NULL);
  uint8_t TgSetData(const uint8_t *pcbInData, const uint8_t cbInData);

private:
  uint8_t _ss, _irq;
  uint8_t Buffer[262 + 2 + 9], cbData;

  uint8_t Information_Frame_Exchange();
  uint8_t Wait_Ready_IRQ();
  void Information_Frame_Host_To_PN532();
  PN532_FRAME_TYPE Generic_Frame_PN532_To_Host();
};

#endif
