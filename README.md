# SPDIFVolumeControl
### What?
This is an Arduino sketch for a Raspberry Pi Pico that reads TOSLINK or SPDIF audio data, modifies the volume, and writes the audio data to a second TOSLINK/SPDIF port. The volume can be adjusted by an IR remote. Optionally the current volume and a VU meter can be displayed on a WS2812 LED strip.

### Why?
I'm in a peculiar situation: I want to connect my LG "Smart" TV to my Logitech PC speakers. However, the smart TV has only 3 types of audio outputs:

1. TOSLINK
2. HDMI ARC
3. Bluetooth 

In particular, it doesn't have an analog output. Using Bluetooth to connect to the speaker is not an option, since I don't want to deal with the hassle of always making sure a wireless connection is established. 
The Logitech speakers support various types of analog input, as well as TOSLINK. Which sounds fine, however the TV refuses to control the volume of the speakers when they are connected via TOSLINK. Since my speakers don't have a separate remote, I'm stuck.

Hence this project was born. It is designed to forward TOSLINK audio from the TV to the speakers while intercepting IR signals of the TV remote, allowing me to adjust the volume!

I'm aware that this method of adjusting the volume can:
- lead to loss of quality when significantly reducing the volume and 
- can lead to distortions when amplifying the volume.

The first point is not an issue for a non-audiophile like me. If the analog volume knob of the speaker is set to a value such that the unmodified signal is close to the target volume, I only need to adjust the volume a few increments above or below this. This typicalls leads to a quality loss of at most ~2 bits, which I can't notice. Distortions due to clipping would be more impactful, but these can be avoided by simply not amplifying the source signal using this method or by only amplifying when the source signal is very quiet.

### Compiling/Modifying

I used the following software versions to compile this project:
| Component              | Version |
|------------------------|---------|
| Arduino IDE            |   2.3.4 |
| Raspberry Pi Pico Core |   4.4.4 |
| audio-tools            |   1.0.1 (early 2025) |
| IRremote               |   4.4.1 |
| Adafruit_NeoPixel      |  1.12.5 |

Download the `code` folder and the contents should directly compile as an Arduino sketch, provided that the above dependencies are installed.

See `schematic/schematic.pdf` for wiring details.

Link to the 3D model [here](https://cad.onshape.com/documents/30e0986ab7bef6b26c69f87a/w/68a5b29d9b0bd2b4bf7dde1c/e/23cf1baabf4b8192e4efa997?renderMode=0&uiState=682874b4fb4dde6d5078e9e4).

### Usage

- Connect TOSLINK to the TV and Speakers as per schematic.
- A single orange/green led on the strip indicates the set volume. Each LED represents half an effective bit (ENOB), on a range from 17 ENOBs to 7 ENOBs.
- If a signal is detected on the input, the on board LED will start blinking and the volume indicator on the LED strip will turn from orange to green.
- The (volume-adjusted) input signal is displayed on the led strip as a VU Meter, again in the units of ENOB.


### Notes

- Currently supports **16-bit 44.1/48 kHz audio**, as this is the format my TV outputs.
- `pico_spdif_rx` should support 24-bit and higher sample rates, but the transmit side (audio-tools) is currently limited to 16-bit.
- IR control is configured for my **LG Magic Remote**, but adapting it to any protocol supported by `IRremote` should be straightforward.

### Libraries

The heavy-lifting is done by [arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools) for transmitting and by [pico_spdif_rx](https://github.com/elehobica/pico_spdif_rx/) for receiving SPDIF signals.

I have included parts of the source of both of these libraries in this repo to accomodate a few minor changes that were necessary to get everything to work together. Despite being included in this repo, these libraries were not written by me and I do not claim ownership of them. More details are in the individual README's.