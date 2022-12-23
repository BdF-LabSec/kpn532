# kpn532

Arduino PN532 proxy (mini relay).

Minimal SPI library at 4 MHz (maximum with availble divisors on the Arduino), using IRQ insted of status polling.

PN532 supports:
- I2C standard mode (100 kHz) and fast mode (400 kHz) ;
- HSU high speed link (1.288 Mbit/s) ;
- **SPI at 5 MHz maximum**.

Terminal available on USB virtual COM port at 115200 bps.


## Hardware list

- 1 x Arduino Uno R3
- 2 x Elechouse NFC Module V3 (or V4)
- 15 x Dupont wires
    - 1 x M/M
    - 7 x F/M
    - 7 x F/F


## Wiring diagram

```
              ARDUINO                                 READER                           EMULATOR
                             +-----+
+----[PWR]-------------------| USB |--+    .. -------------------------     .. -------------------------
|                            +-----+  |    .. -------------------------\\   .. -------------------------\\
|        GND[a][ ]RST2                |        ELECHOUSE NFC MODULE V3 ||       ELECHOUSE NFC MODULE V3 ||
|      MOSI2[ ][ ]SCK2      A5/SCL[ ] |        [ ] [ ] [ ] [ ]    __   ||       [ ] [ ] [ ] [ ]    __   ||
|         5V[b][ ]MISO2     A4/SDA[ ] |        SCL SDA VCC GND   /  \  ||       SCL SDA VCC GND   /  \  ||
|                             AREF[ ] |                          \__/  ||                         \__/  ||
|                              GND[ ] |     1|#_|ON  (SPI)             ||    1|#_|ON  (SPI)             ||
| [ ]N/C                    SCK/13[c] |     2|_#|KE            SCK[c]  ||    2|_#|KE            SCK[i]  ||
| [ ]IOREF                 MISO/12[d] |                       MISO[d]  ||                      MISO[j]  ||
| [ ]RST                   MOSI/11[e]~|           |||||       MOSI[e]  ||          |||||       MOSI[l]  ||
| [ ]3V3    +---+               10[f]~|          +-----+        SS[f]  ||         +-----+        SS[n]  ||
| [ ]5v    -| A |-               9[g]~|         -|PN532|-      VCC[b]  ||        -|PN532|-      VCC[m]  ||
| [ ]GND   -| R |-               8[ ] |          +-----+       GND[a]  ||         +-----+       GND[k]  ||
| [ ]GND   -| D |-                    |           |||||        IRQ[g]  ||          |||||        IRQ[o]  ||
| [ ]Vin   -| U |-               7[n] |                       RSTO[ ]  ||                      RSTO[ ]  ||
|          -| I |-               6[o]~|    |HSU| 0 | 0                 ||   |HSU| 0 | 0                 ||
| [ ]A0    -| N |-               5[ ]~|    |I2C| 1 | 0                 ..   |I2C| 1 | 0                 ..
| [ ]A1    -| O |-               4[ ] |    |SPI| 0 | 1                 ..   |SPI| 0 | 1                 ..
| [ ]A2     +---+           INT1/3[ ]~|
| [ ]A3                     INT0/2[ ] |    SHARED        a <-> k (GND)      READER ONLY        f (SS)
| [p]A4/SDA  RST SCK MISO     TX>1[ ] |                  b <-> m (5V)                          g (IRQ)
| [q]A5/SCL  [ ] [i] [j]      RX<0[ ] |                  c <-> i (SCK)      EMULATOR ONLY      n (SS)
|            [k] [l] [m]              |                  d <-> j (MISO)                        o (IRQ)
|  UNO R3    GND MOSI 5V  ____________/                  e <-> l (MOSI)     MODE     MINIMAL   p <-> q
 \_______________________/                                                           VERBOSE   p < > q
```


## Picture

![Picture of Arduino UNO R3 and two Elechouse NFC module v3, wired to SPI bus & powered](assets_arduino/kpn532_direct.jpg)


## Programming

