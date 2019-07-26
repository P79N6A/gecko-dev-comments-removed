













from __future__ import print_function

import os
import shutil
import sys
import tempfile
import urllib2

from optparse import OptionParser




REPOSITORY_PATH_PREFIX = 'python/mozboot'

REPOSITORY_PATHS = [
    'mozboot/__init__.py',
    'mozboot/base.py',
    'mozboot/bootstrap.py',
    'mozboot/centos.py',
    'mozboot/fedora.py',
    'mozboot/gentoo.py',
    'mozboot/mint.py',
    'mozboot/openbsd.py',
    'mozboot/osx.py',
    'mozboot/ubuntu.py',
]

TEMPDIR = None

def fetch_files(repo_url, repo_type):
    repo_url = repo_url.rstrip('/')

    files = {}

    if repo_type == 'hgweb':
        for path in REPOSITORY_PATHS:
            url = repo_url + '/raw-file/default/python/mozboot/' + path

            req = urllib2.urlopen(url=url, timeout=30)
            files[path] = req.read()
    else:
        raise NotImplementedError('Not sure how to handle repo type.', repo_type)

    return files

def ensure_environment(repo_url=None, repo_type=None):
    """Ensure we can load the Python modules necessary to perform bootstrap."""

    try:
        from mozboot.bootstrap import Bootstrapper
        return Bootstrapper
    except ImportError:
        
        
        pardir = os.path.join(os.path.dirname(__file__), os.path.pardir)
        include = os.path.normpath(pardir)

        sys.path.append(include)
        try:
            from mozboot.bootstrap import Bootstrapper
            return Bootstrapper
        except ImportError:
            sys.path.pop()

            
            
            files = fetch_files(repo_url, repo_type)

            
            
            global TEMPDIR
            TEMPDIR = tempfile.mkdtemp()

            for relpath in files.keys():
                destpath = os.path.join(TEMPDIR, relpath)
                destdir = os.path.dirname(destpath)

                if not os.path.exists(destdir):
                    os.makedirs(destdir)

                with open(destpath, 'wb') as fh:
                    fh.write(files[relpath])

            
            sys.path.append(TEMPDIR)
            from mozboot.bootstrap import Bootstrapper
            return Bootstrapper

def main(args):
    parser = OptionParser()
    parser.add_option('-r', '--repo-url', dest='repo_url',
        default='https://hg.mozilla.org/mozilla-central/',
        help='Base URL of source control repository where bootstrap files can '
             'be downloaded.')

    parser.add_option('--repo-type', dest='repo_type',
        default='hgweb',
        help='The type of the repository. This defines how we fetch file '
             'content. Like --repo, you should not need to set this.')

    options, leftover = parser.parse_args(args)

    try:
        try:
            cls = ensure_environment(options.repo_url, options.repo_type)
        except Exception as e:
            print('Could not load the bootstrap Python environment.\n')
            print('This should never happen. Consider filing a bug.\n')
            print('\n')
            print(e)
            return 1

        dasboot = cls()
        dasboot.bootstrap()

        return 0
    finally:
        if TEMPDIR is not None:
            shutil.rmtree(TEMPDIR)


if __name__ == '__main__':
    sys.exit(main(sys.argv))
