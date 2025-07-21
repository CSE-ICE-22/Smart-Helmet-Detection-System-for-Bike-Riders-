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
- Microcontroller: ESP32/Arduino/Raspberry Pi
- Camera/Sensor: IR sensor or small camera module for helmet detection
- Buckle Detection: Magnetic sensor or pressure switch
- Ignition Control: Relay module to cut off ignition
- Alert System: Buzzer & LED indicators

#### ⛑ Helmet Unit
- Helmet Detection: Accelerometer/IR sensor to detect wearing status(not yet confirm)
- Buckle Sensor: Reed switch or Hall effect sensor for buckle status
- Wireless Communication: RF/NRF24L01/BLE module to transmit data to the bike unit

### 📅 Project Timeline
| **Week**    | **Task**                                                   |
|-------------|------------------------------------------------------------|
|    1        | Identify the project scope and finalize the project idea   |
|    2        | Research & Identify hardware components                    |

### 📂 Repository Structure --(not yet)--
