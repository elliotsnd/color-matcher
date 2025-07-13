import * as vscode from 'vscode';
import OpenAI from 'openai';
import Anthropic from '@anthropic-ai/sdk';

export interface CompletionRequest {
    prompt: string;
    maxTokens?: number;
    temperature?: number;
    model?: string;
}

export interface CompletionResponse {
    completion: string;
    id: string;
    model: string;
    type: string;
    stop_reason?: string;
}

export class XAIGrokClient {
    private openaiClient: OpenAI | null = null;
    private anthropicClient: Anthropic | null = null;
    private config: vscode.WorkspaceConfiguration;

    constructor() {
        this.config = vscode.workspace.getConfiguration('xai-grok');
        this.initializeClients();
    }

    private initializeClients() {
        const apiKey = this.config.get<string>('apiKey');
        const apiEndpoint = this.config.get<string>('apiEndpoint', 'https://api.x.ai/v1');
        
        if (!apiKey) {
            vscode.window.showWarningMessage('xAI API key not configured. Please set it in settings.');
            return;
        }

        // Initialize OpenAI client
        this.openaiClient = new OpenAI({
            apiKey: apiKey,
            baseURL: apiEndpoint,
        });

        // Initialize Anthropic client
        this.anthropicClient = new Anthropic({
            apiKey: apiKey,
            baseURL: apiEndpoint.replace('/v1', ''), // Anthropic SDK expects base without /v1
        });
    }

    public async complete(request: CompletionRequest): Promise<CompletionResponse> {
        const sdkType = this.config.get<string>('sdkType', 'openai');
        const model = request.model || this.config.get<string>('model', 'grok-4');
        const maxTokens = request.maxTokens || this.config.get<number>('maxTokens', 100);
        const temperature = request.temperature || this.config.get<number>('temperature', 0.1);

        try {
            if (sdkType === 'anthropic') {
                return await this.completeWithAnthropic(request.prompt, model, maxTokens, temperature);
            } else {
                return await this.completeWithOpenAI(request.prompt, model, maxTokens, temperature);
            }
        } catch (error) {
            throw new Error(`Completion failed: ${error instanceof Error ? error.message : String(error)}`);
        }
    }

    private async completeWithOpenAI(
        prompt: string, 
        model: string, 
        maxTokens: number, 
        temperature: number
    ): Promise<CompletionResponse> {
        if (!this.openaiClient) {
            throw new Error('OpenAI client not initialized');
        }

        // Format prompt for chat completion
        const messages = [
            { role: 'user' as const, content: prompt }
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

    private async completeWithAnthropic(
        prompt: string, 
        model: string, 
        maxTokens: number, 
        temperature: number
    ): Promise<CompletionResponse> {
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

    public refreshConfiguration() {
        this.config = vscode.workspace.getConfiguration('xai-grok');
        this.initializeClients();
    }
}
