"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.deactivate = exports.activate = void 0;
const vscode = require("vscode");
const apiClient_1 = require("./apiClient");
let grokClient;
function activate(context) {
    console.log('xAI Grok Completion extension is now active!');
    // Initialize the Grok client
    grokClient = new apiClient_1.XAIGrokClient();
    // Register commands
    const completeCommand = vscode.commands.registerCommand('xai-grok.complete', async () => {
        await handleCompletion(false);
    });
    const completeSelectionCommand = vscode.commands.registerCommand('xai-grok.completeSelection', async () => {
        await handleCompletion(true);
    });
    const configureCommand = vscode.commands.registerCommand('xai-grok.configure', async () => {
        await showConfigurationDialog();
    });
    // Register configuration change listener
    const configChangeListener = vscode.workspace.onDidChangeConfiguration(event => {
        if (event.affectsConfiguration('xai-grok')) {
            grokClient.refreshConfiguration();
            vscode.window.showInformationMessage('xAI Grok configuration updated!');
        }
    });
    context.subscriptions.push(completeCommand, completeSelectionCommand, configureCommand, configChangeListener);
}
exports.activate = activate;
async function handleCompletion(useSelection) {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No active editor found');
        return;
    }
    let prompt;
    let insertPosition;
    if (useSelection && editor.selection && !editor.selection.isEmpty) {
        // Use selected text as prompt
        prompt = editor.document.getText(editor.selection);
        insertPosition = editor.selection.end;
    }
    else {
        // Use text from beginning of document to cursor as context
        const cursorPosition = editor.selection.active;
        const textBeforeCursor = editor.document.getText(new vscode.Range(new vscode.Position(0, 0), cursorPosition));
        // For completion, we'll use the context and ask for continuation
        prompt = textBeforeCursor + "\n\nContinue the above text:";
        insertPosition = cursorPosition;
    }
    if (!prompt.trim()) {
        vscode.window.showErrorMessage('No text to complete');
        return;
    }
    // Show progress indicator
    await vscode.window.withProgress({
        location: vscode.ProgressLocation.Notification,
        title: "Generating completion with Grok...",
        cancellable: true
    }, async (progress, token) => {
        try {
            const request = {
                prompt: prompt
            };
            const response = await grokClient.complete(request);
            if (token.isCancellationRequested) {
                return;
            }
            // Insert the completion at the cursor position
            await editor.edit(editBuilder => {
                editBuilder.insert(insertPosition, response.completion);
            });
            vscode.window.showInformationMessage(`Completion generated (${response.completion.length} characters)`);
        }
        catch (error) {
            vscode.window.showErrorMessage(`Failed to generate completion: ${error instanceof Error ? error.message : String(error)}`);
        }
    });
}
async function showConfigurationDialog() {
    const config = vscode.workspace.getConfiguration('xai-grok');
    const items = [
        {
            label: 'API Key',
            description: config.get('apiKey') ? 'Configured' : 'Not set',
            action: 'apiKey'
        },
        {
            label: 'Model',
            description: config.get('model', 'grok-4'),
            action: 'model'
        },
        {
            label: 'SDK Type',
            description: config.get('sdkType', 'openai'),
            action: 'sdkType'
        },
        {
            label: 'Max Tokens',
            description: config.get('maxTokens', 100).toString(),
            action: 'maxTokens'
        },
        {
            label: 'Temperature',
            description: config.get('temperature', 0.1).toString(),
            action: 'temperature'
        }
    ];
    const selected = await vscode.window.showQuickPick(items, {
        placeHolder: 'Select setting to configure'
    });
    if (!selected) {
        return;
    }
    switch (selected.action) {
        case 'apiKey':
            const apiKey = await vscode.window.showInputBox({
                prompt: 'Enter your xAI API key',
                password: true,
                value: config.get('apiKey', '')
            });
            if (apiKey !== undefined) {
                await config.update('apiKey', apiKey, vscode.ConfigurationTarget.Global);
            }
            break;
        case 'model':
            const model = await vscode.window.showQuickPick(['grok-4', 'grok-3', 'grok-2'], {
                placeHolder: 'Select Grok model'
            });
            if (model) {
                await config.update('model', model, vscode.ConfigurationTarget.Global);
            }
            break;
        case 'sdkType':
            const sdkType = await vscode.window.showQuickPick(['openai', 'anthropic'], {
                placeHolder: 'Select SDK compatibility mode'
            });
            if (sdkType) {
                await config.update('sdkType', sdkType, vscode.ConfigurationTarget.Global);
            }
            break;
        case 'maxTokens':
            const maxTokens = await vscode.window.showInputBox({
                prompt: 'Enter maximum tokens to generate',
                value: config.get('maxTokens', 100).toString(),
                validateInput: (value) => {
                    const num = parseInt(value);
                    return isNaN(num) || num <= 0 ? 'Please enter a positive number' : null;
                }
            });
            if (maxTokens) {
                await config.update('maxTokens', parseInt(maxTokens), vscode.ConfigurationTarget.Global);
            }
            break;
        case 'temperature':
            const temperature = await vscode.window.showInputBox({
                prompt: 'Enter temperature (0.0 - 2.0)',
                value: config.get('temperature', 0.1).toString(),
                validateInput: (value) => {
                    const num = parseFloat(value);
                    return isNaN(num) || num < 0 || num > 2 ? 'Please enter a number between 0 and 2' : null;
                }
            });
            if (temperature) {
                await config.update('temperature', parseFloat(temperature), vscode.ConfigurationTarget.Global);
            }
            break;
    }
}
function deactivate() {
    console.log('xAI Grok Completion extension is now deactivated');
}
exports.deactivate = deactivate;
//# sourceMappingURL=extension.js.map