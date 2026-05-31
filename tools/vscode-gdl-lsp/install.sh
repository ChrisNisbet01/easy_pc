#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXT_NAME="gdl-lsp-0.1.0"
VSCODE_EXT_DIR="${HOME}/.vscode/extensions/${EXT_NAME}"

echo "==> Installing npm dependencies..."
cd "${SCRIPT_DIR}"
npm install

echo "==> Copying extension to ${VSCODE_EXT_DIR}..."
rm -rf "${VSCODE_EXT_DIR}"
mkdir -p "${VSCODE_EXT_DIR}"

cp "${SCRIPT_DIR}/package.json"       "${VSCODE_EXT_DIR}/"
cp "${SCRIPT_DIR}/extension.js"       "${VSCODE_EXT_DIR}/"
cp "${SCRIPT_DIR}/language-configuration.json" "${VSCODE_EXT_DIR}/"

# Copy node_modules (vscode-languageclient and its deps)
cp -r "${SCRIPT_DIR}/node_modules"    "${VSCODE_EXT_DIR}/"

echo "==> Done!"
echo ""
echo "The extension is installed. Please restart VSCode, then open a .gdl file."
echo "To verify it's loaded, open the Output panel and select 'GDL Language Server' from the dropdown."
echo "Set the path to the GDL LSP server in ${VSCODE_EXT_DIR}/extension.js, serverPath variable."
echo
