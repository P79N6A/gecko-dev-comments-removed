



from __future__ import unicode_literals

import os


def parse_test_path(test_path, topsrcdir):
    """Returns a dict containing:
    * 'normalized': the normalized path, relative to the topsrcdir
    * 'isdir': whether the path points to a directory
    """
    is_dir = os.path.isdir(test_path)

    normalized = os.path.normpath(test_path)
    topsrcdir = os.path.normpath(topsrcdir)

    if normalized.startswith(topsrcdir):
        normalized = normalized[len(topsrcdir):]

    return {
        'normalized': normalized,
        'is_dir': is_dir,
    }

