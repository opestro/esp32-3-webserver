# 🌟 ESP32 Cloud Bridge 🌟

A powerful bridge between your ESP32 sensor and the internet, enabling remote monitoring and control from anywhere in the world!

![ESP32 Cloud Bridge](https://via.placeholder.com/800x400/3498db/ffffff?text=ESP32+Cloud+Bridge)

## ✨ Features

- 🔄 **Real-time Data Sync**: Push sensor data from ESP32 to the cloud
- 📊 **Beautiful Dashboard**: Monitor temperature and humidity with interactive charts
- 🎨 **RGB LED Control**: Change colors and effects remotely
- 🔐 **Secure Authentication**: JWT for users, API keys for devices
- 📱 **Responsive Design**: Works on desktop, tablet, and mobile
- ⚡ **WebSocket Support**: Instant updates without page refresh
- 📈 **Historical Data**: Track and analyze sensor readings over time
- 🌙 **Dark Mode**: Easy on the eyes during night-time monitoring

## 🚀 Getting Started

### Prerequisites

- Node.js (v14+)
- npm or yarn
- ESP32-S3 with DHT11 sensor and RGB LED
- Arduino IDE with ESP32 support

### 📦 Installation

#### Server Setup

1. **Clone the repository**

```bash
git clone https://github.com/yourusername/esp32-cloud-bridge.git
cd esp32-cloud-bridge
```

2. **Install dependencies**

```bash
npm install
```

3. **Create environment variables**

Create a `.env` file in the root directory:

```
PORT=3000
JWT_SECRET=your-super-secret-key
DEVICE_API_KEY=your-device-api-key
```

4. **Start the server**

```bash
npm start
```

The server will be running at `http://localhost:3000`

#### ESP32 Setup

1. **Open Arduino IDE**

2. **Install required libraries**
   - WiFi
   - HTTPClient
   - ArduinoJson
   - Adafruit_NeoPixel
   - DHTesp

3. **Upload the code**
   - Open `esp32_cloud_client.cpp`
   - Update WiFi credentials and server URL
   - Upload to your ESP32

## 🏗️ Architecture

```
┌─────────────┐       ┌─────────────┐       ┌─────────────┐
│             │       │             │       │             │
│   ESP32     │◄─────►│   Server    │◄─────►│   Browser   │
│   Device    │       │   Bridge    │       │   Client    │
│             │       │             │       │             │
└─────────────┘       └─────────────┘       └─────────────┘
     Push Data           WebSockets            Real-time
     Get Commands        REST API              Dashboard
```

## 🔧 How It Works

### ESP32 Device

- 📤 Periodically sends sensor data to the server
- 📥 Checks for pending commands
- 🔄 Updates LED based on received commands
- 🔌 Automatically reconnects if WiFi connection is lost

### Server Bridge

- 🌉 Acts as a bridge between ESP32 and web clients
- 💾 Stores historical sensor data
- 📋 Queues commands for the ESP32
- 🔒 Handles authentication and security

### Web Dashboard

- 📊 Displays real-time and historical data
- 🎮 Provides controls for the RGB LED
- 🔔 Shows notifications for important events
- 🌓 Supports light and dark themes

## 📱 Screenshots

<div style="display: flex; justify-content: space-between;">
  <img src="https://via.placeholder.com/250x500/3498db/ffffff?text=Dashboard" width="30%" alt="Dashboard">
  <img src="https://via.placeholder.com/250x500/2ecc71/ffffff?text=Charts" width="30%" alt="Charts">
  <img src="https://via.placeholder.com/250x500/e74c3c/ffffff?text=Controls" width="30%" alt="Controls">
</div>

## 🔌 API Endpoints

### Device Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/device/data` | Send sensor data to server |
| GET | `/api/device/commands` | Get pending commands |

### Web Client Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| POST | `/api/login` | Authenticate user |
| GET | `/api/sensor-data` | Get latest sensor data |
| GET | `/api/history` | Get historical data |
| POST | `/api/led` | Control RGB LED |
| GET | `/api/led-status` | Get current LED status |
| GET | `/api/status` | Get system status |

## 🛠️ Customization

### Adding New Sensors

1. Update the ESP32 code to read from your sensor
2. Modify the JSON structure in `sendDataToServer()`
3. Update the server to handle the new data fields
4. Add visualization to the dashboard

### Changing LED Effects

Add new effects in the `updateLedEffect()` function and update the dropdown in the web interface.

## 🔒 Security Considerations

- 🔑 Change default admin password
- 🔐 Use strong, unique values for JWT_SECRET and DEVICE_API_KEY
- 🛡️ Consider enabling HTTPS for production deployments
- 🔍 Regularly check for library updates

## 📊 Data Storage

By default, data is stored in memory. For production use, consider implementing:

- 💾 MongoDB integration
- 📁 File-based storage
- ☁️ Cloud database services

## 🌐 Deployment

### Heroku Deployment

```bash
heroku create
git push heroku main
heroku config:set JWT_SECRET=your-secret-key
heroku config:set DEVICE_API_KEY=your-device-key
```

### Docker Deployment

```bash
docker build -t esp32-cloud-bridge .
docker run -p 3000:3000 -e JWT_SECRET=your-secret-key -e DEVICE_API_KEY=your-device-key esp32-cloud-bridge
```

## 📝 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgements

- [Express.js](https://expressjs.com/)
- [Socket.IO](https://socket.io/)
- [Chart.js](https://www.chartjs.org/)
- [Adafruit NeoPixel Library](https://github.com/adafruit/Adafruit_NeoPixel)
- [DHT Sensor Library for ESPx](https://github.com/beegee-tokyo/DHTesp)

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add some amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📬 Contact

Project Link: [https://github.com/opestro/esp32-3-webserver/](https://github.com/opestro/esp32-3-webserver)

---

<p align="center">
  Made by Mehdi H. with ❤️ for the IoT community <br/>
CSCClub of USDBLIDA
</p>
