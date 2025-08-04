## Smart Helmet Detection System for Bike Riders ##
- Author: Balasuriyage Don Stephan Kanishka Balasuriya (Index: 220056X)
- University: University of Moratuwa, Sri Lanka
- Department: Computer Science & Engineering (ICE Stream)
- Course: Semester 5 - Embedded Systems Project
- Supervisor: [Supervisor's Name]

### 📌 Overview
#### This project implements a Smart Helmet Detection System to enforce helmet compliance among bike riders. The system integrates both helmet-based and bike-based sensors to ensure that the rider wears and fastens the helmet properly before and during the ride. It also monitors the rider's actions after the ride to enforce post-ride safety behavior. The system ensures:
- Helmet Detection (whether the rider is wearing the helmet)
- Buckle Detection (whether the helmet strap is properly fastened)
- Engine ON Detection via Ignition Wire Tap (Detect that bike is start)
- Ignition Control (bike won’t start unless the helmet is worn and buckled)
- Real-time Alerts (warnings if the helmet is removed mid-ride)
- Post-Ride Safety Check (alarm if the rider dismounts without removing the helmet)

### 🔧 Key Features
#### ✔ Before Ride:
- Bike ignition is disabled unless the helmet is both worn and buckled.
- Visual/Audio alerts (LED + buzzer) if the helmet is not detected or the buckle is unfastened.

#### ✔ During Ride:
- If the rider removes the helmet or unbuckles it mid-ride, a 3-4 minute countdown warning is issued.
- If the condition persists, the bike is automatically stopped via ignition cut-off.

#### ✔ After Ride:
- If the rider gets off the bike (detected using seat pressure and stand sensor) without removing the helmet, an alarm is triggered until the helmet is taken off.

### 🛠 Hardware & Software Components  -- (not yet confirm)--
#### 🖥️ Main System (Bike Unit)
##### Microcontrollers:
- Raspberry Pi (Primary logic and communication hub)
- ESP32 (Secondary controller for real-time hardware I/O)
##### Sensors & Components:
- Pressure Sensor (FSR or Load Cell) – Detects rider presence on seat
- Stand Sensor – Detects whether the bike stand is deployed
- Relay Module – Controls bike ignition
- Buzzer & LED Indicators – Alert system for helmet/buckle status
- BLE Module or Adapter – Receives data from helmet unit
- Power Source – Battery or bike power system

#### ⛑ Helmet Unit
##### Microcontroller:
- ESP32-C3 Super Mini
##### Sensors:
- Helmet Detection:
    - Piezo Sensor (detects pressure when helmet is worn)
    - Capacitive Touch Sensor (detects contact with head/skin)
- Buckle Detection:
    - Magnet Sensor, Reed Switch, or Piezo Sensor for detecting strap connection
- Battery Monitoring:
    - Lithium Battery Pack with Power Indicator (monitors battery level via ADC or dedicated battery fuel gauge)
##### Communication:
- Bluetooth Low Energy (BLE) – Sends status data to bike unit
##### Power:
- Lithium-ion battery powering the ESP32-C3 and sensors
  
### 🧱 Architecture Diagram
<img src=".\images\high level functional block.png" alt="High Level System Architecture" width="700"/>
<img src=".\images\Helmet Unit.png" alt="Helmet Unit" width="700"/>
<img src=".\images\Bike Unit.png" alt="Bike Unit" width="700"/>
### 📂 Repository Structure --(not yet)--
