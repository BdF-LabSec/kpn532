/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
*/
#ifndef _KPN532_H_INCLUDED
#define _KPN532_H_INCLUDED
#include <Arduino.h>
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

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_LEONARDO)
#define PN532_SPEED 4000000
#elif defined(ARDUINO_UNOR4_MINIMA)
#define PN532_SPEED 3000000
#else
#error Platform not tested or supported
#endif

#define PN532_INTERFRAME_DELAY_US 10

#define PN532_T_osc_start 2  // in ms, see 7.2.11 PowerDown (SPI)

#define PN532_Data_Writing 0x01    // host controller to PN532
#define PN532_Status_Reading 0x02  // PN532 to host controller
#define PN532_Data_Reading 0x03    // PN532 to host controller

#define PN532_TFI_Host_to_PN532 0xd4
#define PN532_TFI_PN532_to_Host (PN532_TFI_Host_to_PN532 + 1)

typedef enum _PN532_FRAME_TYPE {
  PN532_UNKNOWN = 0,
  PN532_NORMAL_INFORMATION_FRAME = 1,    // 6.2.1.1 Normal information frame
  PN532_EXTENDED_INFORMATION_FRAME = 2,  // 6.2.1.2 Extended information frame (not used here)
  PN532_ACK_FRAME = 3,                   // 6.2.1.3 ACK frame
  PN532_NACK_FRAME = 4,                  // 6.2.1.4 NACK frame
  PN532_ERROR_FRAME = 5,                 // 6.2.1.5 Error frame
} PN532_FRAME_TYPE,
  *PPN532_FRAME_TYPE;

#define PN532_PRINTHEX_NONE 0b00000000
#define PN532_PRINTHEX_NOLN 0b00000001
#define PN532_PRINTHEX_REV  0b00000010

#pragma pack(push)
#pragma pack(1)
typedef struct _PN53X_REGISTER_VALUE {
  uint16_t Register;
  uint8_t Value;
} PN53X_REGISTER_VALUE, *PPN53X_REGISTER_VALUE;
#pragma pack(pop)

#define KPN532_OUTPUT_LEVEL_NONE 0
#define KPN532_OUTPUT_LEVEL_ERROR 1
#define KPN532_OUTPUT_LEVEL_INFO 2
#define KPN532_OUTPUT_LEVEL_DEBUG 3

#define KPN532_OUTPUT_LEVEL KPN532_OUTPUT_LEVEL_INFO

typedef void (*PISR_PN532_ROUTINE)();

class PN532 {

public:
  PN532(const uint8_t ss_pin, const uint8_t irq_pin, PISR_PN532_ROUTINE Routine);  // Only IRQ
  ~PN532();

  void begin();
  volatile uint8_t IrqState;  // needed in external ISR routine

