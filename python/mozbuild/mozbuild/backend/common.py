



from __future__ import unicode_literals

import json
import os

import mozpack.path as mozpath

from .base import BuildBackend

from ..frontend.data import (
    TestManifest,
    XPIDLFile,
)

from ..util import DefaultOnReadDict


class XPIDLManager(object):
    """Helps manage XPCOM IDLs in the context of the build system."""
    def __init__(self, config):
        self.config = config
        self.topsrcdir = config.topsrcdir
        self.topobjdir = config.topobjdir

        self.idls = {}
        self.modules = {}

    def register_idl(self, source, module, allow_existing=False):
        """Registers an IDL file with this instance.

        The IDL file will be built, installed, etc.
        """
        basename = mozpath.basename(source)
        root = mozpath.splitext(basename)[0]

        entry = {
            'source': source,
            'module': module,
            'basename': basename,
            'root': root,
        }

        if not allow_existing and entry['basename'] in self.idls:
            raise Exception('IDL already registered: %' % entry['basename'])

        self.idls[entry['basename']] = entry
        self.modules.setdefault(entry['module'], set()).add(entry['root'])


class TestManager(object):
    """Helps hold state related to tests."""

    def __init__(self, config):
        self.config = config
        self.topsrcdir = mozpath.normpath(config.topsrcdir)

        self.tests_by_path = DefaultOnReadDict({}, global_default=[])

    def add(self, t, flavor=None, topsrcdir=None):
        t = dict(t)
        t['flavor'] = flavor

        if topsrcdir is None:
            topsrcdir = self.topsrcdir
        else:
            topsrcdir = mozpath.normpath(topsrcdir)

        path = mozpath.normpath(t['path'])
        assert path.startswith(topsrcdir)

        key = path[len(topsrcdir)+1:]
        t['file_relpath'] = key
        t['dir_relpath'] = mozpath.dirname(key)

        self.tests_by_path[key].append(t)


class CommonBackend(BuildBackend):
    """Holds logic common to all build backends."""

    def _init(self):
        self._idl_manager = XPIDLManager(self.environment)
        self._test_manager = TestManager(self.environment)

    def consume_object(self, obj):
        if isinstance(obj, TestManifest):
            for test in obj.tests:
                self._test_manager.add(test, flavor=obj.flavor,
                    topsrcdir=obj.topsrcdir)

        if isinstance(obj, XPIDLFile):
            self._idl_manager.register_idl(obj.source_path, obj.module)

    def consume_finished(self):
        if len(self._idl_manager.idls):
            self._handle_idl_manager(self._idl_manager)

        
        path = mozpath.join(self.environment.topobjdir, 'all-tests.json')
        with self._write_file(path) as fh:
            json.dump(self._test_manager.tests_by_path, fh, sort_keys=True)
