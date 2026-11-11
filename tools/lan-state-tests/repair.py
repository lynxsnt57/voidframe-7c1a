import html
from pathlib import Path
for path in Path('CppProject/Lan').iterdir():
    if path.suffix != '.cpp' and path.suffix != '.hpp':
        continue
    text = open(path, encoding='utf-8').read()
    text = html.unescape(text)
    text = text.replace(chr(92) + chr(95), chr(95))
    text = text.replace(chr(92) + chr(91), chr(91))
    text = text.replace(chr(92) + chr(93), chr(93))
    text = text.replace(chr(92) + chr(42), chr(42))
    open(path, 'w', encoding='utf-8').write(text)
test = Path('tools/lan-state-tests/main.cpp')
if test.exists():
    test.rename(test.with_name('main.cpp.in'))
