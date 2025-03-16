require('dotenv').config();
const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const cors = require('cors');
const bodyParser = require('body-parser');
const jwt = require('jsonwebtoken');
const bcrypt = require('bcrypt');
const helmet = require('helmet');
const compression = require('compression');
const path = require('path');

// Configuration
const PORT = process.env.PORT || 3000;
const JWT_SECRET = process.env.JWT_SECRET || '123456789';
const DEVICE_API_KEY = process.env.DEVICE_API_KEY || '123456789';

// Create Express app
const app = express();
const server = http.createServer(app);
const io = socketIo(server, {
  cors: {
    origin: '*',
    methods: ['GET', 'POST']
  }
});

// Middleware
app.use(helmet());
app.use(compression());
app.use(cors());
app.use(bodyParser.json());
app.use(express.static('public'));

// In-memory data store (replace with MongoDB in production)
const dataStore = {
  sensorData: [],
  ledState: {
    r: 0,
    g: 150,
    b: 0,
    brightness: 128,
    effect: "solid"
  },
  pendingCommands: [],
  users: [
    // Default admin user - change in production!
    {
      username: 'admin',
      // Default password: 'admin123'
      passwordHash: '$2b$10$eCc0KVXjKXYMkWNbpYHwB.Dh5hkFV3UVvhvzLk4/aQDJ0Ey1Mgkxm',
      role: 'admin'
    }
  ],
  maxDataPoints: 1000
};

// Connected clients
const connectedClients = new Set();
let lastEspData = null;
let espConnected = false;
let lastEspCheck = Date.now();

// Authentication middleware for web clients
const authenticateUser = (req, res, next) => {
  const token = req.headers.authorization?.split(' ')[1];
  
  if (!token) {
    return res.status(401).json({ error: 'Authentication required' });
  }
  
  try {
    const decoded = jwt.verify(token, JWT_SECRET);
    req.user = decoded;
    next();
  } catch (error) {
    return res.status(401).json({ error: 'Invalid token' });
  }
};

// Authentication middleware for ESP32 device
const authenticateDevice = (req, res, next) => {
  const apiKey = req.headers['x-api-key'];
  
  if (!apiKey || apiKey !== DEVICE_API_KEY) {
    return res.status(401).json({ error: 'Invalid device API key' });
  }
  
  next();
};

// Routes

// Login route for web clients
app.post('/api/login', async (req, res) => {
  const { username, password } = req.body;
  
  const user = dataStore.users.find(u => u.username === username);
  if (!user) {
    return res.status(401).json({ error: 'Invalid credentials' });
  }
  
  const passwordMatch = await bcrypt.compare(password, user.passwordHash);
  if (!passwordMatch) {
    return res.status(401).json({ error: 'Invalid credentials' });
  }
  
  const token = jwt.sign(
    { username, role: user.role },
    JWT_SECRET,
    { expiresIn: '24h' }
  );
  
  res.json({ token, username, role: user.role });
});

// ESP32 sends sensor data to this endpoint
app.post('/api/device/data', authenticateDevice, (req, res) => {
  const sensorData = req.body;
  
  // Add timestamp if not present
  if (!sensorData.timestamp) {
    sensorData.timestamp = Date.now();
  }
  
  // Update last contact info
  lastEspData = sensorData;
  lastEspCheck = Date.now();
  espConnected = true;
  
  // Store data
  dataStore.sensorData.push(sensorData);
  
  // Limit stored data
  if (dataStore.sensorData.length > dataStore.maxDataPoints) {
    dataStore.sensorData.shift();
  }
  
  // Broadcast to all connected clients
  io.emit('sensor-update', sensorData);
  
  // Check if there are any pending commands for the device
  const pendingCommands = [...dataStore.pendingCommands];
  dataStore.pendingCommands = []; // Clear pending commands
  
  // Send response with any pending commands
  res.json({
    status: 'ok',
    pendingCommands: pendingCommands
  });
});

