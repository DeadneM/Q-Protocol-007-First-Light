"""Keep the player-accepted A28 native core, input and renderer frozen."""
from pathlib import Path
import subprocess

BASE='580e57cb1e240df04e1a3f8cdc0fb0b46067fcc2'
def baseline(path):
    return subprocess.check_output(['git','show',f'{BASE}:{path}'],encoding='utf-8')
core=Path('src/QProtocol.cpp').read_text(encoding='utf-8')
original=baseline('src/QProtocol.cpp')
normalized=core.replace('#include "Updates.h"\n','')
normalized=normalized.replace('''    if (!updates::EnsureDefaultIni(iniPath)) {
        Log("[ERROR] Cannot create missing default INI; core not started.");
        return 0;
    }
''','')
normalized=normalized.replace('    updates::Initialize(iniPath);\n','').replace('    updates::Shutdown();\n','')
normalized=normalized.replace('Q Protocol v0.9.7 / Updater and Readable Menu','Q Protocol v0.9.6 / A29 Updater and Menu TEST')
normalized=normalized.replace('Scope: accepted A28/A29 core and menu; stable updates by default.',
                              'Scope: A28 native core preserved; background updater and menu readability TEST.')
normalized=normalized.replace('Q Protocol v0.9.6 / A29 Updater and Menu TEST','Q Protocol v0.9.6 / A28 November Remap TEST')
normalized=normalized.replace('Scope: A28 native core preserved; background updater and menu readability TEST.',
                              'Scope: A28 TEST static native remap; A25/A26/A27 preserved; runtime validation pending.')
assert normalized==original,'Native gameplay changed outside bootstrap/lifecycle/build identity'
assert core.index('if (!ValidateTargetExecutable())') < core.index('updates::EnsureDefaultIni(iniPath)') < core.index('    LoadConfig(iniPath);')

path='src/OverlayA19.cpp'
old=baseline(path); ui=Path(path).read_text(encoding='utf-8')
normalized=ui.replace('#include "Updates.h"\n','')
a=normalized.index('void DrawUpdatesTab() {'); b=normalized.index('void DrawFooter() {',a)
normalized=normalized[:a]+normalized[b:]
for start,end in [('void DrawLoadoutTab() {','std::string Lower('),
                  ('void ApplyStyle() {','RECT GetVirtualScreenRect() {'),
                  ('void DrawOverlayWindow() {','bool IsMouseInputMessage(')]:
    a=normalized.index(start);b=normalized.index(end,a)
    oa=old.index(start);ob=old.index(end,oa)
    normalized=normalized[:a]+old[oa:ob]+normalized[b:]
assert normalized==old,'Changes outside the permitted presentation functions'
for token in ['kFooterReserve = 78.0f','##main_tabs_scroll','DrawFooter();','1000.0f','850.0f',
              'Check updates at start','Beta builds','Update on exit']:
    assert token in ui,token
for path in ['src/GameplayHook.asm','src/Overlay.h','config/QProtocol.ini',
             'src/first_debug/imgui_impl_dx12_first_debug.cpp',
             'src/first_debug/imgui_ps.hlsl','src/first_debug/imgui_vs.hlsl',
             'src/dxgi_proxy/ProxyDxgi.cpp','src/dxgi_proxy/DxgiSmoke.cpp',
             'evidence/nov2026/a28-native-map.json']:
    assert Path(path).read_text(encoding='utf-8')==baseline(path),path
print('A29 scope PASS: A28 native map/gates/gameplay, A26 renderer, A27 scanner, input, footer and personal-config defaults frozen.')
