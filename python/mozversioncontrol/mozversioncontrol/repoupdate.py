



from __future__ import unicode_literals

import os
import subprocess




def update_mercurial_repo(hg, repo, path, revision='default'):
    """Ensure a HG repository exists at a path and is up to date."""
    if os.path.exists(path):
        subprocess.check_call([hg, 'pull', repo], cwd=path)
    else:
        subprocess.check_call([hg, 'clone', repo, path])

    subprocess.check_call([hg, 'update', '-r', revision], cwd=path)


def update_git_repo(git, repo, path, revision='origin/master'):
    """Ensure a Git repository exists at a path and is up to date."""
    if os.path.exists(path):
        subprocess.check_call([git, 'fetch', '--all'], cwd=path)
    else:
        subprocess.check_call([git, 'clone', repo, path])

    subprocess.check_call([git, 'checkout', revision], cwd=path)
