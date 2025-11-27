# 📋 Setup Instructions - ESP32 Repository

## 🎯 Cara Membuat Repository Terpisah untuk ESP32

### Step 1: Inisialisasi Git Repository

```cmd
cd d:\Development\Projects\Active\Pemograman Iot\Smarthome\ESP32
git init
```

### Step 2: Add All Files

```cmd
git add .
git status
```

### Step 3: First Commit

```cmd
git config user.name "Kysu-dev"
git config user.email "your-email@example.com"

git commit -m "Initial commit: ESP32 Smart Home IoT Controllers"
```

### Step 4: Create GitHub Repository

1. Buka https://github.com/new
2. Repository name: `smart-home-esp32-iot`
3. Description: `ESP32 controllers for Smart Home IoT system - sensors, actuators, and ESP32-CAM face recognition`
4. **Public** atau **Private** (pilih sesuai kebutuhan)
5. ❌ **JANGAN** centang "Initialize with README" (sudah ada README.md)
6. Click **Create repository**

### Step 5: Push ke GitHub

```cmd
git remote add origin https://github.com/Kysu-dev/smart-home-esp32-iot.git
git branch -M main
git push -u origin main
```

### Step 6: Verify

Buka: `https://github.com/Kysu-dev/smart-home-esp32-iot`

---

## 🔄 Update Repository (Setelah Edit Code)

```cmd
cd d:\Development\Projects\Active\Pemograman Iot\Smarthome\ESP32

# Cek perubahan
git status

# Add perubahan
git add .

# Commit dengan pesan
git commit -m "Update: add MQTT config switch for local/public broker"

# Push ke GitHub
git push
```

---

## 📁 Struktur Repository yang Akan Dibuat

```
smart-home-esp32-iot/
├── .gitignore                     # ✅ Created
├── README.md                      # ✅ Created
├── SETUP.md                       # ✅ This file
├── esp32_main_controller/         # ✅ Existing
│   ├── esp32_main_controller.ino
│   └── README.md
└── esp32_cam_face_recognition/    # ✅ Existing
    └── esp32cam/
        ├── esp32cam.ino
        ├── handlers.cpp
        ├── WifiCam.hpp
        └── README.md
```

---

## 🔗 Link Repository Lain

Setelah semua repo dibuat, tambahkan link di README:

### Backend Go Repository
```
https://github.com/Kysu-dev/backend_GO_iot
```

### Python Face Service Repository
```
https://github.com/Kysu-dev/smart-home-face-recognition
```

### Flutter App Repository
```
https://github.com/Kysu-dev/smart-home-flutter-app
```

---

## 📝 Best Practices

### Commit Messages
```
✅ Good:
- "Add DHT22 sensor support"
- "Fix: MQTT reconnection logic"
- "Update: WiFi config for dual band support"

❌ Bad:
- "update"
- "fix bug"
- "test"
```

### Branch Strategy (Optional)
```cmd
# Create feature branch
git checkout -b feature/add-new-sensor

# ... make changes ...

git add .
git commit -m "Add ultrasonic distance sensor support"
git push -u origin feature/add-new-sensor

# Merge ke main via Pull Request di GitHub
```

---

## 🚀 Quick Commands Reference

```cmd
# Clone repository
git clone https://github.com/Kysu-dev/smart-home-esp32-iot.git

# Pull latest changes
git pull

# Check status
git status

# View commit history
git log --oneline

# Create new branch
git checkout -b branch-name

# Switch branch
git checkout main

# Delete branch
git branch -d branch-name
```

---

## ⚠️ Important Notes

1. **Jangan commit file credentials!**
   - WiFi password
   - MQTT credentials
   - API keys
   
   (Sudah di-handle oleh `.gitignore`)

2. **File yang di-ignore:**
   - Build artifacts (*.bin, *.elf)
   - IDE configs (.vscode/, .idea/)
   - Local config files (secrets.h)

3. **Sebelum push:**
   - Test kode di ESP32
   - Pastikan tidak ada password hardcoded
   - Review dengan `git status` dan `git diff`

---

## 📞 Support

Jika ada masalah:
1. Check GitHub repository issues
2. Review README.md di masing-masing folder
3. Contact: [GitHub Profile](https://github.com/Kysu-dev)

---

**Created with ❤️ for Smart Home IoT Project**
