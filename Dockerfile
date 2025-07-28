# Google Cloud Run - ESP32 Color Sensor Web Interface
FROM node:18-alpine

# Install system dependencies
RUN apk add --no-cache \
    python3 \
    py3-pip \
    curl \
    && rm -rf /var/cache/apk/*

# Set working directory
WORKDIR /app

# Copy and install Node.js dependencies first (for better caching)
COPY package*.json ./
RUN npm install express

# Copy web interface files
COPY data/ ./public/

# Copy application files
COPY server.js process_color.py ./

# Make Python script executable
RUN chmod +x process_color.py

# Expose port
EXPOSE 8080

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Start the server
CMD ["npm", "start"]
