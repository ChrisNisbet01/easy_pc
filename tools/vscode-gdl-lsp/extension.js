const path = require("path");
const vscode = require("vscode");

let client = null;

function activate(context) {
  const serverPath = context.asAbsolutePath(
    path.join("..", "..", "..", "c_tools", "gdl_lsp_server", "gdl_lsp")
  );

  const serverOptions = {
    command: serverPath,
    args: [],
    options: {
      stdio: "pipe",
    },
  };

  const legend = new vscode.SemanticTokensLegend(
    ["keyword", "string", "number", "variable", "operator", "parameter", "function"],
    []
  );

  const clientOptions = {
    documentSelector: [{ language: "gdl", scheme: "file" }],
    synchronous: {
      handle: {},
    },
    initializationOptions: {},
    semanticTokens: { legend: legend, full: true },
  };

  client = new (require("vscode-languageclient").LanguageClient)(
    "gdl-lsp",
    "GDL Language Server",
    serverOptions,
    clientOptions
  );

  client.start();
}

function deactivate() {
  if (client) {
    return client.stop();
  }
}

module.exports = { activate, deactivate };
