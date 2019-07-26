



from __future__ import unicode_literals

import os
import subprocess

import pymake.parser
from pymake.data import Makefile

from mach.mixin.process import ProcessExecutionMixin


class MozconfigLoader(ProcessExecutionMixin):
    """Handles loading and parsing of mozconfig files."""

    def __init__(self, topsrcdir):
        self.topsrcdir = topsrcdir

    @property
    def _client_mk_loader_path(self):
        return os.path.join(self.topsrcdir, 'build', 'autoconf',
            'mozconfig2client-mk')

    def read_mozconfig(self, path=None):
        env = dict(os.environ)
        if path is not None:
            
            env[b'MOZCONFIG'] = path

        args = self._normalize_command([self._client_mk_loader_path,
            self.topsrcdir], True)

        output = subprocess.check_output(args, stderr=subprocess.PIPE,
            cwd=self.topsrcdir, env=env)

        
        
        statements = pymake.parser.parsestring(output, 'mozconfig')

        makefile = Makefile(workdir=self.topsrcdir, env={
            'TOPSRCDIR': self.topsrcdir})

        statements.execute(makefile)

        result = {
            'topobjdir': None,
        }

        def get_value(name):
            exp = makefile.variables.get(name)[2]

            return exp.resolvestr(makefile, makefile.variables)

        for name, flavor, source, value in makefile.variables:
            
            if source != pymake.data.Variables.SOURCE_MAKEFILE:
                continue

            
            if name in ('.PYMAKE', 'MAKELEVEL', 'MAKEFLAGS'):
                continue

            if name == 'MOZ_OBJDIR':
                result['topobjdir'] = get_value(name)

            
            

        return result
