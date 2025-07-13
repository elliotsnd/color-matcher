import React, { useState, useEffect } from 'react';
import { createRoot } from 'react-dom/client';

// --- TYPES ---
interface ColorData {
    id: number;
    name: string;
    rgb: { r: number, g: number, b: number };
    hex: string;
}



// --- UTILITY ---
const rgbToHex = (r: number, g: number, b: number): string => {
    return "#" + [r, g, b].map(x => {
        const hex = x.toString(16);
        return hex.length === 1 ? '0' + hex : hex;
    }).join('');
};

// --- COMPONENTS ---
const App: React.FC = () => {
    const [liveColor, setLiveColor] = useState<{ rgb: { r: number; g: number; b: number }; hex: string; name?: string } | null>(null);
    const [scannedColor, setScannedColor] = useState<ColorData | null>(null);
    const [savedColors, setSavedColors] = useState<ColorData[]>([]);

    // Load saved colors from localStorage on initial render
    useEffect(() => {
        try {
            const storedColors = localStorage.getItem('savedColors');
            if (storedColors) {
                setSavedColors(JSON.parse(storedColors));
            }
        } catch (e) {
            console.error("Failed to load colors from localStorage", e);
        }
    }, []);
    
    // Poll for live color (now uses server colorName)
    useEffect(() => {
        const fetchLive = async () => {
            try {
                const res = await fetch('/api/color');
                const data = await res.json();
                const hex = rgbToHex(data.r, data.g, data.b);
                setLiveColor({
                    rgb: { r: data.r, g: data.g, b: data.b },
                    hex,
                    name: data.colorName || 'Unknown Color'
                });
            } catch (e) {
                console.error('API error:', e);
            }
        };
        const interval = setInterval(fetchLive, 500); // Fast poll
        fetchLive();
        return () => clearInterval(interval);
    }, []);


    const handleCapture = () => {
        if (!liveColor) return;
        setScannedColor({ ...liveColor, id: Date.now() }); // Use live's Dulux name
    };

    const handleSave = () => {
        if (scannedColor && !savedColors.some(c => c.id === scannedColor.id)) {
            const newSaved = [scannedColor, ...savedColors];
            setSavedColors(newSaved);
            localStorage.setItem('savedColors', JSON.stringify(newSaved));
            setScannedColor(null);
            // Success feedback
            alert('Color saved successfully!'); // Or use toast library
        }
    };

    const handleDelete = (id: number) => {
        const newSavedColors = savedColors.filter(color => color.id !== id);
        setSavedColors(newSavedColors);
        localStorage.setItem('savedColors', JSON.stringify(newSavedColors));
    };

    return (
        <main>
            <h1><i className="fas fa-palette"></i> Color Matcher</h1>
            <div className="card scanner-control">
                <button className="scan-button" onClick={handleCapture} disabled={!liveColor}>
                    <i className="fas fa-camera"></i> Capture Color
                </button>
            </div>

            <div className="view-container">
                <div className="card live-view">
                    <h2><i className="fas fa-eye"></i> Live View</h2>
                    {liveColor ? (
                        <div className="color-display">
                            <div className="color-swatch live-swatch" style={{ backgroundColor: liveColor.hex }}></div>
                            <div className="color-details">
                                <p className="color-name">{liveColor.name || 'Live Sensor Feed'}</p>
                                <p className="color-values">RGB: {`${liveColor.rgb.r}, ${liveColor.rgb.g}, ${liveColor.rgb.b}`}</p>
                                <p className="color-values">HEX: {liveColor.hex}</p>
                            </div>
                        </div>
                    ) : (
                        <div className="placeholder-container">
                            <i className="fas fa-spinner fa-spin"></i>
                            <p className="placeholder-text">Initializing Sensor...</p>
                        </div>
                    )}
                </div>
                <div className="card scanned-view">
                    <h2><i className="fas fa-crosshairs"></i> Last Scanned</h2>
                     {scannedColor ? (
                        <div className="color-display">
                            <div className="color-swatch" style={{ backgroundColor: scannedColor.hex }}></div>
                            <div className="color-details">
                                <p className="color-name">{scannedColor.name}</p>
                                <p className="color-values">RGB: {`${scannedColor.rgb.r}, ${scannedColor.rgb.g}, ${scannedColor.rgb.b}`}</p>
                                <p className="color-values">HEX: {scannedColor.hex}</p>
                                <button className="save-button" onClick={handleSave}><i className="fas fa-save"></i> Save Sample</button>
                            </div>
                        </div>
                    ) : (
                        <div className="placeholder-container">
                            <i className="fas fa-palette"></i>
                            <p className="placeholder-text">Capture a color to see it here</p>
                        </div>
                    )}
                </div>
            </div>


            {savedColors.length > 0 && (
                <div className="card saved-colors-section">
                    <h2><i className="fas fa-bookmark"></i> Saved Colors</h2>
                    <div className="saved-colors-grid">
                        {savedColors.map(color => (
                            <div className="saved-color-card" key={color.id}>
                                <div className="saved-color-swatch" style={{ backgroundColor: color.hex }}></div>
                                <div className="saved-color-info">
                                    <p className="saved-color-name">{color.name}</p>
                                    <p className="saved-color-values">{color.hex}</p>
                                </div>
                                <button className="delete-button" onClick={() => handleDelete(color.id)}><i className="fas fa-trash"></i></button>
                            </div>
                        ))}
                    </div>
                </div>
            )}
        </main>
    );
};

const container = document.getElementById('root');
if (container) {
    const root = createRoot(container);
    root.render(<React.StrictMode><App /></React.StrictMode>);
}