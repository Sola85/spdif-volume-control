# spdif_rx (modified version)

This folder contains a (minimally modified) version of the of the open-source library `pico_spdif_rx`. This was done since `pico_spdif_rx` is not available as an Arduino library and because slight modifications were necessary in order for it to work in this project.

---

## Original Library

- **Author**: Elehobica
- **Repository**: [https://github.com/elehobica/pico_spdif_rx/](https://github.com/elehobica/pico_spdif_rx/)
- **License**: BSD-2-Clause license 

---

## Modifications Made

- Adoptions of pio program according to comments in order to work with 192MHz CPU frequency
- Modification of `#include` paths, in order for Arduino IDE to be able to work with the library
- switch to `#define PICO_SPDIF_RX_DMA_IRQ 1` and `#define PICO_SPDIF_RX_PIO 1` since spdix tx uses PIO 0 and the corresponding DMA channel
- Removal of `pio_clear_instruction_memory(spdif_rx_pio);` in line 435 of `spdif_rx.cpp` as this seems to affect both PIO blocks, even though only one is passed to the function.
- rename `SPDIF_BLOCK_SIZE` as this gobal variable name is also used by spdif tx.
- Use of `pio_get_dreq` instead of manual logic for determining dreq. 
- Print-debugging

No functional or structural changes were made beyond this.


