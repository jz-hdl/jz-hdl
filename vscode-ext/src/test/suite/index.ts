import * as assert from 'assert';
import * as os from 'os';
import * as path from 'path';
import * as vscode from 'vscode';

type UserMessageKind = 'error' | 'warning' | 'info';
type ProjectSelectionState = 'active' | 'ambiguous' | 'not-found' | 'not-imported';

interface ProjectInfoParams {
    uri: string;
    projects: Array<{
        file: string;
        chip: string;
        name: string;
    }>;
    activeIndex: number;
    selectionState: ProjectSelectionState;
    message: string;
}

interface ExtensionTestApi {
    restartClientForTests(): Promise<void>;
    setMessageReporterForTests(
        reporter: (
            kind: UserMessageKind,
            message: string
        ) => Thenable<string | undefined> | undefined
    ): void;
    resetMessageReporterForTests(): void;
    getProjectInfoForUriForTests(uri: string): ProjectInfoParams | undefined;
    selectProjectForTests(uri: string, projectFile: string): void;
}

interface CapturedMessage {
    kind: UserMessageKind;
    message: string;
}

const EXTENSION_ID = 'jz-hdl.jz-hdl';
const FIXTURE_ROOT = path.resolve(__dirname, '../../../test-fixtures/smoke-workspace');
const JZ_HDL_BINARY = process.env.JZ_HDL_BINARY || '';

async function withCapturedMessages(
    api: ExtensionTestApi,
    fn: (messages: CapturedMessage[]) => Promise<void>
): Promise<void> {
    const messages: CapturedMessage[] = [];
    api.setMessageReporterForTests((kind, message) => {
        messages.push({ kind, message });
        return Promise.resolve(undefined);
    });

    try {
        await fn(messages);
    } finally {
        api.resetMessageReporterForTests();
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

async function waitFor<T>(
    label: string,
    producer: () => T | undefined,
    timeoutMs = 10000
): Promise<T> {
    const deadline = Date.now() + timeoutMs;
    while (Date.now() < deadline) {
        const value = producer();
        if (value !== undefined) {
            return value;
        }
        await new Promise((resolve) => setTimeout(resolve, 50));
    }

    throw new Error(`Timed out waiting for ${label}`);
}

function fixturePath(relativePath: string): string {
    return path.join(FIXTURE_ROOT, relativePath);
}

function fixtureUri(relativePath: string): vscode.Uri {
    return vscode.Uri.file(fixturePath(relativePath));
}

async function openFixture(relativePath: string): Promise<vscode.TextDocument> {
    const document = await vscode.workspace.openTextDocument(fixtureUri(relativePath));
    await vscode.window.showTextDocument(document);
    return document;
}

function findMessage(
    messages: CapturedMessage[],
    kind: UserMessageKind,
    needle: string
): CapturedMessage | undefined {
    return messages.find(
        (message) => message.kind === kind && message.message.includes(needle)
    );
}

async function waitForProjectState(
    api: ExtensionTestApi,
    document: vscode.TextDocument,
    state: ProjectSelectionState
): Promise<ProjectInfoParams> {
    return waitFor(`project state ${state}`, () => {
        const info = api.getProjectInfoForUriForTests(document.uri.toString());
        return info && info.selectionState === state ? info : undefined;
    });
}

function findPosition(document: vscode.TextDocument, needle: string): vscode.Position {
    const offset = document.getText().indexOf(needle);
    assert.notStrictEqual(offset, -1, `Missing text marker ${needle}`);
    return document.positionAt(offset);
}

function normalizeLocation(
    location: vscode.Location | vscode.LocationLink
): vscode.Location {
    if ('uri' in location) {
        return location;
    }

    return new vscode.Location(
        location.targetUri,
        location.targetSelectionRange ?? location.targetRange
    );
}

async function waitForDefinitionResults(
    document: vscode.TextDocument,
    position: vscode.Position
): Promise<Array<vscode.Location | vscode.LocationLink>> {
    const deadline = Date.now() + 10000;
    while (Date.now() < deadline) {
        const results = await vscode.commands.executeCommand<
            Array<vscode.Location | vscode.LocationLink>
        >(
            'vscode.executeDefinitionProvider',
            document.uri,
            position
        );
        if (results && results.length > 0) {
            return results;
        }
        await new Promise((resolve) => setTimeout(resolve, 50));
    }

    throw new Error('Timed out waiting for definition results');
}

async function restartWithRealBinary(api: ExtensionTestApi): Promise<void> {
    assert.ok(JZ_HDL_BINARY, 'JZ_HDL_BINARY must be set for the extension smoke test');
    await updateSetting('lsp.enabled', true);
    await updateSetting('binaryPath', JZ_HDL_BINARY);
    await api.restartClientForTests();
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
        content: '@module Smoke\n    PORT { OUT [1] led; }\n    ASYNCHRONOUS { led = 1\'b1; }\n@endmod\n',
    });
    await vscode.window.showTextDocument(document);
    assert.strictEqual(document.languageId, 'jz-hdl');
    assert.strictEqual(document.isUntitled, true);
}

