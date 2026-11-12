import base64
import hashlib
import json
import zlib
from pathlib import Path
spec = json.loads(zlib.decompress(base64.b64decode(Path('tools/lan-state-tests/session-restore.b64').read_bytes())))
path = Path('CppProject/Lan/LanSession.cpp')
text = path.read_text()
digest = hashlib.sha256(text.encode()).hexdigest()
if digest != spec.get('after'):
    if digest != spec.get('before'):
        raise RuntimeError('Unexpected source revision; refusing to overwrite edits')
    for start, end, value in reversed(spec.get('ops')):
        text = text[:start] + value + text[end:]
    if hashlib.sha256(text.encode()).hexdigest() != spec.get('after'):
        raise RuntimeError('Source integrity check failed')
    path.write_text(text)