Arduino IDE is enough to handle build and flash to the device, but you can also flash the `.hex` binary with `avrdude` or another programmer

```
avrdude -p atmega328p -c arduino -P COM8 -b 115200 -D -U flash:w:kpn532.hex:i
```


## Performance

Measured _waiting_ time for `GetVersion` instruction (`0x60`) is ~20,3ms. Two `WTX` frames are present bewtween.

![Measured waiting time for GetVersion instruction on relayed configuration, WTX frames before answer](assets_arduino/kpn532_proxified_getversion.png)

A normal _waiting_ time is around ~383µs for this instruction

![Measured waiting time for GetVersion instruction on direct configuration](assets_arduino/kpn532_direct_getversion.png)

The proxified penalty is around 20ms, more or less depending on the payload size.

You can check timings with `SDR nfc-laboratory v2.0`


## Behavior

### Presented UID
Presented UID can be different from the original.
- Original UID: `0495910A5D6D80`
- Presented UID: `0895910A`
    - `08` is forced by `PN532` design ;
    - Only 4 bytes in total, 3 bytes available.

### Seen as `Felica`
Sometimes the presented card can be seen as `Felica`. Move the original card to a better position on the antenna, then try again (with a `reset`).


## Traces

With a NXP TagInfo scan

### Minimal mode

```
  .#####.         mimicard 0.1 (Arduino - single board)
 .## ^ ##.__ _    "A La Vie, A L'Amour" - (oe.eo)
 ## / \ /   ('>-
 ## \ / | K  |    /*** Benjamin DELPY `gentilkiwi`
 '## v #\____/         benjamin.delpy@banque-france.fr
  '#####' L\_          Kiwi PN532                      ***/

| Verbosity: minimal
| Reader init...
|   PN532 version 1.6.7
| Emulator init...
|   PN532 version 1.6.7
| Initialization OK!
~ Waiting for target...
| Detected target
|   SENS_RES: 0344
|   SEL_RES : 20
|   UID     : 0495910A5D6D80
~ Waiting for reader on emulator...
| Reader detected!
| Target released
```

### Verbose

- `<R` from the real reader
- `>R` to the real reader
- `<C` from the real card
- `>C` to the real card