async function testMissingBinary(): Promise<void> {
    const api = await getExtensionApi();

    await withCapturedMessages(api, async (messages) => {
        const missingPath = path.join(os.tmpdir(), `jz-hdl-missing-${Date.now()}`);
        await updateSetting('lsp.enabled', true);
        await updateSetting('binaryPath', missingPath);

        await api.restartClientForTests();

        assert.ok(
            findMessage(messages, 'error', 'configured binary not found'),
            'Expected a startup failure for a missing binary path'
        );
        assert.ok(
            findMessage(messages, 'error', path.resolve(missingPath)),
            'Expected the missing binary path to be reported'
        );
    });
}

async function testBadBinaryPath(): Promise<void> {
    const api = await getExtensionApi();

    await withCapturedMessages(api, async (messages) => {
        await updateSetting('lsp.enabled', true);
        await updateSetting('binaryPath', os.tmpdir());

        await api.restartClientForTests();

        assert.ok(
            findMessage(messages, 'error', 'is not a file'),
            'Expected a startup failure for a non-binary path'
        );
    });
}

async function testClientServerSmoke(): Promise<void> {
    const api = await getExtensionApi();

    await withCapturedMessages(api, async (messages) => {
        await restartWithRealBinary(api);

        const orphan = await openFixture('orphan.jz');
        const orphanInfo = await waitForProjectState(api, orphan, 'not-imported');
        assert.ok(
            orphanInfo.message.includes('none import this file'),
            'Expected a file-discovery failure message for an unimported file'
        );
        assert.ok(
            findMessage(messages, 'warning', 'none import this file'),
            'Expected a warning for a file without an importing project'
        );

        const sharedModule = await openFixture('shared/shared_module.jz');
        const ambiguousInfo = await waitForProjectState(api, sharedModule, 'ambiguous');
        assert.ok(
            ambiguousInfo.projects.length >= 2,
            'Expected multiple discovered projects for the shared module'
        );
        assert.ok(
            findMessage(messages, 'warning', 'Ambiguous project selection'),
            'Expected a warning for ambiguous project selection'
        );

        api.selectProjectForTests(
            sharedModule.uri.toString(),
            fixturePath('project_alpha.jz')
        );
        const selectedInfo = await waitForProjectState(api, sharedModule, 'active');
        assert.ok(selectedInfo.activeIndex >= 0, 'Expected manual project selection to activate');
        assert.strictEqual(
            selectedInfo.projects[selectedInfo.activeIndex].file,
            fixturePath('project_alpha.jz'),
            'Expected the selected project to become active'
        );

        const definitionPosition = findPosition(sharedModule, 'local_sig; // def-target-use');
        const declarationPosition = findPosition(sharedModule, 'local_sig [1]; // def-target');
        const definitionResults = await waitForDefinitionResults(
            sharedModule,
            definitionPosition
        );
        const definition = normalizeLocation(definitionResults[0]);
        assert.strictEqual(
            definition.uri.fsPath,
            fixturePath('shared/shared_module.jz'),
            'Expected go-to-definition to stay within the shared module fixture'
        );
        assert.strictEqual(
            definition.range.start.line,
            declarationPosition.line,
            'Expected go-to-definition to resolve to the local_sig declaration'
        );

        const diagnosticsDoc = await openFixture('diagnostics_project.jz');
        const diagnostics = await waitFor('diagnostics', () => {
            const current = vscode.languages.getDiagnostics(diagnosticsDoc.uri);
            return current.length > 0 ? current : undefined;
        });
        assert.ok(
            diagnostics.some((diagnostic) =>
                diagnostic.message.toLowerCase().includes('undeclared')
            ),
            'Expected a diagnostic mentioning an undeclared identifier'
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
        await testClientServerSmoke();
    } finally {
        await resetSettings(api);
    }
}
