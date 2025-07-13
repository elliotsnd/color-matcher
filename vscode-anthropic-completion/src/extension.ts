import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import { XAIGrokClient, CompletionRequest } from './apiClient';

let grokClient: XAIGrokClient;

export function activate(context: vscode.ExtensionContext) {
    console.log('xAI Grok Completion extension is now active!');

    // Initialize the Grok client
    grokClient = new XAIGrokClient();

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

    // Enhanced commands for full codebase context
    const analyzeProjectCommand = vscode.commands.registerCommand('xai-grok.analyzeProject', async () => {
        await handleProjectAnalysis();
    });

    const fixCodeCommand = vscode.commands.registerCommand('xai-grok.fixCode', async () => {
        await handleCodeFix();
    });

    const explainCodeCommand = vscode.commands.registerCommand('xai-grok.explainCode', async () => {
        await handleCodeExplanation();
    });

    const refactorCommand = vscode.commands.registerCommand('xai-grok.refactor', async () => {
        await handleRefactor();
    });

    // Register configuration change listener
    const configChangeListener = vscode.workspace.onDidChangeConfiguration(event => {
        if (event.affectsConfiguration('xai-grok')) {
            grokClient.refreshConfiguration();
            vscode.window.showInformationMessage('xAI Grok configuration updated!');
        }
    });

    context.subscriptions.push(
        completeCommand,
        completeSelectionCommand,
        configureCommand,
        analyzeProjectCommand,
        fixCodeCommand,
        explainCodeCommand,
        refactorCommand,
        configChangeListener
    );
}

async function handleCompletion(useSelection: boolean) {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No active editor found');
        return;
    }

    let prompt: string;
    let insertPosition: vscode.Position;

    if (useSelection && editor.selection && !editor.selection.isEmpty) {
        // Use selected text as prompt
        prompt = editor.document.getText(editor.selection);
        insertPosition = editor.selection.end;
    } else {
        // Use text from beginning of document to cursor as context
        const cursorPosition = editor.selection.active;
        const textBeforeCursor = editor.document.getText(
            new vscode.Range(new vscode.Position(0, 0), cursorPosition)
        );
        
        // For completion, we'll use the context and ask for continuation
        const projectContext = await getProjectContext();
        prompt = `PROJECT CONTEXT:
${projectContext}

CURRENT FILE CONTENT:
${textBeforeCursor}

Continue the above code considering the full project context. Generate only the code that should come next, without repeating existing code:`;
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
            const request: CompletionRequest = {
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

            vscode.window.showInformationMessage(
                `Completion generated (${response.completion.length} characters)`
            );

        } catch (error) {
            vscode.window.showErrorMessage(
                `Failed to generate completion: ${error instanceof Error ? error.message : String(error)}`
            );
        }
    });
}

