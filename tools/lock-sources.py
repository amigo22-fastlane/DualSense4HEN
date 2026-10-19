"""Record only read-only Git metadata from locally audited repositories."""
import json
import subprocess
from pathlib import Path
root = Path(__file__).resolve().parents[1]
repos = []
for path in sorted((root / 'research').iterdir()):
    if not (path / '.git').exists():
        continue
    def git(*args):
        return subprocess.check_output(['git', '-C', str(path), *args], text=True).strip()
    repos.append({'name': path.name, 'url': git('remote', 'get-url', 'origin'),
                  'commit': git('rev-parse', 'HEAD'),
                  'branch': git('branch', '--show-current')})
(root / 'docs' / 'sources.lock.json').write_text(
    json.dumps({'audit_date': '2026-10-19', 'repositories': repos}, indent=2) + '\n', encoding='utf-8')
