"""Restrict beta.2 to visible-menu GPU initialization and version identity."""
from pathlib import Path
import subprocess

BASE = '96cd47002b18ec07dd418550fc71b4e0526b3457'
deferred = '''    // GPU objects and the font upload are only needed for a visible menu.
    // In particular, loading-screen resizes must not initialize them.
    if (!g_requestedVisible.load(std::memory_order_acquire)) {
        return;
    }

'''
paths = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', BASE, '--',
                                 'src', 'config', 'updater', 'tests'], encoding='utf-8').splitlines()
for path in paths:
    expected = subprocess.check_output(['git', 'show', f'{BASE}:{path}'], encoding='utf-8')
    actual = Path(path).read_text(encoding='utf-8')
    if path == 'src/OverlayA19.cpp':
        assert actual.count(deferred) == 1
        start = actual.index('void RenderOverlayFrameLocked(')
        assert start < actual.index(deferred) < actual.index('if (!g_imguiDx12Initialized)', start)
        actual = actual.replace(deferred, '')
    elif path == 'src/Updates.h':
        expected = expected.replace('Build = 39', 'Build = 40').replace('v0.9.8-beta.1', 'v0.9.8-beta.2')
    elif path == 'src/QProtocol.cpp':
        expected = expected.replace('Log("Q Protocol v0.9.8-beta.1 / internal build 39");',
                                    'Log("Q Protocol %s / internal build %d", updates::Version, updates::Build);')
    assert actual == expected, path
print('beta.2 scope PASS: visible-menu check precedes GPU initialization; delivery, native guards, hooks, configuration, UI actions and updater retained.')
