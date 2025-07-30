## Smart Helmet Detection System for Bike Riders ##
- Author: Balasuriyage Don Stephan Kanishka Balasuriya (Index: 220056X)
- University: University of Moratuwa, Sri Lanka
- Department: Computer Science & Engineering (ICE Stream)
- Course: Semester 5 - Embedded Systems Project
- Supervisor: [Supervisor's Name]

### 📌 Overview
### This project implements a Smart Helmet Detection System to enforce helmet compliance among bike riders. The system ensures:
- Helmet Detection (whether the rider is wearing the helmet)
- Buckle Detection (whether the helmet strap is properly fastened)
- Ignition Control (bike won’t start unless the helmet is worn and buckled)
- Real-time Alerts (warnings if the helmet is removed mid-ride)
- Post-Ride Safety Check (alarm if the rider dismounts without removing the helmet)

### 🔧 Key Features
#### ✔ Before Ride:
- Bike ignition is disabled unless the helmet is worn and buckled.
- Visual/Audio alerts if the helmet is not detected or the buckle is open.

#### ✔ During Ride:
- If the rider removes the helmet or unbuckles it, a 3-4 minute warning is issued.
- After the warning period, the bike automatically stops.

#### ✔ After Ride:
- If the rider leaves without removing the helmet, an alarm sounds until the helmet is   taken off.

### 🛠 Hardware & Software Components  -- (not yet confirm)--
#### 🖥️ Main System (Bike Unit)
- Microcontroller: ESP32
- Ignition Control: Relay module to cut off ignition
- Alert System: Buzzer & LED indicators
- Detect that Rider get off from bike: Stand sensor

#### ⛑ Helmet Unit
- Microcontroller: ESP32 c3 super mini
- Helmet Detection: Piezo sensor and Bioelectric EMG or Capacitive touch sensor to detect wearing status
- Buckle Sensor: Piezo sensor or Magnet sensor or Hall effect sensor for buckle status
- Wireless Communication: BLE to transmit data to the bike unit


### 📂 Repository Structure --(not yet)--
