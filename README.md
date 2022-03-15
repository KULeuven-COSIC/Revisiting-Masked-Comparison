# Revisiting Higher-Order Masked Comparison for Lattice-Based Cryptography: Algorithms and Bit-sliced Implementations

This is the code supporting our paper: "Revisiting Higher-Order Masked Comparison for Lattice-Based Cryptography: Algorithms and Bit-sliced Implementations" (https://eprint.iacr.org/2022/110). 

## Instructions

The build flow for this repository is based on [libopencm](https://github.com/libopencm3/libopencm3) and the [libopencm3-template repository](https://github.com/libopencm3/libopencm3-template). It also takes away some elements from [PQM4](https://github.com/mupq/pqm4). The code targets the `STM32F407G-DISC1` board, but a `make` target is also available for executing on a host PC.

Being able to run the code in those repositories is a pre-requisite for this repository. In particular, it is necessary to install/setup:

* The `arm-none-eabi-gcc` toolchain
* `stlink`
* `Python3` with `pyserial`

It also also necessary to initialize `libopencm`:

* `git submodule update --init libopencm3`
* `make -C libopencm3`

## Makefile

The code in this repository can be built with `make` and flashed to the board with `make flash`. The serial output can be retrieved with `make screen`, which should be set-up before flashing the binary.

There are a few optional flags that can be selected in the `Makefile`:

* Profiling can be enabled: `{PROFILE_TOP_CYCLES, PROFILE_TOP_RAND, PROFILE_STEP_CYCLES, PROFILE_STEP_RAND}`.

  * `PROFILE_x_CYCLES` profiles the number of cycles for either the total execution (`x=TOP`) or the individual steps (`x=STEP`).

  * `PROFILE_x_RAND` profiles the requested number of random bytes for either the total execution (`x=TOP`) or the individual steps (`x=STEP`).

* The scheme can be selected: `{SABER, KYBER}`

* The number of shares can be configured: `{NSHARES=x}`

* The number of tests can be configured: `{NTESTS=x}`

* The comparison technique can be selected: `{Simple, GF, Arith, Hybridsimple}`

Additionally, it is possibly to compile the code for execution on a host PC by setting `{PLATFORM=host}`. This also enables the `-DDEBUG` flag, which adds debugging statements to the code execution within the routines. The host executable can then be run with `make run`, which can be used for testing purposes.

## License

Files developed in this work are released under the [MIT License](./LICENSE). In addition, if you use or build upon the code in this repository, please cite our paper using our [citation key](./CITATION).

[B2A.c](./src/B2A.c) is licensed under [GNU General Public License version 2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html).
