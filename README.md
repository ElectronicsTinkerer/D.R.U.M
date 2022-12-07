# D.R.U.M (Digital Rhythmic Utility Modules)
Files for the D.R.U.M. Machine, the modular, expandable drum machine complete with digital AND analog circuitry!


![CAD rendering of the DRUM sequencer with three modules attached to its right size](assembly.png)

## File Structure

```
.
├── doc             # Documentation on components, thoughts, h/sware details
├── module          # Files specific to the modules
│   ├── cad         # Housing design
│   ├── hardware    # PCB design (kicad)
│   └── software    # Module firmware
├── README.md       # INFO (this doc)
├── samples         # Sample to c-array conversion utils
│   └── bass-0      # Revision 0 Bass Drum samples (RAW, PCM-16)
├── sequencer       # Files specific to the sequencer
│   ├── cad         # Housing design
│   ├── hardware    # PCB design (kicad)
│   └── software    # Sequencer firmware
└── slidepot        # Slide potentiometer daughter board
    ├── cad         # 3D models for slidepot (knob)
    └── hardware    # Daughter board design
```

## System Architecture

The D.R.U.M. system is composed of a sequencer (the base and primary controller) and between 1 and 8 "modules." The digital section of each device is based on the RP2040 microcontroller, the same device found on the Raspberry Pi Pico development board. The RP2040 was selected due to its relatively high clock speed (compared to a '328, for example), multiple cores, its PIO state machines, good documentation, and great SDK. (Oh, and it's only $1). 

The analog section is designed around a completely passive routing scheme, where each of the channels is simple shifted one position over each module.


## Docs

The `doc` directory contains most of the relevant documentation to this project. This will contain information abart different parts of the system, such as the serial bus protocol or the data sheets for the components used.

* [`serbus.md`](doc/serbus.md) contains information on the custom 32-bit scan chain protocol used for communication between modules. This is implemented using the RP2040's PIO state machines, meaning that the bus can be much faster than bit-banging in software. It also allows an "address-free" control scheme, where modules (bus devices) are selected by their depth on the scan chain, rather than an address. This allows a user to completely fill the bus with 8 of the same module if desired.


