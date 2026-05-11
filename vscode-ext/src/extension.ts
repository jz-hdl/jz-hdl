import * as vscode from 'vscode';
import * as fs from 'fs';
import * as path from 'path';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
} from 'vscode-languageclient/node';

let client: LanguageClient | undefined;
let statusBarItem: vscode.StatusBarItem | undefined;
type UserMessageKind = 'error' | 'warning' | 'info';
type ProjectSelectionState = 'active' | 'ambiguous' | 'not-found' | 'not-imported';

let messageReporter: (
    kind: UserMessageKind,
    message: string
) => Thenable<string | undefined> | undefined = (kind, message) => {
    switch (kind) {
    case 'warning':
        return vscode.window.showWarningMessage(message);
    case 'info':
        return vscode.window.showInformationMessage(message);
    default:
        return vscode.window.showErrorMessage(message);
    }
};

/** Last project info received per URI, used by the picker and status bar. */
const projectInfoByUri = new Map<string, ProjectInfoParams>();
const reportedProjectStateByUri = new Map<string, string>();

interface ProjectEntry {
    file: string;
    chip: string;
    name: string;
}

interface ProjectInfoParams {
    uri: string;
    projects: ProjectEntry[];
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

function getServerCommand(): string {
    const config = vscode.workspace.getConfiguration('jz-hdl');
    const binaryPath = config.get<string>('binaryPath', '');
    return binaryPath || 'jz-hdl';
}

function isLspEnabled(): boolean {
    const config = vscode.workspace.getConfiguration('jz-hdl');
    return config.get<boolean>('lsp.enabled', true);
}

function showUserMessage(
    kind: UserMessageKind,
    message: string
): Thenable<string | undefined> | undefined {
    return messageReporter(kind, message);
}

function getProjectStateKey(params: ProjectInfoParams): string {
    return `${params.selectionState}:${params.message}`;
}

function describeProjectState(params: ProjectInfoParams): string {
    if (params.message) {
        return params.message;
    }
    switch (params.selectionState) {
    case 'ambiguous':
        return 'Ambiguous project selection. Select the intended project.';
    case 'not-imported':
        return 'Project discovery found projects, but none import this file.';
    case 'not-found':
        return 'Project discovery failed: no JZ-HDL project files were found for this file.';
    default:
        return '';
    }
}

function reportProjectState(params: ProjectInfoParams): void {
    if (params.selectionState === 'active') {
        reportedProjectStateByUri.delete(params.uri);
        return;
    }

    const activeUri = vscode.window.activeTextEditor?.document.uri.toString();
    if (activeUri !== params.uri) {
        return;
    }

    const stateKey = getProjectStateKey(params);
    if (reportedProjectStateByUri.get(params.uri) === stateKey) {
        return;
    }
    reportedProjectStateByUri.set(params.uri, stateKey);

    const detail = describeProjectState(params);
    if (!detail) {
        return;
    }

    if (params.selectionState === 'not-found') {
        void showUserMessage('warning', detail);
        return;
    }

    void showUserMessage('warning', detail);
}

function validateConfiguredBinaryPath(binaryPath: string): string | undefined {
    const resolved = path.resolve(binaryPath);

    if (!fs.existsSync(resolved)) {
        return `JZ-HDL LSP startup failed: configured binary not found at "${resolved}". Set jz-hdl.binaryPath to the compiler binary or clear it to use PATH.`;
    }

    let stat: fs.Stats;
    try {
        stat = fs.statSync(resolved);
    } catch (error) {
        const detail = error instanceof Error ? error.message : String(error);
        return `JZ-HDL LSP startup failed: could not inspect configured binary "${resolved}": ${detail}`;
    }

    if (!stat.isFile()) {
        return `JZ-HDL LSP startup failed: configured binary path "${resolved}" is not a file. Point jz-hdl.binaryPath to the jz-hdl compiler binary.`;
    }

    if (process.platform !== 'win32') {
        try {
            fs.accessSync(resolved, fs.constants.X_OK);
        } catch {
            return `JZ-HDL LSP startup failed: configured binary "${resolved}" is not executable. Point jz-hdl.binaryPath to the jz-hdl compiler binary.`;
        }
    }

    return undefined;
}

function getServerStartupError(command: string, configured: boolean, detail: string): string {
    if (configured) {
        return `JZ-HDL LSP startup failed: could not launch configured binary "${command}". ${detail}`;
    }

    return `JZ-HDL LSP startup failed: could not launch "jz-hdl" from PATH. ${detail} Install jz-hdl or set jz-hdl.binaryPath to the compiler binary.`;
}

function isJzDocument(document: vscode.TextDocument): boolean {
    return document.languageId === 'jz-hdl';
}

function getProjectInfoForEditor(
    editor: vscode.TextEditor | undefined
): ProjectInfoParams | undefined {
    if (!editor || !isJzDocument(editor.document)) {
        return undefined;
    }

    return projectInfoByUri.get(editor.document.uri.toString());
}

function renderStatusBarForEditor(editor: vscode.TextEditor | undefined): void {
    if (!statusBarItem) return;

    if (!editor || !isJzDocument(editor.document)) {
        statusBarItem.hide();
        return;
    }

    const params = getProjectInfoForEditor(editor);
    if (!params) {
        statusBarItem.text = "$(circuit-board) ...";
        statusBarItem.tooltip = "Waiting for JZ-HDL project discovery";
        statusBarItem.command = undefined;
        statusBarItem.show();
        return;
    }

    if (params.selectionState === 'not-found') {
        statusBarItem.text = "$(circuit-board) No Project";
        statusBarItem.tooltip = describeProjectState(params);
        statusBarItem.command = undefined;
        statusBarItem.show();
        return;
    }

    if (params.selectionState === 'not-imported') {
        statusBarItem.text = "$(circuit-board) No Project";
        statusBarItem.tooltip = describeProjectState(params);
        statusBarItem.command = 'jz-hdl.selectProject';
        statusBarItem.show();
        return;
    }

    if (params.selectionState === 'ambiguous') {
        statusBarItem.text = "$(circuit-board) Select Project";
        statusBarItem.tooltip = describeProjectState(params);
        statusBarItem.command = 'jz-hdl.selectProject';
        statusBarItem.show();
        return;
    }

    const active = params.projects[params.activeIndex];
    const name = active.name !== '-' ? active.name : 'Unknown';
    const chip = active.chip !== '-' ? active.chip : '';

    statusBarItem.text = chip
        ? `$(circuit-board) ${name} [${chip}]`
        : `$(circuit-board) ${name}`;

    // Build tooltip with all projects.
    const lines: string[] = [];
    for (let i = 0; i < params.projects.length; i++) {
        const p = params.projects[i];
        const pName = p.name !== '-' ? p.name : '?';
        const pChip = p.chip !== '-' ? p.chip : 'no chip';
        const marker = i === params.activeIndex ? ' (active)' : '';
        const basename = path.basename(p.file);
        lines.push(`${pName} [${pChip}] - ${basename}${marker}`);
    }
    if (params.projects.length > 1) {
        lines.push('', 'Click to switch project');
    }
    statusBarItem.tooltip = lines.join('\n');
    statusBarItem.command = params.projects.length > 1 ? 'jz-hdl.selectProject' : undefined;
    statusBarItem.show();
}

async function showProjectPicker(): Promise<void> {
    const editor = vscode.window.activeTextEditor;
    const projectInfo = getProjectInfoForEditor(editor);

    if (!projectInfo || projectInfo.projects.length === 0) {
        void showUserMessage(
            'warning',
            projectInfo ? describeProjectState(projectInfo) : 'No JZ-HDL projects discovered.'
        );
        return;
    }

    const items = projectInfo.projects.map((p, i) => {
        const name = p.name !== '-' ? p.name : '?';
        const chip = p.chip !== '-' ? p.chip : 'no chip';
        const basename = path.basename(p.file);
        const active = i === projectInfo.activeIndex ? ' $(check)' : '';
        return {
            label: `${name}${active}`,
            description: `[${chip}]`,
            detail: basename,
            index: i,
            projectFile: p.file,
        };
    });

    const picked = await vscode.window.showQuickPick(items, {
        placeHolder: 'Select a JZ-HDL project for this file',
    });

    if (!picked || !client) return;

    // Send the selection to the LSP server.
    client.sendNotification('jz-hdl/selectProject', {
        uri: projectInfo.uri,
        projectFile: picked.projectFile,
    });
}

async function startClient(): Promise<void> {
    if (client) {
        return;
    }

    const configuredBinaryPath = vscode.workspace
        .getConfiguration('jz-hdl')
        .get<string>('binaryPath', '')
        .trim();
    const hasConfiguredBinaryPath = configuredBinaryPath.length > 0;
    const command = hasConfiguredBinaryPath
        ? path.resolve(configuredBinaryPath)
        : getServerCommand();

    if (hasConfiguredBinaryPath) {
        const validationMessage = validateConfiguredBinaryPath(configuredBinaryPath);
        if (validationMessage) {
            void showUserMessage('error', validationMessage);
            return;
        }
    }

    const serverOptions: ServerOptions = {
        command: command,
        args: ['--lsp'],
    };

    const config = vscode.workspace.getConfiguration('jz-hdl');

    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ language: 'jz-hdl' }],
        initializationOptions: {
            hover: {
                clocks: config.get<boolean>('hover.clocks', true),
                declarations: config.get<boolean>('hover.declarations', true),
            },
        },
    };

    client = new LanguageClient(
        'jz-hdl',
        'JZ-HDL Language Server',
        serverOptions,
        clientOptions
    );

    try {
        await client.start();

        // Listen for project info notifications from the server.
        client.onNotification('jz-hdl/projectInfo', (params: ProjectInfoParams) => {
            projectInfoByUri.set(params.uri, params);
            reportProjectState(params);
            if (vscode.window.activeTextEditor?.document.uri.toString() === params.uri) {
                renderStatusBarForEditor(vscode.window.activeTextEditor);
            }
        });
    } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        void showUserMessage(
            'error',
            getServerStartupError(command, hasConfiguredBinaryPath, message)
        );
        client = undefined;
    }
}

