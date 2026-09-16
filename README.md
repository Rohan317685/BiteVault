<h1 align="center">
  <br>
  <img src="https://cdn.hackclub.com/01a0abce-a8c7-7daf-926c-197d34cb86b0/snip-2026-09-16_21-00-36.png" alt="BiteVault" width="200"></a>
  <br>
  BiteVault
  <br>
</h1>

<h4 align="center">
A simple CH32 Cheap, Bit sized security key! 

<div align="center">

![KiCad](https://img.shields.io/badge/kicad-%2300578F.svg?style=for-the-badge&logo=kicad&logoColor=white)

</div>

<p align="center">
  <a href="#key-features">Key Features</a> •
  <a href="#pcb">PCB</a> •
  <a href="#firmware">Firmware</a> •
  <a href="#credits">Credits</a> •
  <a href="#license">License</a>
</p>

<img src="https://cdn.hackclub.com/01a0abd2-2aff-7583-9270-7d204c61655b/snip-2026-09-16_21-04-28.png" alt="3d model" width="800"/>



## Key Features

- **CH32** based security key
- **Very cheap** at mass production BOM is estimated to be 3-4$/pop
- **USB-A**
- **2-layer PCB** designed for cheap prices at bulk
- **Panelized design** for cost-effective manufacturing
- **BiteSized** to munch on them but also their extremely portable


## PCB

Designed in Kicad! 

### Schematic

<img src="https://user-cdn.hackclub-assets.com/01a0abd4-32ec-729c-8b13-946bcfdc7ea3/snip-2026-09-16_21-06-34.png" alt="Schematic" width="800"/>

### PCB Layers

The 2-layer stackup:

**Layer 1 (Main routing layer):**

<img src="https://user-cdn.hackclub-assets.com/01a0abd5-15dc-7300-8431-2d0d5c87a123/snip-2026-09-16_21-07-28.png" alt="PCB Layer 1" width="800"/>

**Layer 2 (Layer to navigate tight spaces):**

<img src="https://cdn.hackclub.com/01a0abd5-1767-7cf0-bee9-e5608a2504b5/snip-2026-09-16_21-07-37.png" alt="PCB Layer 2" width="800"/>

This project uses:

- [KiCad](https://www.kicad.org/)


## Firmware

Lives in [`firmware/`](firmware) and is built with
[ch32fun](https://github.com/cnlohr/ch32fun):

```sh
git submodule update --init --recursive
cd firmware && make flash
```

Right now it brings the board up, enumerates over USB as a FIDO HID device,
speaks the CTAPHID transport, and can talk to the ATECC608A by pressing the
button runs a secure element self test, which is a handy way to check a freshly
assembled board. The crypto (key generation, signing, registration) is not
written yet, so it is not a working security key just yet; see
[firmware/README.md](firmware/README.md) for exactly what is and is not there.

## AI
For transparency AI has been used in the project for the firmware only, AI has not partaken in any research or hardware aspect of the project. 

## License

MIT

---

> Readme inspired by [@NotARoomba](https://github.com/NotARoomba) &nbsp;&middot;&nbsp; go follow him now!!! : D
