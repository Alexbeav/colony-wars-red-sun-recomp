"""Validate a private Red Sun probe receipt; requires Pillow for PNG checks."""
import argparse
from collections import Counter
import json
from pathlib import Path
from PIL import Image

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('capture', type=Path)
parser.add_argument('--original-layout', action='store_true')
args = parser.parse_args()
receipt = json.loads((args.capture / 'receipt.json').read_text())
terminal = json.loads((args.capture / 'psx_last_run_report.json').read_text())
assert receipt.get('exit_code') == 0 and not receipt.get('forced_stop'), 'Probe did not exit normally'
assert receipt['pid'] == receipt['owned_listener'], 'Listener ownership differs'
assert terminal['exit_origin'] == 'tcp_quit', terminal['exit_origin']
assert terminal['frame'] >= receipt['samples'][-1]['gpu']['ws']['cur_frame']
uploads = []
for sample in receipt['samples']:
    for ring in sample['rings']:
        uploads.extend(e for e in ring.get('entries', []) if e['op'] == '0xA0')
history_path = args.capture / 'startup-gpu.json'
if history_path.exists():
    history = json.loads(history_path.read_text())
    assert history['stats']['total'] < history['stats']['capacity'], 'Initial uploads were overwritten'
    for frame in history['frames']:
        uploads.extend(frame['copies'])
seen = Counter()
for entry in uploads:
    position, size = (int(v, 16) for v in entry['w'][1:3])
    x, y, width, height = position & 65535, position >> 16, size & 65535, size >> 16
    if width != 24 or height not in (192, 220):
        continue
    expected = 32 if args.original_layout else (240 - height) // 2
    assert y in (expected, expected + 256), (entry, expected)
    seen[height, y // 256, x] += 1
for height in (192, 220):
    for buffer in (0, 1):
        for x in range(0, 480, 24):
            assert seen[height, buffer, x], ('Missing movie strip', height, buffer, x)
images = 0
if not args.original_layout:
    for i, sample in enumerate(receipt['samples']):
        heights = [int(e['w'][2], 16) >> 16 for ring in sample['rings']
                   for e in ring.get('entries', []) if e['op'] == '0xA0']
        if not heights or len(set(heights)) != 1 or heights[0] not in (192, 220):
            continue
        image = Image.open(args.capture / f'frame-{i:02}.png').convert('RGB')
        assert image.size == (320, 240), image.size
        border = (240 - heights[0]) // 2
        assert image.crop((0, 0, 320, border)).getbbox() is None, ('Top border', i)
        assert image.crop((0, 240 - border, 320, 240)).getbbox() is None, ('Bottom border', i)
        images += image.getbbox() is not None
    assert images >= 2, 'No useful movie images'
print(json.dumps(dict(upload_checks=sum(seen.values()), strip_cases=len(seen),
                      nonempty_images=images, initial_uploads_checked=history_path.exists())))
