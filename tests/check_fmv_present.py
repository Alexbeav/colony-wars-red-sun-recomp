"""Check completed OpenGL movie captures from the private desktop probe."""
import argparse
import json
from pathlib import Path
from PIL import Image

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('capture', type=Path)
args = p.parse_args()
receipt = json.loads((args.capture / 'receipt.json').read_text())
isolation = json.loads((args.capture / 'isolation.json').read_text())
terminal = json.loads((args.capture / 'psx_last_run_report.json').read_text())
assert isolation['isolation_proven'] and isolation['interactive_window_count'] == 0
assert isolation['exit_code'] == 0 and not isolation['timed_out']
assert receipt['pid'] == receipt['owned_listener'] == isolation['process_id']
assert terminal['exit_origin'] == 'tcp_quit'
checked = {192: 0, 220: 0}
previous_seq = 0
for i, sample in enumerate(receipt['samples']):
    native_path = args.capture / f'native-{i:02}.png'
    present_path = args.capture / f'present-{i:02}.png'
    if sample['gpu']['depth'] != 24 or not present_path.exists() or not native_path.exists():
        continue
    assert sample['done']['wrote'] and sample['done']['seq'] > previous_seq
    previous_seq = sample['done']['seq']
    native = Image.open(native_path).convert('RGB')
    box = native.getbbox()
    if box not in ((0, 24, 320, 216), (0, 10, 320, 230)):
        continue  # A dark frame cannot establish the full movie boundary.
    present = Image.open(present_path).convert('RGB')
    shown = present.getbbox()
    assert shown is not None
    assert abs(shown[1] - (present.height - shown[3])) <= 1, (i, shown)
    checked[box[3] - box[1]] += 1
assert all(checked.values()), checked
print(json.dumps(dict(centered_presentations=checked, terminal_frame=terminal['frame'])))
