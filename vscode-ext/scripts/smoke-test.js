const cp = require('child_process');
const fs = require('fs');
const os = require('os');
const path = require('path');
const {
  downloadAndUnzipVSCode,
  resolveCliPathFromVSCodeExecutablePath,
} = require('@vscode/test-electron');

const extensionRoot = path.resolve(__dirname, '..');
const extensionTestsPath = path.join(extensionRoot, 'out', 'test', 'suite', 'index.js');
const vsixPath = path.join(extensionRoot, 'build', 'jz-hdl.vsix');

function runCommand(command, args, options = {}) {
  const result = cp.spawnSync(command, args, {
    cwd: extensionRoot,
    stdio: 'inherit',
    ...options,
  });

  if (result.status !== 0) {
    const rendered = [command, ...args].join(' ');
    throw new Error(`Command failed: ${rendered}`);
  }
}

async function main() {
  fs.mkdirSync(path.dirname(vsixPath), { recursive: true });

  runCommand('npm', ['run', 'package:vsix']);

  const vscodeExecutablePath = await downloadAndUnzipVSCode('stable');
  const vscodeCliPath = resolveCliPathFromVSCodeExecutablePath(vscodeExecutablePath);
  const userDataDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jz-hdl-vscode-user-'));
  const extensionsDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jz-hdl-vscode-exts-'));

  try {
    runCommand(vscodeCliPath, [
      '--user-data-dir',
      userDataDir,
      '--extensions-dir',
      extensionsDir,
      '--install-extension',
      vsixPath,
      '--force',
    ]);

    runCommand(vscodeCliPath, [
      '--user-data-dir',
      userDataDir,
      '--extensions-dir',
      extensionsDir,
      `--extensionDevelopmentPath=${extensionRoot}`,
      `--extensionTestsPath=${extensionTestsPath}`,
      '--disable-extensions',
    ]);
  } finally {
    fs.rmSync(userDataDir, { recursive: true, force: true });
    fs.rmSync(extensionsDir, { recursive: true, force: true });
  }
}

main().catch((error) => {
  console.error(error instanceof Error ? error.message : String(error));
  process.exit(1);
});