  // Miscellaneous
  uint8_t Diagnose();  // TODO
  uint8_t GetFirmwareVersion(uint8_t *pIC = NULL, uint8_t *pVer = NULL, uint8_t *pRev = NULL, uint8_t *pSupport = NULL);
  uint8_t GetGeneralStatus();  // TODO
  uint8_t ReadRegister(uint16_t Register, uint8_t *pValue);
  uint8_t ReadRegister(uint16_t *pRegisters, uint8_t *pValues, uint8_t cb);  // TODO
  uint8_t WriteRegister(uint16_t Register, uint8_t Value);
  uint8_t WriteRegister(uint16_t *pRegisters, uint8_t *pValues, uint8_t cb);  // TODO
  uint8_t WriteRegister(const PN53X_REGISTER_VALUE *pRV, uint8_t szData);
  uint8_t ReadGPIO();           // TODO
  uint8_t WriteGPIO();          // TODO
  uint8_t SetSerialBaudRate();  // TODO
  uint8_t SetParameters();      // TODO
  uint8_t SAMConfiguration(uint8_t IRQ = 0x01);
  uint8_t PowerDown();  // TODO
  // RFcommunication
  uint8_t RfConfiguration__RF_field(uint8_t ConfigurationData);                               // CfgItem = 0x01: RF field (AutoRFCA & RF on/off)
  uint8_t RfConfiguration__Various_timings(uint8_t fATR_RES_Timeout, uint8_t fRetryTimeout);  // CfgItem = 0x02: Various timings (fATR_RES_Timeout & fRetryTimeout)
  uint8_t RfConfiguration__MaxRtyCOM(uint8_t ConfigurationData);                              // CfgItem = 0x04: MaxRtyCOM
  uint8_t RfConfiguration__MaxRetries(uint8_t MxRtyPassiveActivation = 0xff);                 // CfgItem = 0x05: MaxRetries (MxRtyATR, MxRtyPSL & MxRtyPassiveActivation)
  uint8_t RfConfiguration__Analog_settings_A_106();                                           // CfgItem = 0x0A: Analog settings for the baudrate 106 kbps type A
  uint8_t RfConfiguration__Analog_settings_212_424();                                         // CfgItem = 0x0B: Analog settings for the baudrate 212/424 kbps
  uint8_t RfConfiguration__Analog_settings_B();                                               // CfgItem = 0x0C: Analog settings for the type B
  uint8_t RfConfiguration__Analog_settings_212_424_848_l4();                                  // CfgItem = 0x0D: Analog settings for baudrates 212/424 and 848 kbps with ISO/IEC14443-4 protocol
  uint8_t RFRegulationTest();                                                                 //
  // Initiator
  uint8_t InJumpForDEP();                                                                                               // TODO
  uint8_t InJumpForPSL();                                                                                               // TODO
  uint8_t InListPassiveTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t *pcbUID, uint8_t SENS_RES[2], uint8_t *pSEL_RES);  // NFC-A, 106kbps, 1 target, ATS not here
  uint8_t InATR();                                                                                                      // TODO
  uint8_t InPSL();                                                                                                      // TODO
  uint8_t InDataExchange(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t **ppReceived = NULL, uint8_t *pcbReceived = NULL, uint8_t *pErrorCode = NULL);
  uint8_t InCommunicateThru(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t **ppReceived = NULL, uint8_t *pcbReceived = NULL, uint8_t *pErrorCode = NULL);
  uint8_t InDeselect();  // TODO
  uint8_t InRelease(uint8_t *pErrorCode = NULL);
  uint8_t InSelect();    // TODO
  uint8_t InAutoPoll();  // TODO
  // Target
  uint8_t TgInitAsTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t SENS_RES[2], uint8_t SEL_RES);
  uint8_t TgSetGeneralBytes();  // TODO
  uint8_t TgGetData(uint8_t **ppReceived, uint8_t *pcbReceived, uint8_t *pErrorCode = NULL);
  uint8_t TgSetData(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t *pErrorCode = NULL);
  uint8_t TgSetMetaData();          // TODO
  uint8_t TgGetInitiatorCommand(uint8_t **ppReceived, uint8_t *pcbReceived, uint8_t *pErrorCode = NULL);
  uint8_t TgResponseToInitiator(const uint8_t *pcbInData, const uint8_t cbInData, uint8_t *pErrorCode = NULL);
  uint8_t TgGetTargetStatus();      // TODO

private:
  uint8_t _ss, _irq;
  uint8_t Buffer[262 + 2 + 9], cbData;

  uint8_t Information_Frame_Exchange(uint8_t bNoAnswer = 0x00);
  uint8_t Wait_Ready_IRQ();
  void Information_Frame_Host_To_PN532();
  PN532_FRAME_TYPE Generic_Frame_PN532_To_Host();

public:
  static void InitGlobalSPI();
  static void PrintHex(const byte *pcbData, const size_t cbData, const uint8_t flags = PN532_PRINTHEX_NONE);
};

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

