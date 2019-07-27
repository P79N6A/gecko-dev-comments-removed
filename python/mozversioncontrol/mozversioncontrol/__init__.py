



from __future__ import unicode_literals

import os
import re
import subprocess

from distutils.version import LooseVersion

def get_hg_version(hg):
    """Obtain the version of the Mercurial client."""

    env = os.environ.copy()
    env[b'HGPLAIN'] = b'1'

    info = subprocess.check_output([hg, '--version'], env=env)
    match = re.search('version ([^\+\)]+)', info)
    if not match:
        raise Exception('Unable to identify Mercurial version.')

    return LooseVersion(match.group(1))
