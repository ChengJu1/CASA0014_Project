# Interactive Home Lighting System with Arduino MKR WiFi 1010 💡

---

This product is designed to develop a more **aesthetically pleasing** and **interactive lighting controller**. The larger external casing makes it more accessible for **visually impaired users**, allowing them to easily locate the controller and enhancing safety when searching for the light switch. The pressure sensors are fixed in specific positions, ensuring **stable and consistent control**. As development progresses, the enclosure design will also be refined for improved appearance.

---

## 📁 Project Structure
```
CASA0014_Project/
│
├── Develop_process/      → Contains earlier versions and debugging code used during system development.
│
├── Documents/            → Includes circuit diagrams, design sketches, and initial concept documentation.
│
├── Final_version/        → Contains the final working Arduino code (main .ino file for MKR WiFi 1010).
│
├── img/                  → Prototype and final product images used for documentation and demonstration.
│
└── README.md             → Main project documentation (this file).
```

### Directory Details

- **Develop_process/** - Earlier versions and debugging code used during system development
- **Documents/** - Circuit diagrams, design sketches, and initial concept documentation
- **Final_version/** - Final working Arduino code (main .ino file for MKR WiFi 1010)
- **img/** - Prototype and final product images used for documentation and demonstration
- **README.md** - Main project documentation

## Control Method 

The system consists of **two switches** and **two pressure sensors**:

- **One switch** serves as the **main power control** — pressing it will turn off the lights immediately, regardless of the current lighting mode.
- **The second switch** is used to cycle through **six preset lighting modes**, including:

  1. **Reading Mode** – Bright cool white  
  2. **Relax Mode** – Warm amber tone  
  3. **Movie Mode** – Dim blue light  
  4. **Sunrise Mode** – Gradual warm-to-white transition  
  5. **Party Mode** – Fast-changing random colors  
  6. **Night Light Mode** – Soft dim orange for nighttime use  

The pressure sensors detect the **strength of the press** — the harder the press, the brighter the light. To avoid accidental activation, the brightness adjustment is triggered **only when both pressure sensors are pressed simultaneously**.

---

## Hardware List 

- **Arduino MKR WIFI 1010**  
- **2 switches** (SS-5GL)  
- **2 Pressure sensors** (Youmile FSR402 Thin film)  

---

## Dependencies 

- **Arduino IDE 2.3.2 or higher**  
- **WiFiNINA v1.8.14 or higher**  
- **PubSubClient v2.8 or higher**  
- **SPI** (built-in library)  

---