# 🛠️ BUILD ONE ELLIC

### Experimental Build Guide (Real-World Version)

This document describes how a **real Ellic machine is actually built in practice**.

It is not theoretical — it reflects the **actual workflow, tools, and decisions** used during prototyping.

The goal is simple:
👉 help you understand **what you really need to build one**


<p align="center">
  <img src="media/battle.gif?v=1" width="30%">
  <img src="media/VRELLIC.gif?v=2" width="30%">
  <img src="media/VRBOT.gif?v=3" width="30%">
</p>

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

# ⚙️ 2. 🚲 Bike Components & Machined Parts (Outsourced or DIY)


You will need:

- standart bike components
- simple shafts  
- connection axles  

These are:

- 🟢 cheap  
- 🟢 simple  
- 🟢 easy to outsource  

👉 You can order them from a local machinist.
👉 Option:
- source yourself  
- or order a **pre-selected kit from the project. just ask me somhere here**

---

# 🔩 3. Laser-Cut Parts (Highly Recommended)

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

# 🧱 4. Optional: 🎨 Body & Styling 3D Printing

- covers  
- visual design  
- plywood 🪵  
- foam panels 🧩  
- composite / carbon panels  
Not required for function, but important for experience.

---

# 🔌 5. 🔗 Wiring & Electronics Assembly 

After mechanics works — add electronics.

### Core components:

- ⚡ 2× motor wheel drivers  
- 🔋 battery (36V, 4–8Ah)  
- 🔽 voltage step-down converter  
- 🧠 ESP32 controller (WiFi + BLE)  
- 🎮 joystick  

Everything is mounted directly on the framу CONNECT:

- motor drivers → motor wheels  
- battery → drivers  
- ESP32 → control signals  
- joystick → ESP32  

---

# 🧪 6. First Functional Mode (Scooter Mode)

At this stage: 👉 The machine works as a **drive platform**

Control logic:

- joystick → ESP32  
- ESP32 → motor drivers  
- motor wheels → motion  

You now have:

🟢 a controllable vehicle  
🟢 basic mobility  
🟢 testable system  

---

# 🤖 7. Mech Mode (Extended System)

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

# 🎯 8. MechSport Layer

With full setup:

- 🎮 player controls mech  
- 🚶 machine mirrors motion  
- 🔫 virtual weapons controlled by joystick  

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

# 💰 Cost & Time

Typical prototype:

- $3,000 – $7,000 (DIY build)
Prebuilt system:
- ~$20,000  
- solo builder: **2-4 months**  

---

# All the draws, CAD models, firmware i try you can [get here]() 

If not just ask me dselog@gmail.com 

[README.MD](README.md)

[START_A_TEAM](START_A_TEAM.md)

---

<a href="https://github.com/Dvijtech/ellic/stargazers" target="_blank" style="text-decoration:none;">
  <kbd>⭐ Star the repository</kbd>
</a>

<a href="https://github.com/Dvijtech/ellic/fork" target="_blank" style="text-decoration:none;">
  <kbd>🔧 Fork the project</kbd>
</a>

<a href="https://github.com/Dvijtech/ellic/issues" target="_blank" style="text-decoration:none;">
  <kbd>💡 Open issues with ideas</kbd>
</a>

<a href="https://twitter.com/intent/tweet?text=Check+out+this+project+on+GitHub&url=https://github.com/Dvijtech/ellic" target="_blank" style="text-decoration:none;">
  <kbd>📢 Share the project</kbd>
</a>
