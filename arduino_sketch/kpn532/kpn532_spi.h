/*	Benjamin DELPY `gentilkiwi`
	LabSec - DGSI DIT ARCOS
	benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
	Licence : https://creativecommons.org/licenses/by/4.0/
*/
#ifndef _KPN532_SPI_H_INCLUDED
#define _KPN532_SPI_H_INCLUDED

#include <SPI.h>

#define PN532_INTERFRAME_DELAY_US 10

void PrintHex(const byte *pcbData, const uint32_t cbData);

#define PACKET_DATA_IN (pbBuffer + 7)
#define PACKET_DATA_OUT (pbBuffer + 8)

typedef enum _KPN532_FRAME {
  KPN532_FRAME_Unk = 0,  // ?
  KPN532_FRAME_Information = 1,
  KPN532_FRAME_Extended_Information = 2,  // not used
  KPN532_FRAME_Ack = 3,
  KPN532_FRAME_Nack = 4,
  KPN532_FRAME_Error = 5
} KPN532_FRAME,
  *PKPN532_FRAME;

class PN532_SPI {
public:
  // PN532_SPI(const uint8_t ss_pin);                         // No IRQ, max timeout 25500 (25.5 ms)
  PN532_SPI(const uint8_t ss_pin, const uint8_t irq_pin);  // Only IRQ
  ~PN532_SPI();

  uint8_t begin();

  void setVerbose(const uint8_t bIsVerbose) {
    _bIsVerbose = bIsVerbose;
  }
  uint8_t getVerbose() {
    return _bIsVerbose;
  }

  uint8_t GetFirmwareVersion(uint8_t *pIC = NULL, uint8_t *pVer = NULL, uint8_t *pRev = NULL, uint8_t *pSupport = NULL);
  uint8_t InListPassiveTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t *pcbUID, uint8_t SENS_RES[2], uint8_t *pSEL_RES);  // NFC-A, 106kbps, 1 target, ATS not here
  uint8_t InRelease();
  uint8_t InDataExchange(const uint8_t *pbData, const uint8_t cbData, uint8_t **ppReceived, uint8_t *pcbReceived);
  uint8_t TgInitAsTarget(uint8_t *pbUID, uint8_t cbUID, uint8_t SENS_RES[2], uint8_t SEL_RES);
  uint8_t TgGetData(uint8_t **ppReceived, uint8_t *pcbReceived);
  uint8_t TgSetData(const uint8_t *pbData, const uint8_t cbData);
  uint8_t RfConfiguration(uint8_t MxRtyPassiveActivation = 0xff);  // MaxRetries
  uint8_t SAMConfiguration();

private:
  uint8_t _bIsVerbose;
  uint8_t _ss, _irq;
  // uint16_t _timeout_loops;
  uint8_t pbBuffer[262 + 2 + 9], cbBuffer, cbResult;

  void Information_Frame_Host_To_PN532(const uint8_t cbData);
  inline void Generic_Frame_Host_To_PN532(const uint8_t *pbData, const uint8_t cbData);
  uint8_t Generic_Frame_PN532_To_Host();

  uint8_t Wait_Ready();
  uint8_t Status_Frame();
  // uint8_t Wait_Ready_Status();
  uint8_t Wait_Ready_IRQ();

  uint8_t Information_Frame_Exchange(const uint8_t cbData);
};

#endif
