# BiteVault firmware

Firmware for the CH32V203G6U6 on the BiteVault board. It brings the hardware up,
enumerates over USB as a FIDO HID device, and speaks the CTAPHID transport that
every FIDO host uses. It is a starting point, not a finished security key —
see [What works](#what-works) before you trust it with anything.

## What works

- **Clocks and board bring-up.** 8 MHz crystal (Y1) → PLL ×12 → 96 MHz, which
  the USB peripheral divides down to the 48 MHz it needs. 1 kHz SysTick
  timebase, so nothing in the main loop blocks.
- **USB FIDO HID.** Enumerates with the FIDO usage page (0xF1D0/0x01) and a
  64-byte interrupt IN/OUT endpoint pair, exactly as the CTAP spec describes.
  Hosts discover it as a security key without any driver.
- **CTAPHID transport.** Channel allocation, message fragmentation and
  reassembly, sequence checking, 500 ms transaction timeouts, busy-channel
  handling, `INIT`, `PING` and `WINK`.
- **U2F (CTAP1) framing.** ISO 7816 APDU parsing (short and extended) and a
  working `U2F_VERSION`.
- **ATECC608A driver.** Wake pulse, sleep, command framing with the device's
  CRC-16, and the `Info`, `Random` and config-zone `Read` commands — enough to
  read the revision and serial number and pull entropy off the part.
- **Board self test.** Runs at boot and on every press of SW1. Result goes to
  D1 and to the debug channel, so you can tell a good assembly from a bad one
  with nothing but a USB port.

## What does not work yet

**This cannot register or authenticate anything.** `U2F_REGISTER` returns
`SW_INS_NOT_SUPPORTED` and `U2F_AUTHENTICATE` returns `SW_WRONG_DATA` (the
status word for a key handle the device does not own — correct, since it has
never issued one). CTAP2/CBOR is not implemented and, accordingly, the CBOR
capability bit is not set in the `INIT` response, so hosts will not try WebAuthn
against it.

Getting to a real key needs, roughly in order:

1. Provision the ATECC608A: write and lock the config zone, set up slots for the
   attestation key and for key generation.
2. ECDSA P-256 key generation and signing through the `GenKey`/`Sign` commands.
3. A key handle scheme (wrap the private key or the slot reference so it can be
   handed to the relying party and recognised later), plus the counter.
4. Attestation certificate storage and the `U2F_REGISTER` response format.
5. User presence gating on SW1 for every signing operation, with `KEEPALIVE`
   frames while the host waits.
6. CTAP2: CBOR parsing, `authenticatorGetInfo`, `makeCredential`,
   `getAssertion`, and resident keys if you want passwordless.

## Layout

| Path | What it is |
| --- | --- |
| `bitevault.c` | Main loop and the USB stack callbacks |
| `usb_config.h` | Device/config/HID descriptors and the FIDO report descriptor |
| `funconfig.h` | Clock and ch32fun options |
| `src/board.h` | Pin map, taken from the KiCad netlist |
| `src/timebase.c` | 1 kHz SysTick tick |
| `src/led.c` | Non-blocking status patterns on D1 |
| `src/button.c` | Debounced SW1 with press latching |
| `src/i2c.c` | I2C1 master on PB6/PB7 |
| `src/atecc608.c` | ATECC608A (U4) driver |
| `src/ctaphid.c` | CTAPHID transport |
| `src/u2f.c` | U2F/CTAP1 message layer |

## Pin map

Straight out of `Production/netlist.ipc`:

| Signal | Pin | Notes |
| --- | --- | --- |
| Status LED (D1) | PA1 | Anode at 3V3 through R5, so the pin sinks: **low = lit** |
| User button (SW1) | PA0 | Shorts to GND, internal pull-up: **low = pressed** |
| USB D− / D+ | PA11 / PA12 | Through R7/R6 and the USBLC6 (U2) |
| I2C SCL / SDA | PB6 / PB7 | To the ATECC608A (U4), pulled up by R4/R3 |
| SWDIO / SWCLK | PA13 / PA14 | TP1 / TP2 |
| Crystal | PD0 / PD1 | Y1 with C9/C10 |

## LED patterns

| Pattern | Meaning |
| --- | --- |
| Short blip every few seconds | Powered, USB not configured yet |
| Solid on | Enumerated and idle |
| Fast flicker | Handling a CTAPHID transaction |
| Slow even blink | Wink from the host, or self test passed |
| Urgent blink | Secure element self test failed |

## Building

You need a bare-metal RISC-V GCC. Any of these work:

```sh
# Debian/Ubuntu
sudo apt install gcc-riscv64-unknown-elf picolibc-riscv64-unknown-elf

# Arch
sudo pacman -S riscv64-elf-gcc riscv64-elf-newlib

# or the xPack toolchain ch32fun recommends: riscv-none-elf-gcc
```

Then:

```sh
git submodule update --init --recursive   # pulls in ch32fun
cd firmware
make
```

The Ubuntu `gcc-riscv64-unknown-elf` package ships without newlib headers, so
point the build at picolibc's instead:

```sh
make NEWLIB=/usr/lib/picolibc/riscv64-unknown-elf/include
```

Current size: about 8.9 KB of the 32 KB flash and 3 KB of the 10 KB RAM.

## Flashing

Wire a WCH-LinkE to the test points — TP1 to SWDIO, TP2 to SWCLK, TP3 to 3V3,
TP4 to GND — and:

```sh
make flash      # builds and writes bitevault.bin
make monitor    # printf output over the same link
```

`make flash` builds ch32fun's `minichlink` on first use. On Linux you may need
its udev rules (`ch32fun/minichlink/99-WCH-LinkE.rules`) to talk to the
programmer without root.

## Trying it out

With the key plugged in:

```sh
lsusb -d 1209:0001 -v          # check the HID descriptor
fido2-token -L                 # libfido2: the device should be listed
```

Anything that enumerates the device and sends `INIT`/`PING` will work. WebAuthn
in a browser will find the key and then fail the registration, which is the
expected outcome until the crypto above is done.

The CTAPHID and U2F layers are plain C with no hardware dependencies beyond a
millisecond counter, so they can be compiled and exercised on a host — that is
how the framing, fragmentation, sequence handling and status words in this
firmware were checked.

## USB IDs

The build ships with `1209:0001`, the pid.codes *test* VID/PID. That is fine on
your own bench and nowhere else — get a real PID from
[pid.codes](https://pid.codes) before handing these out.
