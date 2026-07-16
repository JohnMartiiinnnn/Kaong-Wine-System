
Here is the detailed unit testing plan for your Automated Kaong Wine Brewing 
System, tailored to prepare you for developing your data logging source code. 

### Purpose of Unit Testing
Before you begin writing the software to log data, you must isolate and test 
each individual electronic component [1]. The primary objective of this phase is
to:
*   **Prevent Cascading Failures:** Ensure that faulty high-voltage components 
(like your 1.5kW heaters) do not damage sensitive low-voltage logic components 
(like the ESP32 or sensors) [1].
*   **Verify Voltage Stability:** Ensure there is no "voltage sag" under load, 
which could corrupt your data logging or SD card write processes [1].
*   **Establish Baseline Accuracy:** Calibrate analog and digital sensors 
against known standards so the data you eventually log to your CSV files is 
actually accurate [1].

### Unit Testing Plan
Based on your manuscript's testing protocol, you need to perform the following 7
unit tests. Each test requires **10 independent trials**, with an expected 
success rate of at least 80% to pass [2, 3].

| Test No. | Component | Test Procedure | Expected Result |
| :--- | :--- | :--- | :--- |
| **1** | **Power Supply** | Measure voltage output under load (with Heaters 
ON). | Stable 12V ± 0.2V; no significant voltage sag. |
| **2** | **Load Cells** | Place a 1.0 kg calibrated weight on the platform. | 
Reading of 1.0 kg ± 0.05 kg after taring. |
| **3** | **DS18B20** | Immerse the temperature probes in boiling water (100°C).
| Reading of 100°C ± 0.5°C. |
| **4** | **pH Sensor** | Submerge the probe in a pH 4.0 buffer solution. | 
Reading of 4.0 ± 0.1 pH after calibration. |
| **5** | **RAPT Pill** | Float the device in water and check the WiFi signal. |
Stable Specific Gravity (SG) reading of 1.000; No data loss. |
| **6** | **Relay Module** | Trigger a HIGH signal from the ESP32 to each 
channel. | Audible "click" and LED indicator turns ON every time. |
| **7** | **Servo Motor** | Run a basic "Sweep" code (0° to 180°). | The arm 
moves its full range without stalling or jittering. |

### Preparing for Data Logging Development
Since your immediate goal is to start developing the source code for data 
logging, **Tests 3, 4, and 5** are the most critical right now. These tests 
validate the DS18B20 temperature sensors, the pH sensor, and the RAPT Pill, 
which are the exact metrics your system needs to write to the SD card [2, 4].

**Important notes for your data logging code:**
*   **Hardware Setup:** As we discussed previously, since your TFT display's SD 
card reader is broken, remember to wire your standalone SD Card module to the 
same SPI pins on the ESP32. 
*   **Future System Testing:** While unit testing ensures the sensors read 
correctly, the actual performance of your data logging code will be formally 
evaluated later during the **System Testing phase (Test 18)**. During that 
phase, you will run the system for 1-hour intervals to ensure the SD Card 
successfully saves timestamped CSV logs of pH, Temperature, and Specific Gravity
every 15 minutes without generating corrupted files [4, 5].