// ESP32 checks for commands
app.get('/api/device/commands', authenticateDevice, (req, res) => {
  // Send any pending commands to the device
  const pendingCommands = [...dataStore.pendingCommands];
  dataStore.pendingCommands = []; // Clear pending commands
  
  // Update last contact info
  lastEspCheck = Date.now();
  espConnected = true;
  
  res.json({
    status: 'ok',
    pendingCommands: pendingCommands
  });
});

// Web clients get sensor data
app.get('/api/sensor-data', (req, res) => {
  if (lastEspData) {
    res.json({
      ...lastEspData,
      lastContact: lastEspCheck,
      connected: espConnected
    });
  } else {
    res.status(404).json({ error: 'No sensor data available yet' });
  }
});

// Get historical data
app.get('/api/history', (req, res) => {
  // Optional: filter by time range
  const { hours } = req.query;
  let filteredData = dataStore.sensorData;
  
  if (hours) {
    const hoursAgo = Date.now() - (parseInt(hours) * 60 * 60 * 1000);
    filteredData = dataStore.sensorData.filter(item => item.timestamp >= hoursAgo);
  }
  
  res.json(filteredData);
});

// Web clients control LED
app.post('/api/led', (req, res) => {
  const ledData = req.body;
  console.log('Setting LED:', ledData);
  
  // Update stored LED state
  dataStore.ledState = {
    ...dataStore.ledState,
    ...ledData
  };
  
  // Add command to pending queue for ESP32
  dataStore.pendingCommands.push({
    type: 'led',
    data: dataStore.ledState
  });
  
  // Broadcast LED change to all clients
  io.emit('led-update', dataStore.ledState);
  
  res.json({
    status: 'ok',
    message: 'LED command queued for device',
    ledState: dataStore.ledState
  });
});

// Get LED status
app.get('/api/led-status', (req, res) => {
  res.json(dataStore.ledState);
});

// System status
app.get('/api/status', (req, res) => {
  res.json({
    espConnected,
    lastContact: lastEspCheck,
    clientsConnected: connectedClients.size,
    uptime: process.uptime(),
    memoryUsage: process.memoryUsage(),
    pendingCommands: dataStore.pendingCommands.length
  });
});

// Admin routes (protected)
app.get('/api/admin/users', authenticateUser, (req, res) => {
  if (req.user.role !== 'admin') {
    return res.status(403).json({ error: 'Unauthorized' });
  }
  
  // Return user list without password hashes
  const users = dataStore.users.map(({ username, role }) => ({ username, role }));
  res.json(users);
});

// Serve the frontend
app.get('*', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Socket.IO connection handling
io.on('connection', (socket) => {
  console.log('Client connected:', socket.id);
  connectedClients.add(socket.id);
  
  // Send initial data
  if (lastEspData) {
    socket.emit('sensor-update', lastEspData);
    socket.emit('led-update', dataStore.ledState);
    socket.emit('esp-status', { connected: espConnected });
  }
  
  // Handle LED control from socket
  socket.on('set-led', (ledData) => {
    // Update stored LED state
    dataStore.ledState = {
      ...dataStore.ledState,
      ...ledData
    };
    
    // Add command to pending queue for ESP32
    dataStore.pendingCommands.push({
      type: 'led',
      data: dataStore.ledState
    });
    
    // Broadcast to all clients
    io.emit('led-update', dataStore.ledState);
  });
  
  // Handle disconnection
  socket.on('disconnect', () => {
    console.log('Client disconnected:', socket.id);
    connectedClients.delete(socket.id);
  });
});

// Check ESP32 connection status
setInterval(() => {
  // If we haven't heard from ESP32 in 2 minutes, mark as disconnected
  if (Date.now() - lastEspCheck > 120000 && espConnected) {
    console.log('ESP32 connection timed out');
    espConnected = false;
    io.emit('esp-status', { connected: false });
  }
}, 30000);

// Start the server
server.listen(PORT, () => {
  console.log(`Cloud bridge server running at http://localhost:${PORT}`);
});

// Handle graceful shutdown
process.on('SIGINT', () => {
  console.log('Server shutting down...');
  server.close(() => {
    console.log('Server closed');
    process.exit(0);
  });
}); 