import * as assert from 'assert';
import * as os from 'os';
import * as path from 'path';
import * as vscode from 'vscode';

interface ExtensionTestApi {
    restartClientForTests(): Promise<void>;
    setErrorReporterForTests(
        reporter: (message: string) => Thenable<string | undefined> | undefined
    ): void;
    resetErrorReporterForTests(): void;
}

const EXTENSION_ID = 'jz-hdl.jz-hdl';

async function withCapturedErrors(
    api: ExtensionTestApi,
    fn: (messages: string[]) => Promise<void>
): Promise<void> {
    const messages: string[] = [];
    api.setErrorReporterForTests((message) => {
        messages.push(message);
        return Promise.resolve(undefined);
    });

    try {
        await fn(messages);
    } finally {
        api.resetErrorReporterForTests();
    }
}

async function updateSetting<T>(key: string, value: T): Promise<void> {
    await vscode.workspace.getConfiguration('jz-hdl').update(
        key,
        value,
        vscode.ConfigurationTarget.Global
    );
}

async function getExtensionApi(): Promise<ExtensionTestApi> {
    const extension = vscode.extensions.getExtension<ExtensionTestApi>(EXTENSION_ID);
    assert.ok(extension, `Missing extension ${EXTENSION_ID}`);
    return extension.activate();
}

async function testStartupWithoutLsp(): Promise<void> {
    await updateSetting('lsp.enabled', false);
    await updateSetting('binaryPath', '');

    const api = await getExtensionApi();
    await api.restartClientForTests();

    const commands = await vscode.commands.getCommands(true);
    assert.ok(commands.includes('jz-hdl.selectProject'));

    const document = await vscode.workspace.openTextDocument({
        language: 'jz-hdl',
        content: 'module smoke (out led) { led = 1\'b1; }',
    });
    await vscode.window.showTextDocument(document);
    assert.strictEqual(document.languageId, 'jz-hdl');
    assert.strictEqual(document.isUntitled, true);
}

async function testMissingBinary(): Promise<void> {
    const api = await getExtensionApi();

    await withCapturedErrors(api, async (messages) => {
        await updateSetting('lsp.enabled', true);
        await updateSetting(
            'binaryPath',
            path.join(os.tmpdir(), `jz-hdl-missing-${Date.now()}`)
        );

        await api.restartClientForTests();

        assert.ok(
            messages.some((message) =>
                message.includes('Failed to start JZ-HDL language server')
            ),
            'Expected a startup failure for a missing binary path'
        );
    });
}

async function testBadBinaryPath(): Promise<void> {
    const api = await getExtensionApi();

    await withCapturedErrors(api, async (messages) => {
        await updateSetting('lsp.enabled', true);
        await updateSetting('binaryPath', os.tmpdir());

        await api.restartClientForTests();

        assert.ok(
            messages.some((message) =>
                message.includes('Failed to start JZ-HDL language server')
            ),
            'Expected a startup failure for a non-executable binary path'
        );
    });
}

async function resetSettings(api: ExtensionTestApi): Promise<void> {
    await updateSetting('binaryPath', '');
    await updateSetting('lsp.enabled', true);
    await api.restartClientForTests();
}

export async function run(): Promise<void> {
    const api = await getExtensionApi();

    try {
        await testStartupWithoutLsp();
        await testMissingBinary();
        await testBadBinaryPath();
    } finally {
        await resetSettings(api);
    }
}
