const express = require('express');
const axios = require('axios');
const app = express();
const port = 3000;

// Enable JSON parsing for request bodies
app.use(express.json());

// Replace with your ESP32's IP address
const ESP_IP = '192.168.232.67';

// Store historical data
const dataHistory = {
  temperature: [],
  humidity: [],
  timestamps: []
};

// Maximum number of historical data points to store
const MAX_HISTORY = 100;

// Serve static files
app.use(express.static('public'));

// Route for the main page
app.get('/', (req, res) => {
    res.sendFile(__dirname + '/public/index.html');
});

// Route to match what the frontend is requesting
app.get('/sensor-data', async (req, res) => {
    try {
        const response = await axios.get(`http://${ESP_IP}/sensor-data`);
        const sensorData = response.data;
        
        // Add timestamp if not present
        if (!sensorData.timestamp) {
            sensorData.timestamp = Date.now();
        }
        
        // Store data in history
        dataHistory.temperature.push(sensorData.temperature);
        dataHistory.humidity.push(sensorData.humidity);
        dataHistory.timestamps.push(sensorData.timestamp);
        
        // Trim history if needed
        if (dataHistory.temperature.length > MAX_HISTORY) {
            dataHistory.temperature.shift();
            dataHistory.humidity.shift();
            dataHistory.timestamps.shift();
        }
        
        // Log data to console
        console.log(`[${new Date().toLocaleString()}] Temp: ${sensorData.temperature}°C, Humidity: ${sensorData.humidity}%`);
        
        res.json(sensorData);
    } catch (error) {
        console.error('Error fetching sensor data:', error.message);
        res.status(500).json({ 
            error: 'Failed to fetch sensor data',
            message: error.message 
        });
    }
});

// New endpoint to get historical data
app.get('/history', (req, res) => {
    res.json(dataHistory);
});

// LED control endpoint
app.post('/led', async (req, res) => {
    try {
        const ledData = req.body;
        console.log('Setting LED:', ledData);
        
        const response = await axios.post(`http://${ESP_IP}/led`, ledData);
        res.json(response.data);
    } catch (error) {
        console.error('Error setting LED:', error.message);
        res.status(500).json({
            error: 'Failed to set LED',
            message: error.message
        });
    }
});

// Get LED status
app.get('/led-status', async (req, res) => {
    try {
        const response = await axios.get(`http://${ESP_IP}/led-status`);
        res.json(response.data);
    } catch (error) {
        console.error('Error getting LED status:', error.message);
        res.status(500).json({
            error: 'Failed to get LED status',
            message: error.message
        });
    }
});

// Health check endpoint
app.get('/health', (req, res) => {
    res.status(200).send('OK');
});

// Start the server
app.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
    console.log(`ESP32 address: http://${ESP_IP}`);
});

// Handle graceful shutdown
process.on('SIGINT', () => {
    console.log('Server shutting down...');
    process.exit(0);
});