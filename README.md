# kpn532
Arduino PN532 proxy (mini relay)


# Wiring Diagram

```
              ARDUINO                                     READER                               EMULATOR
                             +-----+
+----[PWR]-------------------| USB |--+        .. -------------------------          .. -------------------------
|                            +-----+  |        .. -------------------------\\        .. -------------------------\\
|        GND[a][ ]RST2                |            ELECHOUSE NFC MODULE V3 ||            ELECHOUSE NFC MODULE V3 ||
|      MOSI2[ ][ ]SCK2      A5/SCL[ ] |            [ ] [ ] [ ] [ ]    __   ||            [ ] [ ] [ ] [ ]    __   ||
|         5V[b][ ]MISO2     A4/SDA[ ] |            SCL SDA VCC GND   /  \  ||            SCL SDA VCC GND   /  \  ||
|                             AREF[ ] |                              \__/  ||                              \__/  ||
|                              GND[ ] |         1|#_|ON  (SPI)             ||         1|#_|ON  (SPI)             ||
| [ ]N/C                    SCK/13[c] |         2|_#|KE            SCK[c]  ||         2|_#|KE            SCK[i]  ||
| [ ]IOREF                 MISO/12[d] |                           MISO[d]  ||                           MISO[j]  ||
| [ ]RST                   MOSI/11[e]~|               |||||       MOSI[e]  ||               |||||       MOSI[l]  ||
| [ ]3V3    +---+               10[f]~|              +-----+        SS[f]  ||              +-----+        SS[n]  ||
| [ ]5v    -| A |-               9[g]~|             -|PN532|-      VCC[b]  ||             -|PN532|-      VCC[m]  ||
| [ ]GND   -| R |-               8[ ] |              +-----+       GND[a]  ||              +-----+       GND[k]  ||
| [ ]GND   -| D |-                    |               |||||        IRQ[g]  ||               |||||        IRQ[o]  ||
| [ ]Vin   -| U |-               7[n] |                           RSTO[ ]  ||                           RSTO[ ]  ||
|          -| I |-               6[o]~|        |HSU| 0 | 0                 ||        |HSU| 0 | 0                 ||
| [ ]A0    -| N |-               5[ ]~|        |I2C| 1 | 0                 ..        |I2C| 1 | 0                 ..
| [ ]A1    -| O |-               4[ ] |        |SPI| 0 | 1                 ..        |SPI| 0 | 1                 ..
| [ ]A2     +---+           INT1/3[ ]~|
| [ ]A3                     INT0/2[ ] |        SHARED        a <-> k (GND)           READER ONLY        f (SS)
| [p]A4/SDA  RST SCK MISO     TX>1[ ] |                      b <-> m (5V)                               g (IRQ)
| [q]A5/SCL  [ ] [i] [j]      RX<0[ ] |                      c <-> i (SCK)           EMULATOR ONLY      n (SS)
|            [k] [l] [m]              |                      d <-> j (MISO)                             o (IRQ)
|  UNO R3    GND MOSI 5V  ____________/                      e <-> l (MOSI)          MODE     MINIMAL   p <-> q
 \_______________________/                                                                    VERBOSE   p < > q
```
