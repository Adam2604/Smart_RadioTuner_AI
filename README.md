# 📻 Smart Tuner AI: Intelligent SDR Radio & Content Classifier

An AI-powered Software Defined Radio (SDR) system that goes beyond simple frequency tuning. This project combines real-time digital signal processing with deep learning to automatically identify radio content, enabling features like automatic commercial skipping and genre-based station hunting.

---

## 💡 How It Works

The system bridges high-performance C++ signal processing with Python’s advanced machine learning ecosystem using a real-time UDP pipeline.

### The Core Pipeline:

1. **Signal Acquisition:** The C++ core interfaces with an **RTL-SDR** dongle to capture raw IQ data at a sample rate of 1.152 MHz.
2. **Digital Signal Processing:** * **FM Demodulation:** Raw data is converted into audio samples using phase difference calculation.
* **Filtering:** A custom **FIR (Finite Impulse Response)** filter and de-emphasis are applied to ensure high-quality audio output.


3. **Real-Time Bridge:** The processed audio is streamed via **UDP (Port 5005)** to a Python backend and simultaneously played locally via **PortAudio**.
4. **AI Classification:** * The Python service accumulates 3-second audio buffers and transforms them into **Mel Spectrograms**.
* A multi-output **TensorFlow/Keras** model analyzes the spectrogram to determine the category (Speech vs. Music) and specific subcategory (e.g., Pop, Rock, News, or Ad).


5. **Feedback Loop:** Results are sent back to the C++ application via **UDP (Port 5006)** to trigger automated hardware actions.

---

## 🌟 Key Features

### 🚫 Anti-Ad Mode (`m1`)

Tired of commercials? When activated, the system monitors the current station's metadata.

* **Logic:** If the AI model detects a "Commercial" (Reklama), the radio automatically switches to the next pre-defined station in the list.

### 🎵 Genre Hunter (`m2`)

A smart search engine for live radio.

* **Targeting:** Users can specify a desired genre (e.g., Rock, Pop, or News).
* **Automation:** The system will scan through available frequencies until it finds a station currently broadcasting content that matches the user's choice.

### 🧠 Dual-Head Neural Network

The underlying AI doesn't just guess; it analyzes content on two levels simultaneously:

* **Main Category:** Distinguishes between speech and music.
* **Subcategory:** Identifies specific formats like 80s-00s hits, alternative rock, pop-rock, reggae, or news broadcasts.

### 📈 Training & Dataset Tools

The project includes a full pipeline for building the intelligence:

* **`cutter.py`:** Automatically slices long recordings into uniform 3-second training samples.
* **Spectrogram Analysis:** Uses `librosa` to convert audio into a visual format that the convolutional neural network (CNN) can process.

---

## 🏗️ Tech Stack

### C++ Backend (Signal Processing)

* **Libraries:** `librtlsdr`, `PortAudio`, `FFTW3`.
* **Features:** Real-time FM demodulation, FIR filtering, UDP networking, and multi-threaded console interaction.

### Python Backend (Artificial Intelligence)

* **Framework:** `TensorFlow` / `Keras`.
* **Audio Analysis:** `Librosa`.
* **Architecture:** Convolutional Neural Network (CNN) with Gaussian Noise layers for robustness against radio interference.

### Infrastructure

* **Build System:** CMake.
* **Testing:** GoogleTest (integrated for Filter and WavWriter validation).
