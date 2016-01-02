Demonstration of using a 4-character 7-segment LED display and multiple
timers.

Written for MSP430G2553 but might work for other MSP430G2xxx devices with
at least two timers, such as the MSP430G2x53 and MSPG2x13.

Written using Code Composer Studio v6.1.

Used LED display part KYX-5461AS hooked as follows:

```
    Pin 11 (Segment A) --- P1.6
    Pin  7 (Segment B) --- P1.5
    Pin  4 (Segment C) --- P1.4
    Pin  2 (Segment D) --- P1.3
    Pin  1 (Segment E) --- P1.2
    Pin 10 (Segment F) --- P1.1
    Pin  5 (Segment G) --- P1.0
    Pin  3 (Segment DP) -- P1.7
    Pin 12 (Digit 1) ----- P2.3
    Pin  9 (Digit 2) ----- P2.2
    Pin  8 (Digit 3) ----- P2.1
    Pin  6 (Digit 4) ----- P2.0
```

Where display device is as follows:

```
    Pins     12 11 10  9  8  7
            +-----------------+
    Digits  |  1.  2.  3.  4. |  <-- KYX-5461AS
            +-----------------+
    Pins      1  2  3  4  5  6
```
