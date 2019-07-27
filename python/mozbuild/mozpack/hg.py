




























from __future__ import absolute_import

import mercurial.error as error
import mercurial.hg as hg
import mercurial.ui as hgui

from .files import (
    BaseFinder,
    MercurialFile,
)
import mozpack.path as mozpath




class MercurialNativeFile(MercurialFile):
    def __init__(self, data):
        self.data = data

    def read(self):
        return self.data


class MercurialNativeRevisionFinder(BaseFinder):
    def __init__(self, repo, rev='.', recognize_repo_paths=False):
        """Create a finder attached to a specific changeset.

        Accepts a Mercurial localrepo and changectx instance.
        """
        if isinstance(repo, (str, unicode)):
            path = repo
            repo = hg.repository(hgui.ui(), repo)
        else:
            path = repo.root

        super(MercurialNativeRevisionFinder, self).__init__(base=repo.root)

        self._repo = repo
        self._rev = rev
        self._root = mozpath.normpath(path)
        self._recognize_repo_paths = recognize_repo_paths

    def _find(self, pattern):
        if self._recognize_repo_paths:
            raise NotImplementedError('cannot use find with recognize_repo_path')

        return self._find_helper(pattern, self._repo[self._rev], self._get)

    def get(self, path):
        if self._recognize_repo_paths:
            if not path.startswith(self._root):
                raise ValueError('lookups in recognize_repo_paths mode must be '
                                 'prefixed with repo path: %s' % path)
            path = path[len(self._root) + 1:]

        return self._get(path)

    def _get(self, path):
        if isinstance(path, unicode):
            path = path.encode('utf-8', 'replace')

        try:
            fctx = self._repo.filectx(path, self._rev)
            return MercurialNativeFile(fctx.data())
        except error.LookupError:
            return None
