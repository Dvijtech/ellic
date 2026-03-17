# 🛠️ BUILD ONE ELLIC

### Experimental Build Guide (Real-World Version)

This document describes how a **real Ellic machine is actually built in practice**.

It is not theoretical — it reflects the **actual workflow, tools, and decisions** used during prototyping.

The goal is simple:
👉 help you understand **what you really need to build one**

---

# 🚀 What You Are Building

Ellic is a **human-scale walking machine** driven by an **elliptical linkage mechanism**.

It behaves somewhere between:

- 🛴 a scooter  
- 🚶 a walking machine  
- 🤖 a mech-like vehicle  

This is not just transport — it is a **mechanical platform + experimental interface system**.

---

# ⚠️ Important Warning

This is a **mechanically active experimental system**.

Risks include:

- moving linkages ⚙️  
- structural loads 🏗️  
- electric drive systems ⚡  

👉 Build only if you understand:
- mechanical systems
- basic electronics
- safe testing practices

Always test:
- at **low speed**
- with **support**
- with **emergency stop ready**

---

# 🧠 How the Build Actually Happens

The build is **not linear engineering perfection**.

In reality, it looks like this:

👉 Mechanics first  
👉 Parts sourcing in parallel  
👉 Electronics later  
👉 Software last  

---

# 🔧 1. Mechanical Fabrication (Core Stage)

This is **80% of the real work**.

### Required tools:

- angle grinder (minimum viable) 🪚  
- tube cutting + bending tools 🔩  
- welding setup 🔥  
- (optional but ideal) laser cutter ✂️  

### Materials:

- steel tubes (main structure)
- standard bearings (used as-is)

💡 Design principle:
> Bearings are selected to fit directly into standard tubes  
(no complex machining required)

---

# ⚙️ 2. Machined Parts (Outsourced or DIY)

Some parts **cannot be avoided**.

You will need:

- simple shafts  
- connection axles  

These are:

- 🟢 cheap  
- 🟢 simple  
- 🟢 easy to outsource  

👉 You can order them from a local machinist.

---

# 🚲 3. Bike Components (Parallel Step)

A large part of Ellic uses **standard bicycle components**.

You need:

- drivetrain parts  
- mechanical connectors  
- possibly wheels / motor wheels  

👉 Option:
- source yourself  
- or order a **pre-selected kit from the project**

---

# 🔩 4. Laser-Cut Parts (Highly Recommended)

For:

- brackets  
- mounts  
- structural connectors  

👉 A **laser cutter** significantly improves:

- precision  
- assembly speed  
- rigidity  

Without it:
- you can improvise, but accuracy will drop

---

# 🧱 5. Optional: 3D Printing

Useful for:

- covers  
- mounts  
- interface parts  
- visual design  

Alternatives:

- plywood 🪵  
- foam panels 🧩  

High-end builds may use:

- metal  
- composite / carbon panels  

---

# 🎨 6. Body & Styling

Not required for function, but important for experience.

Typical materials:

- rubberized foam panels  
- plastic sheets  
- composite shells  

👉 This is where Ellic becomes:

**a machine → a mech**

---

# 🔌 7. Electronics Assembly

After mechanics works — add electronics.

### Core components:

- ⚡ 2× motor wheel drivers  
- 🔋 battery (36V, 4–8Ah)  
- 🔽 voltage step-down converter  
- 🧠 ESP32 controller (WiFi + BLE)  
- 🎮 joystick  

Everything is mounted directly on the frame.

---

# 🔗 8. Wiring

Connect:

- motor drivers → motor wheels  
- battery → drivers  
- ESP32 → control signals  
- joystick → ESP32  

⚠️ Important:

- keep wires away from moving parts  
- secure everything physically  

---

# 🧪 9. First Functional Mode (Scooter Mode)

At this stage:

👉 The machine works as a **drive platform**

Control logic:

- joystick → ESP32  
- ESP32 → motor drivers  
- motor wheels → motion  

You now have:

🟢 a controllable vehicle  
🟢 basic mobility  
🟢 testable system  

---

# 🤖 10. Mech Mode (Extended System)

Now the interesting part.

You can connect:

- 🥽 VR headset  
- 🎮 same joystick  
- 💻 Unreal Engine system (from repo)

Result:

👉 The machine becomes part of a **mixed-reality mech system**

You can:

- control movement physically  
- control a mech avatar  
- interact in AR/VR space  

---

# 🎯 11. MechSport Layer

With full setup:

- 🎮 player controls mech  
- 🚶 machine mirrors motion  
- 🔫 virtual weapons controlled by joystick  

You can:

- participate in mech competitions  
- score points  
- join teams  

Future direction:

- 🏆 tournaments  
- 💰 prize pools  
- 🤝 team revenue sharing  

---

# 🧭 Build Summary (Reality)

Real build flow:

1. 🔧 fabricate frame  
2. ⚙️ assemble linkage  
3. 🧱 install machined parts  
4. 🚲 add bike components  
5. 🔩 mount brackets (laser-cut)  
6. 🔌 install electronics  
7. 🎮 test scooter mode  
8. 🤖 connect VR system  

---

# 💰 Cost Reality

Typical prototype:

- $3,000 – $7,000 (DIY build)

Prebuilt system (future):

- ~$20,000  

---

# ⏱️ Time Estimate

- solo builder: **1–3 months**  
- small team: faster  

---

# 🧪 Current State

This is still:

- experimental ⚠️  
- evolving ⚠️  
- not production-ready ⚠️  

Expect:

- tuning  
- failures  
- redesigns  

---

# 🤝 If You Build One

If you attempt:

- 📸 document everything  
- 🧠 share insights  
- 🐞 open issues  

👉 This project grows through builders.

---

# 🌍 Why This Matters

Ellic is not just a machine.

It is an attempt to explore:

👉 **a new class of human-scale locomotion systems**

If enough people build and experiment:

- designs improve  
- use cases emerge  
- a new category can form  

---

# 🔜 What's Next

Future versions will include:

- detailed drawings  
- exact part lists  
- assembly tolerances  
- control system documentation  

---

**Build one. Break it. Improve it. Repeat.** 🔁