// 8.7.1 Standard registers
#define PN53X_REG_Config_I0_I1 0x6103
#define PN53X_REG_Observe_testbus 0x6104
#define PN53X_REG_Data_rng 0x6105
#define PN53X_REG_Control_switch_rng 0x6106
#define PN53X_REG_GPIRQ 0x6107
#define PN53X_REG_LDO 0x6109
#define PN53X_REG_i2c_wu_control 0x610a
#define PN53X_REG_Andet_control 0x610c
#define PN53X_REG_NFC_WI_control 0x610e
#define PN53X_REG_PCR_CFR 0x6200
#define PN53X_REG_PCR_CER 0x6201
#define PN53X_REG_PCR_ILR 0x6202
#define PN53X_REG_PCR_Control 0x6203
#define PN53X_REG_PCR_Status 0x6204
#define PN53X_REG_PCR_Wakeupen 0x6205
#define PN53X_REG_CIU_Mode 0x6301
#define PN53X_REG_CIU_TxMode 0x6302
#define PN53X_REG_CIU_RxMode 0x6303
#define PN53X_REG_CIU_TxControl 0x6304
#define PN53X_REG_CIU_TxAuto 0x6305  // 8.6.23.21 CIU_TxAuto register (6305h)
#define PN53X_REG_CIU_TxSel 0x6306
#define PN53X_REG_CIU_RxSel 0x6307
#define PN53X_REG_CIU_RxThreshold 0x6308
#define PN53X_REG_CIU_Demod 0x6309
#define PN53X_REG_CIU_FelNFC1 0x630a
#define PN53X_REG_CIU_FelNFC2 0x630b
#define PN53X_REG_CIU_MifNFC 0x630c
#define PN53X_REG_CIU_ManualRCV 0x630d
#define PN53X_REG_CIU_TypeB 0x630e
#define PN53X_REG_CIU_CRCResultMSB 0x6311
#define PN53X_REG_CIU_CRCResultLSB 0x6312
#define PN53X_REG_CIU_GsNOff 0x6313
#define PN53X_REG_CIU_ModWidth 0x6314
#define PN53X_REG_CIU_TxBitPhase 0x6315
#define PN53X_REG_CIU_RFCfg 0x6316
#define PN53X_REG_CIU_GsNOn 0x6317
#define PN53X_REG_CIU_CWGsP 0x6318   // 8.6.23.38 CIU_CWGsP register (6318h)
#define PN53X_REG_CIU_ModGsP 0x6319  // 8.6.23.39 CIU_ModGsP register (6319h)
#define PN53X_REG_CIU_TMode 0x631a
#define PN53X_REG_CIU_TPrescaler 0x631b
#define PN53X_REG_CIU_TReloadVal_Hi 0x631c
#define PN53X_REG_CIU_TReloadVal_Lo 0x631d
#define PN53X_REG_CIU_TCounterVal_hi 0x631e
#define PN53X_REG_CIU_TCounterVal_lo 0x631f
#define PN53X_REG_CIU_TestSel1 0x6321
#define PN53X_REG_CIU_TestSel2 0x6322
#define PN53X_REG_CIU_TestPinEn 0x6323
#define PN53X_REG_CIU_TestPinValue 0x6324
#define PN53X_REG_CIU_TestBus 0x6325
#define PN53X_REG_CIU_AutoTest 0x6326
#define PN53X_REG_CIU_Version 0x6327
#define PN53X_REG_CIU_AnalogTest 0x6328
#define PN53X_REG_CIU_TestDAC1 0x6329
#define PN53X_REG_CIU_TestDAC2 0x632a
#define PN53X_REG_CIU_TestADC 0x632b
#define PN53X_REG_CIU_RFlevelDet 0x632f
#define PN53X_REG_SIC_CLK 0x6330
#define PN53X_REG_CIU_Command 0x6331
#define PN53X_REG_CIU_CommIEn 0x6332
#define PN53X_REG_CIU_DivIEn 0x6333
#define PN53X_REG_CIU_CommIrq 0x6334
#define PN53X_REG_CIU_DivIrq 0x6335
#define PN53X_REG_CIU_Error 0x6336
#define PN53X_REG_CIU_Status1 0x6337
#define PN53X_REG_CIU_Status2 0x6338
#define PN53X_REG_CIU_FIFOData 0x6339
#define PN53X_REG_CIU_FIFOLevel 0x633a
#define PN53X_REG_CIU_WaterLevel 0x633b
#define PN53X_REG_CIU_Control 0x633c
#define PN53X_REG_CIU_BitFraming 0x633d
#define PN53X_REG_CIU_Coll 0x633e

