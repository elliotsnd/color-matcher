"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.XAIGrokClient = void 0;
const vscode = require("vscode");
const openai_1 = require("openai");
const sdk_1 = require("@anthropic-ai/sdk");
class XAIGrokClient {
    constructor() {
        this.openaiClient = null;
        this.anthropicClient = null;
        this.config = vscode.workspace.getConfiguration('xai-grok');
        this.initializeClients();
    }
    initializeClients() {
        const apiKey = this.config.get('apiKey');
        const apiEndpoint = this.config.get('apiEndpoint', 'https://api.x.ai/v1');
        if (!apiKey) {
            vscode.window.showWarningMessage('xAI API key not configured. Please set it in settings.');
            return;
        }
        // Initialize OpenAI client
        this.openaiClient = new openai_1.default({
            apiKey: apiKey,
            baseURL: apiEndpoint,
        });
        // Initialize Anthropic client
        this.anthropicClient = new sdk_1.default({
            apiKey: apiKey,
            baseURL: apiEndpoint.replace('/v1', ''), // Anthropic SDK expects base without /v1
        });
    }
    async complete(request) {
        const sdkType = this.config.get('sdkType', 'openai');
        const model = request.model || this.config.get('model', 'grok-4');
        const maxTokens = request.maxTokens || this.config.get('maxTokens', 100);
        const temperature = request.temperature || this.config.get('temperature', 0.1);
        try {
            if (sdkType === 'anthropic') {
                return await this.completeWithAnthropic(request.prompt, model, maxTokens, temperature);
            }
            else {
                return await this.completeWithOpenAI(request.prompt, model, maxTokens, temperature);
            }
        }
        catch (error) {
            throw new Error(`Completion failed: ${error instanceof Error ? error.message : String(error)}`);
        }
    }
    async completeWithOpenAI(prompt, model, maxTokens, temperature) {
        if (!this.openaiClient) {
            throw new Error('OpenAI client not initialized');
        }
        // Format prompt for chat completion
        const messages = [
            { role: 'user', content: prompt }
        ];
        const completion = await this.openaiClient.chat.completions.create({
            model: model,
            messages: messages,
            max_tokens: maxTokens,
            temperature: temperature,
        });
        const choice = completion.choices[0];
        return {
            completion: choice.message.content || '',
            id: completion.id,
            model: completion.model,
            type: 'completion',
            stop_reason: choice.finish_reason || undefined
        };
    }
    async completeWithAnthropic(prompt, model, maxTokens, temperature) {
        if (!this.anthropicClient) {
            throw new Error('Anthropic client not initialized');
        }
        const message = await this.anthropicClient.messages.create({
            model: model,
            max_tokens: maxTokens,
            temperature: temperature,
            messages: [
                { role: 'user', content: prompt }
            ]
        });
        const content = message.content[0];
        const completion = content.type === 'text' ? content.text : '';
        return {
            completion: completion,
            id: message.id,
            model: message.model,
            type: 'completion',
            stop_reason: message.stop_reason || undefined
        };
    }
    refreshConfiguration() {
        this.config = vscode.workspace.getConfiguration('xai-grok');
        this.initializeClients();
    }
}
exports.XAIGrokClient = XAIGrokClient;
//# sourceMappingURL=apiClient.js.map