const express = require('express');
const path = require('path');
const { spawn } = require('child_process');

const app = express();
const PORT = process.env.PORT || 8080;

// Serve static files
app.use(express.static('public'));
app.use(express.json());

// API endpoints for color processing
app.post('/api/process-color', (req, res) => {
    // Proxy to Python color processing scripts
    const python = spawn('python3', ['process_color.py'], {
        stdio: ['pipe', 'pipe', 'pipe']
    });
    
    python.stdin.write(JSON.stringify(req.body));
    python.stdin.end();
    
    let output = '';
    python.stdout.on('data', (data) => {
        output += data.toString();
    });
    
    python.on('close', (code) => {
        try {
            res.json(JSON.parse(output));
        } catch (e) {
            res.status(500).json({ error: 'Processing failed' });
        }
    });
});

// Health check endpoint
app.get('/health', (req, res) => {
    res.json({ status: 'healthy', timestamp: new Date().toISOString() });
});

// Catch-all handler
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.listen(PORT, '0.0.0.0', () => {
    console.log(`Color Matcher Web Interface running on port ${PORT}`);
});
