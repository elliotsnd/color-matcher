# Installation Guide

## Quick Start

1. **Navigate to the extension directory:**
   ```bash
   cd vscode-anthropic-completion
   ```

2. **Install dependencies:**
   ```bash
   npm install
   ```

3. **Compile the extension:**
   ```bash
   npm run compile
   ```

4. **Open in VS Code:**
   ```bash
   code .
   ```

5. **Launch extension development:**
   - Press `F5` to open a new VS Code window with the extension loaded
   - Or use the Command Palette: `Developer: Reload Window`

## Configuration

1. **Set your xAI API key:**
   - Open Settings (`Ctrl+,`)
   - Search for "xai-grok"
   - Enter your API key from [xAI Console](https://console.x.ai)

2. **Test the extension:**
   - Open the `example.md` file
   - Follow the test instructions
   - Use `Ctrl+Shift+G` for text completion
   - Use `Ctrl+Shift+S` for selection completion

## Package for Distribution

To create a `.vsix` package for distribution:

1. **Install vsce (VS Code Extension Manager):**
   ```bash
   npm install -g vsce
   ```

2. **Package the extension:**
   ```bash
   vsce package
   ```

3. **Install the packaged extension:**
   ```bash
   code --install-extension xai-grok-completion-1.0.0.vsix
   ```

## Troubleshooting

- **TypeScript errors:** Run `npm run compile` to check for compilation issues
- **Extension not loading:** Check the Developer Console (`Help > Toggle Developer Tools`)
- **API errors:** Verify your xAI API key is correct and has sufficient credits

## Development Workflow

1. Make changes to TypeScript files in `src/`
2. Run `npm run compile` or `npm run watch`
3. Reload the extension development window (`Ctrl+R`)
4. Test your changes
