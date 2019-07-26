



from __future__ import unicode_literals

from configobj import ConfigObj
import re
import os


BUGZILLA_FINGERPRINT = '47:13:a2:14:0c:46:45:53:12:0d:e5:36:16:a5:60:26:3e:da:3a:60'
HG_FINGERPRINT = '10:78:e8:57:2d:95:de:7c:de:90:bd:22:e1:38:17:67:c5:a7:9c:14'


class MercurialConfig(object):
    """Interface for manipulating a Mercurial config file."""

    def __init__(self, infiles=None):
        """Create a new instance, optionally from an existing hgrc file."""

        if infiles:
            
            if len(infiles) > 1:
                picky_infiles = filter(os.path.isfile, infiles)
                if picky_infiles:
                    picky_infiles = [(os.path.getsize(path), path) for path in picky_infiles]
                    infiles = [max(picky_infiles)[1]]

            infile = infiles[0]
            self.config_path = infile
        else:
            infile = None

        
        
        
        
        self._c = ConfigObj(infile=infile, encoding='utf-8',
            write_empty_values=True, list_values=False)

    @property
    def config(self):
        return self._c

    @property
    def extensions(self):
        """Returns the set of currently enabled extensions (by name)."""
        return set(self._c.get('extensions', {}).keys())

    def write(self, fh):
        return self._c.write(fh)

    def have_valid_username(self):
        if 'ui' not in self._c:
            return False

        if 'username' not in self._c['ui']:
            return False

        

        return True

    def add_mozilla_host_fingerprints(self):
        """Add host fingerprints so SSL connections don't warn."""
        if 'hostfingerprints' not in self._c:
            self._c['hostfingerprints'] = {}

        self._c['hostfingerprints']['bugzilla.mozilla.org'] = \
            BUGZILLA_FINGERPRINT
        self._c['hostfingerprints']['hg.mozilla.org'] = HG_FINGERPRINT

    def set_username(self, name, email):
        """Set the username to use for commits.

        The username consists of a name (typically <firstname> <lastname>) and
        a well-formed e-mail address.
        """
        if 'ui' not in self._c:
            self._c['ui'] = {}

        username = '%s <%s>' % (name, email)

        self._c['ui']['username'] = username.strip()

    def activate_extension(self, name, path=None):
        """Activate an extension.

        An extension is defined by its name (in the config) and a filesystem
        path). For built-in extensions, an empty path is specified.
        """
        if not path:
            path = ''

        if 'extensions' not in self._c:
            self._c['extensions'] = {}

        self._c['extensions'][name] = path

    def have_recommended_diff_settings(self):
        if 'diff' not in self._c:
            return False

        old = dict(self._c['diff'])
        try:
            self.ensure_recommended_diff_settings()
        finally:
            self._c['diff'].update(old)

        return self._c['diff'] == old

    def ensure_recommended_diff_settings(self):
        if 'diff' not in self._c:
            self._c['diff'] = {}

        d = self._c['diff']
        d['git'] = 1
        d['showfunc'] = 1
        d['unified'] = 8

    def autocommit_mq(self, value=True):
        if 'mqext' not in self._c:
            self._c['mqext'] = {}

        if value:
            self._c['mqext']['mqcommit'] = 'auto'
        else:
            try:
                del self._c['mqext']['mqcommit']
            except KeyError:
                pass


    def have_qnew_currentuser_default(self):
        if 'defaults' not in self._c:
            return False
        d = self._c['defaults']
        if 'qnew' not in d:
            return False
        argv = d['qnew'].split(' ')
        for arg in argv:
            if arg == '--currentuser' or re.match("-[^-]*U.*", arg):
                return True
        return False

    def ensure_qnew_currentuser_default(self):
        if self.have_qnew_currentuser_default():
            return
        if 'defaults' not in self._c:
            self._c['defaults'] = {}

        d = self._c['defaults']
        if 'qnew' not in d:
          d['qnew'] = '-U'
        else:
          d['qnew'] = '-U ' + d['qnew']


