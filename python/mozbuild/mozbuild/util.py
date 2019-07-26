






from __future__ import unicode_literals

import hashlib


def hash_file(path):
    """Hashes a file specified by the path given and returns the hex digest."""

    
    
    h = hashlib.sha1()

    with open(path, 'rb') as fh:
        while True:
            data = fh.read(8192)

            if not len(data):
                break

            h.update(data)

    return h.hexdigest()
