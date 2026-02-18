<h1 align="center"><b>SMART FARM KIT</b></h1>

  The Smart Farm Kit is an MCU-based agricultural monitoring and control system designed to automate plant care and monitor environmental conditions. It uses an ESP32 microcontroller integrated with multiple sensors and actuators to ensure optimal plant growth.

The system can:
- Monitor soil moisture, temperature, humidity, water level, and light intensity
- Detect motion and distance
- Automatically control water pump, fan motor, and servo motor
- Provide alerts using buzzer and LED indicators
- Display data using an OLED module

<img width="548" height="828" alt="image" src="https://github.com/user-attachments/assets/f11fa58f-3a4c-4a07-b775-eb5102b626e8" />


<img width="548" height="482" alt="image" src="https://github.com/user-attachments/assets/c4195dd9-05c8-4f3d-8305-8d09e42a9b5d" />


<h2 align="center"><b>Wiring Connections</b></h2>


Each component is connected to the ESP32 using designated GPIO pins:

<img width="548" height="600" alt="image" src="https://github.com/user-attachments/assets/61360d32-8151-43f0-98f4-0e0be1a532e3" />

<img width="548" height="554" alt="image" src="https://github.com/user-attachments/assets/38090ef5-0ea6-46f3-b838-4824853a4c92" />

<h2 align="center"><b>System Architecture Diagram</b></h2>

  The Smart Farm Kit follows a three-layer embedded system architecture consisting of Input, Processing, and Output layers. The ESP32 microcontroller serves as the central processing unit that gathers sensor data, evaluates programmed conditions, and controls output devices accordingly.