async function stopClient(): Promise<void> {
    if (client) {
        await client.stop();
        client = undefined;
    }

    projectInfoByUri.clear();
    reportedProjectStateByUri.clear();
    renderStatusBarForEditor(vscode.window.activeTextEditor);
}

export async function activate(context: vscode.ExtensionContext): Promise<ExtensionTestApi> {
    // Register the project picker command.
    context.subscriptions.push(
        vscode.commands.registerCommand('jz-hdl.selectProject', showProjectPicker)
    );

    // Create the status bar item.
    statusBarItem = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Left, 50
    );
    statusBarItem.name = 'JZ-HDL Project';
    context.subscriptions.push(statusBarItem);

    // Show/hide based on active editor language.
    context.subscriptions.push(
        vscode.window.onDidChangeActiveTextEditor((editor) => {
            renderStatusBarForEditor(editor);
        })
    );

    context.subscriptions.push(
        vscode.workspace.onDidCloseTextDocument((document) => {
            projectInfoByUri.delete(document.uri.toString());
            reportedProjectStateByUri.delete(document.uri.toString());
        })
    );

    renderStatusBarForEditor(vscode.window.activeTextEditor);

    if (isLspEnabled()) {
        await startClient();
    }

    // React to configuration changes.
    context.subscriptions.push(
        vscode.workspace.onDidChangeConfiguration(async (e) => {
            if (e.affectsConfiguration('jz-hdl.lsp.enabled') ||
                e.affectsConfiguration('jz-hdl.binaryPath')) {

                if (isLspEnabled()) {
                    // Restart with potentially new binary path.
                    await stopClient();
                    await startClient();
                } else {
                    await stopClient();
                }
            }
        })
    );

    return {
        async restartClientForTests(): Promise<void> {
            await stopClient();
            if (isLspEnabled()) {
                await startClient();
            }
        },
        setMessageReporterForTests(
            reporter: (
                kind: UserMessageKind,
                message: string
            ) => Thenable<string | undefined> | undefined
        ): void {
            messageReporter = reporter;
        },
        resetMessageReporterForTests(): void {
            messageReporter = (kind, message) => {
                switch (kind) {
                case 'warning':
                    return vscode.window.showWarningMessage(message);
                case 'info':
                    return vscode.window.showInformationMessage(message);
                default:
                    return vscode.window.showErrorMessage(message);
                }
            };
        },
        getProjectInfoForUriForTests(uri: string): ProjectInfoParams | undefined {
            return projectInfoByUri.get(uri);
        },
        selectProjectForTests(uri: string, projectFile: string): void {
            if (!client) {
                return;
            }
            client.sendNotification('jz-hdl/selectProject', { uri, projectFile });
        },
    };
}

export async function deactivate(): Promise<void> {
    await stopClient();
}