async function showConfigurationDialog() {
    const config = vscode.workspace.getConfiguration('xai-grok');
    
    const items = [
        {
            label: 'API Key',
            description: config.get<string>('apiKey') ? 'Configured' : 'Not set',
            action: 'apiKey'
        },
        {
            label: 'Model',
            description: config.get<string>('model', 'grok-4'),
            action: 'model'
        },
        {
            label: 'SDK Type',
            description: config.get<string>('sdkType', 'openai'),
            action: 'sdkType'
        },
        {
            label: 'Max Tokens',
            description: config.get<number>('maxTokens', 100).toString(),
            action: 'maxTokens'
        },
        {
            label: 'Temperature',
            description: config.get<number>('temperature', 0.1).toString(),
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
                value: config.get<string>('apiKey', '')
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
                value: config.get<number>('maxTokens', 100).toString(),
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
                value: config.get<number>('temperature', 0.1).toString(),
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

async function handleProjectAnalysis() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No active editor found');
        return;
    }

    await vscode.window.withProgress({
        location: vscode.ProgressLocation.Notification,
        title: "Analyzing project with Grok...",
        cancellable: true
    }, async (progress, token) => {
        try {
            const projectContext = await getProjectContext();
            const currentFile = editor.document.getText();
            const fileName = path.basename(editor.document.fileName);

            const prompt = `Analyze this project and provide insights:

PROJECT CONTEXT:
${projectContext}

CURRENT FILE (${fileName}):
${currentFile}

Please provide:
1. Project overview and architecture
2. Code quality assessment
3. Potential improvements
4. Security considerations
5. Performance optimizations
6. Best practices recommendations`;

            const request: CompletionRequest = { prompt: prompt };
            const response = await grokClient.complete(request);

            // Show analysis in a new document
            const doc = await vscode.workspace.openTextDocument({
                content: response.completion,
                language: 'markdown'
            });
            await vscode.window.showTextDocument(doc);

        } catch (error) {
            vscode.window.showErrorMessage(`Analysis failed: ${error instanceof Error ? error.message : String(error)}`);
        }
    });
}

async function handleCodeFix() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No active editor found');
        return;
    }

    const selection = editor.selection;
    const selectedText = editor.document.getText(selection);

    if (!selectedText.trim()) {
        vscode.window.showErrorMessage('Please select code to fix');
        return;
    }

    await vscode.window.withProgress({
        location: vscode.ProgressLocation.Notification,
        title: "Fixing code with Grok...",
        cancellable: true
    }, async (progress, token) => {
        try {
            const projectContext = await getProjectContext();
            const fullFile = editor.document.getText();
            const fileName = path.basename(editor.document.fileName);

            const prompt = `Fix the selected code considering the full project context:

PROJECT CONTEXT:
${projectContext}

FULL FILE (${fileName}):
${fullFile}

SELECTED CODE TO FIX:
${selectedText}

Please provide:
1. Fixed code
2. Explanation of issues found
3. Improvements made
4. Any additional recommendations

Format the response as:
## Fixed Code:
\`\`\`
[fixed code here]
\`\`\`

## Issues Found:
[explanation]

## Improvements Made:
[details]`;

            const request: CompletionRequest = { prompt: prompt };
            const response = await grokClient.complete(request);

            // Show fix in a new document
            const doc = await vscode.workspace.openTextDocument({
                content: response.completion,
                language: 'markdown'
            });
            await vscode.window.showTextDocument(doc, vscode.ViewColumn.Beside);

        } catch (error) {
            vscode.window.showErrorMessage(`Code fix failed: ${error instanceof Error ? error.message : String(error)}`);
        }
    });
}

async function handleCodeExplanation() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No active editor found');
        return;
    }

    const selection = editor.selection;
    const selectedText = editor.document.getText(selection);
    const textToExplain = selectedText.trim() || editor.document.getText();

    await vscode.window.withProgress({
        location: vscode.ProgressLocation.Notification,
        title: "Explaining code with Grok...",
        cancellable: true
    }, async (progress, token) => {
        try {
            const projectContext = await getProjectContext();
            const fileName = path.basename(editor.document.fileName);

            const prompt = `Explain this code in the context of the full project:

PROJECT CONTEXT:
${projectContext}

CODE TO EXPLAIN (from ${fileName}):
${textToExplain}

Please provide:
1. High-level overview
2. Line-by-line explanation
3. How it fits in the project
4. Dependencies and relationships
5. Potential issues or improvements`;

            const request: CompletionRequest = { prompt: prompt };
            const response = await grokClient.complete(request);

            // Show explanation in a new document
            const doc = await vscode.workspace.openTextDocument({
                content: response.completion,
                language: 'markdown'
            });
            await vscode.window.showTextDocument(doc, vscode.ViewColumn.Beside);

        } catch (error) {
            vscode.window.showErrorMessage(`Code explanation failed: ${error instanceof Error ? error.message : String(error)}`);
        }
    });
}