// 8.7.2 SFR registers
#define PN53X_REG_SFR_SP_Stack_Pointer 0xff81
#define PN53X_REG_SFR_DPL_Data_Pointer_Low 0xff82
#define PN53X_REG_SFR_DPH_Data_Pointer_High 0xff83
#define PN53X_REG_SFR_PCON 0xff87
#define PN53X_REG_SFR_T01CON 0xff88
#define PN53X_REG_SFR_T01MOD 0xff89
#define PN53X_REG_SFR_T0L 0xff8a
#define PN53X_REG_SFR_T1L 0xff8b
#define PN53X_REG_SFR_T0H 0xff8c
#define PN53X_REG_SFR_T1H 0xff8d
#define PN53X_REG_SFR_S0CON 0xff98
#define PN53X_REG_SFR_S0BUF 0xff99
#define PN53X_REG_SFR_RWL 0xff9a
#define PN53X_REG_SFR_TWL 0xff9b
#define PN53X_REG_SFR_FIFOFS 0xff9c
#define PN53X_REG_SFR_FIFOFF 0xff9d
#define PN53X_REG_SFR_SFF 0xff9e
#define PN53X_REG_SFR_FIT 0xff9f
#define PN53X_REG_SFR_FITEN 0xffa1
#define PN53X_REG_SFR_FDATA 0xffa2
#define PN53X_REG_SFR_FSIZE 0xffa3
#define PN53X_REG_SFR_IE0 0xffa8
#define PN53X_REG_SFR_SPIcontrol 0xffa9
#define PN53X_REG_SFR_SPIstatus 0xffaa
#define PN53X_REG_SFR_HSU_STA 0xffab
#define PN53X_REG_SFR_HSU_CTR 0xffac
#define PN53X_REG_SFR_HSU_PRE 0xffad
#define PN53X_REG_SFR_HSU_CNT 0xffae
#define PN53X_REG_SFR_P3 0xffb0
#define PN53X_REG_SFR_IP0 0xffb8
#define PN53X_REG_SFR_T2CON 0xffc8
#define PN53X_REG_SFR_T2MOD 0xffc9
#define PN53X_REG_SFR_RCAP2L 0xffca
#define PN53X_REG_SFR_RCAP2H 0xffcb
#define PN53X_REG_SFR_T2L 0xffcc
#define PN53X_REG_SFR_T2H 0xffcd
#define PN53X_REG_SFR_PSW_Program_Status_Word 0xffd0
#define PN53X_REG_SFR_CIU_Command 0xffd1
#define PN53X_REG_SFR_CIU_CommIEn 0xffd2
#define PN53X_REG_SFR_CIU_DivIEn 0xffd3
#define PN53X_REG_SFR_CIU_CommIrq 0xffd4
#define PN53X_REG_SFR_CIU_DivIrq 0xffd5
#define PN53X_REG_SFR_CIU_Error 0xffd6
#define PN53X_REG_SFR_I2CCON 0xffd8
#define PN53X_REG_SFR_I2CSTA 0xffd9
#define PN53X_REG_SFR_I2CDAT 0xffda
#define PN53X_REG_SFR_I2CADR 0xffdb
#define PN53X_REG_SFR_CIU_Status1 0xffdf
#define PN53X_REG_SFR_ACC_Accumulator 0xffe0
#define PN53X_REG_SFR_IE1 0xffe8
#define PN53X_REG_SFR_CIU_Status2 0xffe9
#define PN53X_REG_SFR_CIU_FIFOData 0xffea
#define PN53X_REG_SFR_CIU_FIFOLevel 0xffeb
#define PN53X_REG_SFR_CIU_WaterLevel 0xffec
#define PN53X_REG_SFR_CIU_Control 0xffed
#define PN53X_REG_SFR_CIU_BitFraming 0xffee
#define PN53X_REG_SFR_CIU_Coll 0xffef
#define PN53X_REG_SFR_B_register 0xfff0
#define PN53X_REG_SFR_P7FGA 0xfff4
#define PN53X_REG_SFR_P7FGB 0xfff5
#define PN53X_REG_SFR_P7 0xfff7
#define PN53X_REG_SFR_IP1 0xfff8
#define PN53X_REG_SFR_XRAMP 0xfffa
#define PN53X_REG_SFR_P3FGA 0xfffc
#define PN53X_REG_SFR_P3FGB 0xfffd

#endif
