# ELEE 2045 Assignment 5: Smart Voice Assistant & Cloud Web Dashboard

**Name:** Nikhil Mahadevan  

<img src="images/headshot.jpg" alt="My Headshot" width="200"/>

---

## 🌐 Live Deployment Link
**Dashboard URL:** https://0518-73-7-194-238.ngrok-free.app  

---

## 🎥 Video Demonstration
Link to Assignment 5 Video: https://youtu.be/_ZTwuI9cxw8

---

## 🚀 Project Description

This project is a **Smart Voice Assistant system** that integrates an M5Stick device with a cloud-accessible Flask web dashboard. The system enables bidirectional communication between embedded hardware and a backend server.

### System Overview
- The **M5Stick** records audio using its onboard microphone
- Audio is converted from PCM → WAV and uploaded to the backend
- The device periodically polls the backend for todo updates
- Users can mark tasks as complete directly from the device

### Backend
The backend is implemented using **Flask** and uses **SQLite** for persistent storage.

Database tables:
- `todos`: stores task text, completion state, and pin priority
- `notes`: stores transcripts, summaries, and audio file paths

### Speech Processing
- Audio files are processed using **OpenAI Whisper** (if installed)
- Transcripts are automatically generated
- A short summary is created for quick viewing

### Frontend
- Built using **HTML + Jinja templates**
- Lightweight CSS for clean UI
- Supports:
  - Todo management
  - Voice note viewing
  - Audio playback

---

## ☁️ Cloud Deployment & Security Architecture

### Deployment Strategy
- Flask server runs locally
- Exposed publicly using **ngrok**
- Monolithic design (frontend + backend together)

### Security Features
- **Session-based authentication** for dashboard users
- **API key authentication** for device access

#### Protections Implemented:
- `X-API-Key` header validation for device endpoints
- Secure comparison using `secrets.compare_digest`
- Unauthorized requests return proper HTTP errors
- HTTPS enforced via ngrok tunnel

---

## 🛠️ How to Build & Run Locally (For Development)

### Installation
```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

### Environment Variables
Set the following:
```bash
export FLASK_SECRET_KEY="your-secret-key"
export DASHBOARD_PASSWORD="your-dashboard-password"
export DEVICE_API_KEY="device-key"
export WHISPER_MODEL="base"
```

Install ffmpeg (required for Whisper)
```bash
brew install ffmpeg
```

Run Server
```bash
python3 basic_flask.py
```

Expose Public URL (ngrok)
```bash
ngrok http 5001 --host-header=rewrite
```
Use the generated HTTPS URL as the BASE_URL in your M5Stick client.
```bash
https://0518-73-7-194-238.ngrok-free.app
```

## 🔌 Backend API
### Endpoints
- GET /api/todos
- POST /api/todos/<id>/done
- DELETE /api/todos/<id>
- POST /api/audio (multipart upload)
Required Headers
```bash
X-API-Key: <DEVICE_API_KEY>
ngrok-skip-browser-warning: true
```
## ✨ Creative Extension
### Task Pinning / Prioritization System

This project extends the base requirements by adding the ability to **pin (star) important tasks**:

- Users can mark tasks as "pinned"
- Pinned tasks are prioritized and displayed at the top of the dashboard
- The M5Stick device reflects pinned task ordering
- Backend stores pin state in the database

Benefits:
- Helps prioritize important tasks
- Improves usability of todo system
- Adds a layer of task management beyond basic CRUD

## 📁 Project Structure
```bash
.
├── part2.py                 # Flask backend
├── templates/
│   └── index.html          # Dashboard UI
├── m5stick_client.ino      # M5Stick firmware
├── audio_uploads/          # Stored audio files
├── assistant.db            # SQLite database
└── requirements.txt
```

## ✅ Features Summary
- Secure login-protected dashboard
- Todo add/edit/delete + pinning
- Voice note upload and playback
- Automatic transcription + summary
- M5Stick ↔ server bidirectional sync
- REST API with authentication
