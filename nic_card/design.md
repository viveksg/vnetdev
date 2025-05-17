# NIC Chip Design Specification

This repository contains the design specification for a custom NIC (Network Interface Controller) chip simulated in software. The chip is used for learning and demonstration of low-level networking, DMA, and driver development.

---

## 🔧 Functional Overview

- **Receive Path**: TAP → RX DMA buffer → DMA to host → Notify driver
- **Transmit Path**: Driver → TX descriptor + buffer → NIC → TAP
- **DMA Engine**: Ring-buffer based transfer with TX/RX descriptors
- **Interrupts**: On RX, TX, and error events
- **Timers**: Optional; for polling and deferred processing

---

## 🧠 FSM (Finite State Machine)

| From State | Trigger            | To State   |
|------------|--------------------|------------|
| RESET      | Init complete      | IDLE       |
| IDLE       | RX/TX ready        | RX_READY / TX_READY |
| RX/TX_READY| DMA start          | DMA_BUSY   |
| DMA_BUSY   | DMA complete       | IDLE       |
| Any        | Error              | ERROR      |
| ERROR      | Reset command      | RESET      |

---

## 🧱 Memory Layout

| Region                 | Description                | Size   |
|------------------------|----------------------------|--------|
| Register Space         | Config/control registers   | ~4 KB  |
| TX Descriptor Ring     | TX packet descriptors      | ~8 KB  |
| RX Descriptor Ring     | RX packet descriptors      | ~8 KB  |
| TX DMA Buffer          | On-chip TX buffer          | 64 KB  |
| RX DMA Buffer          | On-chip RX buffer          | 64 KB  |
| IRQ/Status Block       | Interrupt/status control   | ~4 KB  |

---

## 📊 Stats Tracked

- Packets sent/received
- Bytes transferred
- Errors/drops

---

## 📂 Files

- `NIC_Chip_Design_Specs.pdf`: Written spec
- `fsm_diagram.png`: Finite State Machine diagram
- `memory_map.png`: Memory layout diagram
- `README.md`: You are here!

---

## 🛠 Future Work

- Add visual diagrams (FSM + Memory Map)
- Integrate device tree and platform driver in kernel
- Implement real packet testing via TAP and QEMU