```
  .#####.         mimicard 0.1 (Arduino - single board)
 .## ^ ##.__ _    "A La Vie, A L'Amour" - (oe.eo)
 ## / \ /   ('>-
 ## \ / | K  |    /*** Benjamin DELPY `gentilkiwi`
 '## v #\____/         benjamin.delpy@banque-france.fr
  '#####' L\_          Kiwi PN532                      ***/

| Verbosity: full
| Reader init...
|   PN532 version 1.6.7
| Emulator init...
|   PN532 version 1.6.7
| Initialization OK!
~ Waiting for target...
| Detected target
|   SENS_RES: 0344
|   SEL_RES : 20
|   UID     : 0495910A5D6D80
~ Waiting for reader on emulator...
We got a reader! : 08E080
| Reader detected!
<R 9060000000
>C 9060000000
<C 04010133001A0591AF
>R 04010133001A0591AF
<R 00A4040007D276000085010100
>C 00A4040007D276000085010100
<C 6A82
>R 6A82
<R 00A4040007D2760000850100
>C 00A4040007D2760000850100
<C 9000
>R 9000
<R 00A4000002E103
>C 00A4000002E103
<C 6A82
>R 6A82
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 9060000000
>C 9060000000
<C 04010133001A0591AF
>R 04010133001A0591AF
<R 905A00000300000000
>C 905A00000300000000
<C 91CA
>R 91CA
<R 9060000000
>C 9060000000
<C 04010133001A0591AF
>R 04010133001A0591AF
<R 90AF000000
>C 90AF000000
<C 04010103001A0591AF
>R 04010103001A0591AF
<R 90AF000000
>C 90AF000000
<C 0495910A5D6D80995367303020209100
>R 0495910A5D6D80995367303020209100
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 00A4040007D276000085010100
>C 00A4040007D276000085010100
<C 6A82
>R 6A82
<R 900A0000010000
>C 900A0000010000
<C 53AE95457A7FDF8391AF
>R 53AE95457A7FDF8391AF
<R 90AF0000100000000000000000FBF96823321C6E5C00
>C 90AF0000100000000000000000FBF96823321C6E5C00
<C CC0B2E2555EA033D9100
>R CC0B2E2555EA033D9100
<R 906A000000
>C 906A000000
<C 9100
>R 9100
<R 906E000000
>C 906E000000
<C 0020009100
>R 0020009100
<R 906D000000
>C 906D000000
<C 9100
>R 9100
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A0000033010F200
>C 905A0000033010F200
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A0000039011F200
>C 905A0000039011F200
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A00000300805700
>C 905A00000300805700
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A00000342220100
>C 905A00000342220100
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A000003444D0100
>C 905A000003444D0100
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A0000030011F200
>C 905A0000030011F200
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A00000331594F00
>C 905A00000331594F00
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A00000300407800
>C 905A00000300407800
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A0000031120EF00
>C 905A0000031120EF00
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A00000353453100
>C 905A00000353453100
<C 91A0
>R 91A0
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 905A00000303B80000
>C 905A00000303B80000
<C 91A0
>R 91A0
<R 903C0000010000
>C 903C0000010000
<C 131C43204449CBAF8BDE7E643EE0F778BAF778CC122991EC034737CFCE0161B2DF1C5FE2C5576FFE9B92221A2ABAAC35733984682EF679E29190
>R 131C43204449CBAF8BDE7E643EE0F778BAF778CC122991EC034737CFCE0161B2DF1C5FE2C5576FFE9B92221A2ABAAC35733984682EF679E29190
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 900A0000010000
>C 900A0000010000
<C 4494B9FDD9EDB9B391AF
>R 4494B9FDD9EDB9B391AF
<R 90AF000010000000000000000003F877465D67C6C500
>C 90AF000010000000000000000003F877465D67C6C500
<C CC0B2E2555EA033D9100
>R CC0B2E2555EA033D9100
<R 9045000000
>C 9045000000
<C 0F019100
>R 0F019100
<R 906A000000
>C 906A000000
<C 9100
>R 9100
<R 905A00000300000000
>C 905A00000300000000
<C 9100
>R 9100
<R 7703
>C 7703
<C AF1A177C61CE831BC736CD487004A74E52
>R AF1A177C61CE831BC736CD487004A74E52
<R AFBC0A27624F9BE6304254F47592314A51FAD5B3B558A693FA4C74FEE65B4A52CC
>C AFBC0A27624F9BE6304254F47592314A51FAD5B3B558A693FA4C74FEE65B4A52CC
<C AE
>R AE
<R 9060000000
>C 9060000000
<C 04010133001A0591AF
>R 04010133001A0591AF
<R 90AF000000
>C 90AF000000
<C 04010103001A0591AF
>R 04010103001A0591AF
<R 90AF000000
>C 90AF000000
<C 0495910A5D6D80995367303020209100
>R 0495910A5D6D80995367303020209100
<R Status code: 0xB
| Target released
```

### TagInfo side