async function handleRefactor() {
    const editor = vscode.window.activeTextEditor;
    if (!editor) {
        vscode.window.showErrorMessage('No active editor found');
        return;
    }

    const selection = editor.selection;
    const selectedText = editor.document.getText(selection);

    if (!selectedText.trim()) {
        vscode.window.showErrorMessage('Please select code to refactor');
        return;
    }

    await vscode.window.withProgress({
        location: vscode.ProgressLocation.Notification,
        title: "Refactoring code with Grok...",
        cancellable: true
    }, async (progress, token) => {
        try {
            const projectContext = await getProjectContext();
            const fullFile = editor.document.getText();
            const fileName = path.basename(editor.document.fileName);

            const prompt = `Refactor the selected code considering the full project context:

PROJECT CONTEXT:
${projectContext}

FULL FILE (${fileName}):
${fullFile}

CODE TO REFACTOR:
${selectedText}

Please provide:
1. Refactored code with improvements
2. Explanation of changes made
3. Benefits of the refactoring
4. Any breaking changes or considerations

Format as:
## Refactored Code:
\`\`\`
[refactored code]
\`\`\`

## Changes Made:
[explanation]

## Benefits:
[details]`;

            const request: CompletionRequest = { prompt: prompt };
            const response = await grokClient.complete(request);

            // Show refactored code in a new document
            const doc = await vscode.workspace.openTextDocument({
                content: response.completion,
                language: 'markdown'
            });
            await vscode.window.showTextDocument(doc, vscode.ViewColumn.Beside);

        } catch (error) {
            vscode.window.showErrorMessage(`Refactoring failed: ${error instanceof Error ? error.message : String(error)}`);
        }
    });
}

// Enhanced functions for full codebase context
async function getProjectContext(): Promise<string> {
    const workspaceFolder = vscode.workspace.workspaceFolders?.[0];
    if (!workspaceFolder) {
        return '';
    }

    let context = '';
    const projectPath = workspaceFolder.uri.fsPath;

    // Get project structure
    context += '=== PROJECT STRUCTURE ===\n';
    context += await getDirectoryStructure(projectPath, 0, 3);

    // Get key files content
    context += '\n=== KEY FILES ===\n';
    const keyFiles = await findKeyFiles(projectPath);
    for (const file of keyFiles) {
        try {
            const content = fs.readFileSync(file, 'utf8');
            const relativePath = path.relative(projectPath, file);
            context += `\n--- ${relativePath} ---\n`;
            context += content.substring(0, 2000); // Limit file size
            context += '\n';
        } catch (error) {
            // Skip files that can't be read
        }
    }

    return context;
}

async function getDirectoryStructure(dirPath: string, depth: number, maxDepth: number): Promise<string> {
    if (depth > maxDepth) return '';

    let structure = '';
    const indent = '  '.repeat(depth);

    try {
        const items = fs.readdirSync(dirPath);
        for (const item of items) {
            if (item.startsWith('.') || item === 'node_modules' || item === 'out') continue;

            const itemPath = path.join(dirPath, item);
            const stat = fs.statSync(itemPath);

            if (stat.isDirectory()) {
                structure += `${indent}📁 ${item}/\n`;
                structure += await getDirectoryStructure(itemPath, depth + 1, maxDepth);
            } else {
                structure += `${indent}📄 ${item}\n`;
            }
        }
    } catch (error) {
        // Skip directories that can't be read
    }

    return structure;
}

async function findKeyFiles(projectPath: string): Promise<string[]> {
    const keyFiles: string[] = [];
    const extensions = ['.cpp', '.h', '.js', '.ts', '.tsx', '.html', '.css', '.json', '.md', '.py'];

    function scanDirectory(dirPath: string, depth: number = 0) {
        if (depth > 3) return; // Limit depth

        try {
            const items = fs.readdirSync(dirPath);
            for (const item of items) {
                if (item.startsWith('.') || item === 'node_modules' || item === 'out') continue;

                const itemPath = path.join(dirPath, item);
                const stat = fs.statSync(itemPath);

                if (stat.isDirectory()) {
                    scanDirectory(itemPath, depth + 1);
                } else if (extensions.some(ext => item.endsWith(ext))) {
                    keyFiles.push(itemPath);
                }
            }
        } catch (error) {
            // Skip directories that can't be read
        }
    }

    scanDirectory(projectPath);
    return keyFiles.slice(0, 20); // Limit number of files
}

export function deactivate() {
    console.log('xAI Grok Completion extension is now deactivated');
}
