"""Read-only audit models. No game, GPU, native call or installation is exercised."""
from pathlib import Path
import itertools
import json
import subprocess

COMMIT='13a843ae576d082dfaae2d5bb1909b6442dc7da2'
def source(path):
    return subprocess.check_output(['git','show',f'{COMMIT}:{path}'],encoding='utf-8')
core=source('src/QProtocol.cpp')
state=source('src/runtime/PlayableSession.h')
overlay=source('src/OverlayA19.cpp')
backend=source('src/first_debug/imgui_impl_dx12_first_debug.cpp')
assert 'limit = 500' in state and 'return g_publishedPlayer;' in core
assert 'qp::g_tickPlayer = qp::ObserveGameplayPlayer();' in core
assert 'FailActiveWeapon("gameplay trigger timeout")' in core
assert 'FailActiveWeapon("completion timeout")' in core
assert 'GetCurrentBackBufferIndex()' in overlay and 'bd->frameIndex = bd->frameIndex + 1' in backend
assert 'pFrameResources[bd->frameIndex % bd->numFramesInFlight]' in backend

results={
    'scope':COMMIT,
    'limits':'Source-anchored logical models only; no runtime or GPU proof.',
    'stale_observer':{'last_observed_ms':1000,'worker_read_ms':1501,'same_character':True,
                      'fresh':1501-1000<=500,'interpretation':'A34 can reset with no new native loss of control.'},
    'timeout_after_acceptance':{'native_accepted':True,'idle_at_5001_ms':False,
                               'path':'completion timeout -> FailActiveWeapon -> retry',
                               'interpretation':'No cancellation/inventory check proves a repeat is safe.'},
}

# Attempt to falsify a suspected frontend/backend ring-index mismatch. Do not
# report it as a defect if the front fences already bound all outstanding work.
checked=0
for buffers in [2,3]:
    for visible in itertools.product([False,True],repeat=14):
        front=[0]*buffers; uploads=[0]*buffers
        completed=0; submitted=0; rendered=0
        for present,is_visible in enumerate(visible):
            if not is_visible:continue
            swap=present%buffers
            completed=max(completed,front[swap]) # minimum completion forced by frontend wait
            upload=rendered%buffers
            assert uploads[upload]<=completed, (buffers,visible,present,front,uploads,completed)
            submitted+=1;rendered+=1
            front[swap]=submitted;uploads[upload]=submitted
        checked+=1
results['ring_index_suspicion']={
    'sequences_checked':checked,'buffers':[2,3],'presents_per_sequence':14,
    'counterexample_found':False,
    'assumptions':'Equal pool/backbuffer count, one backend call per submitted visible frame, successful monotonic fences, rendering stops on failures.',
    'interpretation':'Different indices alone do not establish unsafe reuse; exclude this suspicion from confirmed findings.'}

report=Path(__file__).resolve().parents[1]/'docs/audits/20261104/AUDIT_MODELS.json'
report.parent.mkdir(parents=True,exist_ok=True)
report.write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'model_sequences':checked,'ring_counterexample':False,
                  'stale_observer_counterexample':True,'native_runtime_test':False}))
