# Embedded Network System Coursework

Listen to UART via C232HM Cable
```c
$ screen /dev/{console_port} 115200
// example - baud rate 9600 with UTF-8 encoding
$ sudo screen /dev/ttyUSB0 9600 -U
```

Flash Atmegga via USB
```c
$ avrdude -c usbasp -p m644p -U flash:w:{program}.hex
```

For UART communication 
| Atmegga Port | C232HM Cable | Application   |
|:------------:|:------------:|:-------------:|
| PD0 (RX)     | Orange (TX)  |      -        |
| PD1 (TX)     | Yellow (RX)  |      -        |
| PD2          |     -        |  INT0 (Port0) |
| PD3          |     -        |  INT1 (Port1) |
| PA0          |     -        |  ADC0         |
| PD4 (not enabled)|     -    |  OC1B (TIMER1)|
| PD5          |     -        |  OC1A (TIMER1)|



### App circuit setup
- push-button switch
- 1kohm resistor
- 220nF capacitor
The interrupt pins are pulled high; therefore, to trigger the interrupt cycle, those pins have to be pulled low. Each interrupt pin will be connected to a 1kohm resistor. One side of the switch is connected to the resistor and the other side connected to GND. A capacitor is placed across the switch to debounce the switch.

### App circuit diagram
<img src=app_circuit_diagram.png>
