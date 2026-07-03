# Vion Language VS Code Support

This local VS Code extension adds:

- `.vion` language registration
- basic syntax highlighting
- bracket/comment behavior
- a Vion file icon theme

## Try It Locally

Open this folder as a VS Code extension development host:

```powershell
code tools/vscode-vion
```

Then press `F5` in VS Code to launch an Extension Development Host.

## Package Later

When the extension is ready to publish or install permanently, package it with `vsce`:

```powershell
npm install -g @vscode/vsce
cd tools/vscode-vion
vsce package
code --install-extension .\vion-language-0.2.0.vsix
```

After installing, select the `Vion File Icons` icon theme from VS Code's icon theme picker.
