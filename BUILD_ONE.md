# BUILD ONE ELLIC

### Experimental Build Guide (Early Version)

This document explains how a **single experimental Ellic locomotion machine** can be built.

The guide is intentionally simple and incomplete.
It is meant to help **engineers understand the scale and structure of the machine**, not to serve as a fully detailed manufacturing manual.

More detailed instructions will appear as the project evolves.

---

# What You Are Building

Ellic is a **human-scale walking locomotion machine** based on an **elliptical mechanical linkage**.

Instead of wheels rolling continuously, the mechanism produces a **step-like trajectory** using rotating links.

The result is a vehicle that moves in a motion somewhere between:

* a scooter
* a walking machine
* a mechanical exoskeleton vehicle

The prototype is designed as a **research platform** rather than a finished consumer product.

---

# Important Warning

This is an **experimental machine**.

It involves:

* mechanical loads
* moving linkages
* electric drive systems

Improper construction or testing may cause **equipment damage or personal injury**.

Anyone attempting to build this machine should have experience with:

* mechanical fabrication
* basic electrical systems
* safe prototype testing

Always perform initial tests at **very low speed** and with safety support.

---

# Expected Difficulty

Building a prototype requires moderate to advanced workshop capabilities.

Typical requirements:

* metal fabrication
* mechanical assembly
* basic electronics integration
* firmware flashing and testing

Estimated build time for a small engineering team or experienced builder:

**1–3 months**

---

# Estimated Cost

Prototype cost varies depending on fabrication methods.

Typical range:

**$3,000 – $7,000**
if parts are fabricated in a workshop.

A fully assembled research machine prepared by the project may cost approximately:

**~$20,000**

---

# Main Subsystems

An Ellic machine consists of several major subsystems.

## 1. Structural Frame

The frame holds:

* the rider platform
* the locomotion mechanism
* the drive system

Typical materials:

* steel tubing
* aluminum plates
* welded or bolted frame structure

The frame must be rigid enough to handle rider weight and dynamic loads.

---

## 2. Locomotion Linkage

The core of the machine is an **elliptical walking linkage**.

It converts rotary motion from the drive system into a **closed stepping trajectory**.

Main elements include:

* rotating crank
* connecting rods
* step linkage
* foot platform or wheel-foot element

The geometry determines:

* step length
* ground contact timing
* overall stability

CAD models in this repository describe the current experimental geometry.

---

## 3. Drive System

The locomotion linkage is powered by an electric drive.

Typical configuration:

* electric motor
* reduction gearbox
* chain or belt transmission
* crank shaft drive

The goal is to produce **slow, controlled torque**, not high speed.

---

## 4. Rider Platform

The rider platform includes:

* standing support
* steering interface
* safety handles or bars

The rider acts as the **human pilot** of the machine.

Weight distribution and balance are important for stability.

---

## 5. Electronics

Basic electronics include:

* motor controller
* microcontroller
* battery pack
* safety power switch

Early prototypes use **ESP32-based control electronics**.

Firmware examples are located in:

```
firmware/ESP32ELLIC
```

---

## 6. Control System

The control system is experimental.

Possible interfaces include:

* joystick or gamepad
* BLE control
* VR-linked control experiments

Different control experiments are documented in:

```
vr/
ros/
firmware/
```

---

# Basic Build Sequence

A typical build process might follow these steps.

### 1 Frame Fabrication

Build the main chassis frame.

Ensure:

* rigid structure
* accurate mounting points
* sufficient ground clearance

---

### 2 Linkage Assembly

Assemble the locomotion linkage system.

Check carefully:

* alignment
* free movement
* absence of binding

This is the most mechanically sensitive part of the machine.

---

### 3 Drive Installation

Install:

* motor
* gearbox
* transmission
* crank shaft

Verify rotation before attaching the linkage.

---

### 4 Electronics Installation

Mount:

* battery
* motor driver
* microcontroller
* emergency power cutoff

Route cables safely away from moving parts.

---

### 5 Firmware Setup

Flash firmware to the control board.

Initial tests should verify:

* motor direction
* throttle response
* emergency stop

---

### 6 First Motion Tests

Initial tests should be done:

* at very low speed
* with external support
* without a rider if possible

Observe:

* linkage motion
* frame vibration
* ground contact stability

---

### 7 First Ride

Once the mechanism behaves predictably, a careful first ride can be attempted.

Use:

* protective gear
* a controlled environment
* low speed

Expect tuning and adjustments.

This is an experimental platform.

---

# Current Limitations

The current Ellic prototype is still under development.

Known limitations include:

* experimental stability
* mechanical tuning required
* control systems evolving
* limited long-term durability testing

Builders should treat this machine as a **research prototype**.

---

# If You Attempt a Build

If you attempt to build an Ellic machine:

* document your progress
* share photos or notes
* open an issue in the repository

Independent builders help improve the design.

---

# Why This Guide Exists

The goal of the Ellic project is not only to build a single machine.

It is to explore whether **human-scale walking machines** can become a new engineering platform.

If multiple people experiment with similar mechanisms, the design can evolve faster.

This document is the **first step toward that possibility**.

---

More detailed build documentation will appear in future versions.