# GitHub MCP Server Setup Guide

## 🚀 Enhanced Development with GitHub Integration

This guide sets up the GitHub MCP Server to enable powerful GitHub integration directly through AI assistance for the ESP32 Color Matcher project.

## 🎯 Capabilities Enabled

With GitHub MCP Server, you can:
- ✅ **Repository Management**: Direct file operations, commits, branches
- ✅ **Issue Tracking**: Create, update, and manage GitHub issues
- ✅ **Pull Request Management**: Create, review, and merge PRs
- ✅ **GitHub Actions**: Monitor and manage CI/CD pipelines
- ✅ **Code Security**: Track vulnerabilities and security alerts
- ✅ **Context Awareness**: Understand current repository state

## 🔧 Prerequisites

1. **Docker**: Ensure Docker is installed and running
2. **GitHub Token**: Personal Access Token with appropriate permissions
3. **VS Code**: With GitHub Copilot extension

## 📝 Step 1: Create GitHub Personal Access Token

1. Go to [GitHub Settings → Developer settings → Personal access tokens](https://github.com/settings/tokens)
2. Click "Generate new token (classic)"
3. **Token Name**: `ESP32-Color-Matcher-MCP`
4. **Expiration**: 90 days (or as needed)
5. **Select Scopes**:
   ```
   ✅ repo                    (Full control of repositories)
   ✅ workflow               (Update GitHub Action workflows)
   ✅ write:packages         (Upload packages)
   ✅ read:org              (Read org and team membership)
   ✅ notifications         (Access notifications)
   ✅ user                  (Update user data)
   ✅ project               (Access projects)
   ✅ security_events       (Read security events)
   ```

6. **Copy the token** - you'll need it for VS Code configuration

## 🛠️ Step 2: VS Code Configuration

The MCP configuration is already set up in `.vscode/mcp.json` with optimized toolsets:

```json
{
  "inputs": [
    {
      "type": "promptString",
      "id": "github_token",
      "description": "GitHub Personal Access Token for color-matcher repository",
      "password": true
    }
  ],
  "servers": {
    "github": {
      "command": "docker",
      "args": [
        "run", "-i", "--rm",
        "-e", "GITHUB_PERSONAL_ACCESS_TOKEN",
        "-e", "GITHUB_TOOLSETS",
        "-e", "GITHUB_HOST",
        "ghcr.io/github/github-mcp-server"
      ],
      "env": {
        "GITHUB_PERSONAL_ACCESS_TOKEN": "${input:github_token}",
        "GITHUB_TOOLSETS": "context,repos,issues,pull_requests,actions,code_security",
        "GITHUB_HOST": "https://github.com"
      }
    }
  }
}
```

## 🎯 Step 3: Toolset Configuration

**Enabled Toolsets** (optimized for our ESP32 project):
- **context**: Repository and user context (essential)
- **repos**: File operations, commits, branches
- **issues**: Bug tracking and feature requests
- **pull_requests**: Code review workflow
- **actions**: CI/CD pipeline management
- **code_security**: Vulnerability scanning

## 🚀 Step 4: Activation

1. **Open VS Code** in the color-matcher project
2. **Enable GitHub Copilot** if not already active
3. **Toggle Agent Mode** (located by the Copilot Chat text input)
4. **Enter your GitHub token** when prompted
5. **Verify connection** by asking: "What's the current status of this repository?"

## 🧪 Testing the Setup

Try these commands to verify the MCP server is working:

```
# Repository context
"What's the current branch and recent commits?"

# Issue management
"List open issues in this repository"

# File operations
"Show me the recent changes to src/main.cpp"

# Security monitoring
"Are there any security alerts for this repository?"

# Actions monitoring
"What's the status of the latest GitHub Actions runs?"
```

## 🔒 Security Notes

- ✅ **Token Protection**: Tokens are stored securely by VS Code
- ✅ **Gitignore**: Token files are excluded from version control
- ✅ **Scoped Access**: Only necessary permissions are granted
- ✅ **Expiration**: Tokens have limited lifetime for security

## 🛠️ Troubleshooting

### Docker Issues
```bash
# Check Docker is running
docker --version

# Pull the MCP server image manually
docker pull ghcr.io/github/github-mcp-server
```

### Token Issues
- Verify token has correct scopes
- Check token hasn't expired
- Ensure token is for the correct GitHub account

### Connection Issues
- Restart VS Code
- Check Docker is running
- Verify network connectivity to GitHub

## 🎊 Benefits for Color Matcher Development

With GitHub MCP Server, you can now:
- **Track calibration issues** directly through GitHub Issues
- **Manage code reviews** for calibration improvements
- **Monitor build status** of firmware updates
- **Track security vulnerabilities** in dependencies
- **Automate release management** for stable versions
- **Coordinate team development** through PR workflows

## 📚 Additional Resources

- [GitHub MCP Server Documentation](https://github.com/github/github-mcp-server)
- [MCP Protocol Specification](https://modelcontextprotocol.io/)
- [VS Code Agent Mode Documentation](https://code.visualstudio.com/docs/copilot/copilot-chat)
