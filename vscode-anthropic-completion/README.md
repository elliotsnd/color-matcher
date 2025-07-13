# xAI Grok Completion for VS Code

A VS Code extension that provides text completion using xAI's Grok models with support for both OpenAI and Anthropic SDK compatibility.

## Features

- **Text Completion**: Generate text completions using Grok models
- **Selection Completion**: Complete selected text or use it as a prompt
- **Multiple SDK Support**: Compatible with both OpenAI and Anthropic SDKs
- **Configurable Settings**: Customize model, temperature, max tokens, and more
- **Keyboard Shortcuts**: Quick access via keyboard shortcuts

## Installation

### Prerequisites

- VS Code 1.74.0 or higher
- Node.js and npm
- xAI API key (get from [xAI Console](https://console.x.ai))

### Setup

1. Clone or download this extension to your local machine
2. Open the extension folder in VS Code
3. Install dependencies:
   ```bash
   npm install
   ```
4. Compile the TypeScript:
   ```bash
   npm run compile
   ```
5. Press `F5` to launch a new VS Code window with the extension loaded

## Configuration

### API Key Setup

1. Open VS Code settings (`Ctrl+,` or `Cmd+,`)
2. Search for "xai-grok"
3. Set your xAI API key in the "Api Key" field

Or use the command palette:
1. Press `Ctrl+Shift+P` (or `Cmd+Shift+P` on Mac)
2. Type "xAI Grok: Configure API Settings"
3. Select "API Key" and enter your key

### Available Settings

- **API Endpoint**: Default is `https://api.x.ai/v1`
- **Model**: Choose from `grok-4`, `grok-3`, or `grok-2`
- **SDK Type**: Select `openai` or `anthropic` compatibility mode
- **Max Tokens**: Maximum tokens to generate (default: 100)
- **Temperature**: Controls randomness (0.0-2.0, default: 0.1)

## Usage

### Commands

- **Complete Text** (`Ctrl+Shift+G` / `Cmd+Shift+G`): Generate completion from cursor position
- **Complete Selected Text** (`Ctrl+Shift+S` / `Cmd+Shift+S`): Use selected text as prompt
- **Configure API Settings**: Open configuration dialog

### How to Use

1. **Text Completion**:
   - Place your cursor where you want to generate text
   - Press `Ctrl+Shift+G` (or use Command Palette)
   - The extension will use text before the cursor as context

2. **Selection Completion**:
   - Select text you want to complete or use as a prompt
   - Press `Ctrl+Shift+S` (or use Command Palette)
   - The completion will be inserted after the selection

### Example Workflow

```javascript
// Type some code and place cursor at the end
function calculateSum(a, b) {
    // Press Ctrl+Shift+G here
```

The extension will generate a completion like:
```javascript
function calculateSum(a, b) {
    return a + b;
}
```

## SDK Compatibility

This extension supports both OpenAI and Anthropic SDK patterns:

### OpenAI Mode (Default)
- Uses chat completions format
- Compatible with OpenAI SDK patterns
- Recommended for better stability

### Anthropic Mode
- Uses messages format
- Compatible with Anthropic SDK patterns
- Alternative option for Anthropic users

## Development

### Building from Source

```bash
# Install dependencies
npm install

# Compile TypeScript
npm run compile

# Watch for changes during development
npm run watch
```

### Project Structure

```
├── src/
│   ├── extension.ts      # Main extension logic
│   └── apiClient.ts      # xAI API client with dual SDK support
├── package.json          # Extension manifest
├── tsconfig.json         # TypeScript configuration
└── README.md            # This file
```

## Troubleshooting

### Common Issues

1. **"API key not configured"**
   - Set your xAI API key in VS Code settings
   - Get your key from [xAI Console](https://console.x.ai)

2. **"Completion failed"**
   - Check your internet connection
   - Verify your API key is valid
   - Check if you have sufficient API credits

3. **Extension not loading**
   - Ensure VS Code version is 1.74.0 or higher
   - Try reloading the window (`Ctrl+R`)

### Debug Mode

1. Open the extension development window (`F5`)
2. Open Developer Tools (`Help > Toggle Developer Tools`)
3. Check the Console tab for error messages

## API Reference

The extension uses xAI's API which is compatible with both OpenAI and Anthropic formats:

- **Endpoint**: `https://api.x.ai/v1`
- **Models**: `grok-4`, `grok-3`, `grok-2`
- **Authentication**: Bearer token (your xAI API key)

## License

MIT License - see LICENSE file for details

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## Support

For issues and questions:
- Check the troubleshooting section above
- Open an issue on the project repository
- Refer to [xAI documentation](https://docs.x.ai)