```
** TagInfo scan (version 4.25.5) 2022-12-22 23:43:24 **
Report Type: -- IC INFO ------------------------------

# IC manufacturer:
NXP Semiconductors

# IC type:
MIFARE DESFire EV3 (MF3D83)

-- NDEF ------------------------------

# No NDEF data storage populated:

-- EXTRA ------------------------------

# Memory information:
Size: 8 kB
Available: 8.0 kB (8192 bytes)

# IC information:
Capacitance: 17 pF

# Version information:
Vendor ID: NXP (0x04)
Hardware info:
* Type/subtype: 0x01/0x01
* Version: 3.0
* Storage size: 8192 bytes (0x1A)
* Protocol: ISO/IEC 14443-4 (0x05)
Software info:
* Type/subtype: 0x01/0x01
* Version: 3.0
* Storage size: 8192 bytes (0x1A)
* Protocol: ISO/IEC 14443-4 (0x05)
Production date: week 20, 2020 (0x2020)

# Authentication information:
Default PICC master key

# Originality Check (asymmetric):
Signature could not be verified with NXP public key

# Originality Check (symmetric):
Originality Check not successful

# TagInfo Version:
Version :4.25.5

# Device Info:
Device Model :samsung ( SM-T636B )
Android OS Version :12

-- FULL SCAN ------------------------------

# Technologies supported:
ISO/IEC 7816-4 compatible
Native DESFire APDU framing
ISO/IEC 14443-4 (Type A) compatible
ISO/IEC 14443-3 (Type A) compatible
ISO/IEC 14443-2 (Type A) compatible

# Android technology information:
Tag description:
* TAG: Tech [android.nfc.tech.IsoDep, android.nfc.tech.NfcA, android.nfc.tech.NdefFormatable]
* Maximum transceive length: 65279 bytes
* Default maximum transceive time-out: 618 ms
* Extended length APDUs supported
* Maximum transceive length: 253 bytes
* Default maximum transceive time-out: 618 ms
MIFARE Classic support present in Android

# Detailed protocol information:
ID: 08:95:91:0A
* Random ID
ATQA: 0x4403
SAK: 0x20
ATS: 0x067533920380
* Max. accepted frame size: 64 bytes (FSCI: 5)
* Supported receive rates:
	- 106, 212, 424 kbit/s (DR: 1, 2, 4)
* Supported send rates:
	- 106, 212, 424 kbit/s (DS: 1, 2, 4)
* Different send and receive rates supported
* SFGT: 1.208 ms  (SFGI: 2)
* FWT: 154.7 ms  (FWI: 9)
* NAD supported
* CID supported
* Historical bytes: 0x80 |.|

# Memory content:

Application ID 0x000000 (PICC)
* Default master key
* Key configuration: (0x0F01)
  - 1 (3)DES key
  - Master key changeable
  - Master key required for:
    ~ directory list access: no
    ~ create/delete files: no
  - Configuration changeable

--------------------------------------
```

#### Originality checks

```
# Originality Check (asymmetric):
Signature could not be verified with NXP public key

# Originality Check (symmetric):
Originality Check not successful
```

Checks are not successful: signature verification and originality check (with symmetric keys from NXP) are UID dependant, and are using the presented one (not from `GetVersion` answer). As it's altered by PN532 emulation, checks fail.


## Licence
CC BY 4.0 licence - https://creativecommons.org/licenses/by/4.0/


## Author

Benjamin DELPY `gentilkiwi`, you can contact me on Twitter ( @gentilkiwi ) or by mail ( benjamin.delpy [at ] banque-france.fr )

This is a POC / experimental development, please respect its philosophy and don't use it for bad things!


## References

### Arduino UNO R3
- https://docs.arduino.cc/hardware/uno-rev3
- https://docs.arduino.cc/static/456435a38ab7cadcf0d9e4d6de11b9bb/A000066-datasheet.pdf

#### Wiring diagram adapted from
- http://busyducks.com/ascii-art-arduinos

### Elechouse NFC Module
- https://www.elechouse.com/product/pn532-nfc-rfid-module-v4/ (same as V3)
- https://www.elechouse.com/elechouse/images/product/PN532_module_V3/PN532_%20Manual_V3.pdf

### NXP PN532
- https://www.nxp.com/products/rfid-nfc/nfc-hf/nfc-readers/nfc-integrated-solution:PN5321A3HN
- https://www.nxp.com/docs/en/nxp/data-sheets/PN532_C1.pdf
- https://www.nxp.com/docs/en/user-guide/141520.pdf

### AVRDUDE - AVR Downloader Uploader
- https://github.com/avrdudes/avrdude

### SDR nfc-laboratory v2.0
- https://github.com/josevcm/nfc-laboratory
