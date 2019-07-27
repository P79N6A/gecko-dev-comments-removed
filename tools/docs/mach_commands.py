



from __future__ import unicode_literals

import os

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)

from mozbuild.base import MachCommandBase
from mozbuild.frontend.reader import BuildReader


@CommandProvider
class Documentation(MachCommandBase):
    """Helps manage in-tree documentation."""

    @Command('build-docs', category='build-dev',
        description='Generate documentation for the tree.')
    @CommandArgument('--format', default='html',
        help='Documentation format to write.')
    @CommandArgument('outdir', default='<DEFAULT>', nargs='?',
        help='Where to write output.')
    def build_docs(self, format=None, outdir=None):
        self._activate_virtualenv()
        self.virtualenv_manager.install_pip_package('sphinx_rtd_theme==0.1.6')

        from moztreedocs import SphinxManager

        if outdir == '<DEFAULT>':
            outdir = os.path.join(self.topobjdir, 'docs')

        manager = SphinxManager(self.topsrcdir, os.path.join(self.topsrcdir,
            'tools', 'docs'), outdir)

        
        
        def remove_gyp_dirs(context):
            context['GYP_DIRS'][:] = []

        reader = BuildReader(self.config_environment,
            sandbox_post_eval_cb=remove_gyp_dirs)

        for path, name, key, value in reader.find_sphinx_variables():
            reldir = os.path.dirname(path)

            if name == 'SPHINX_TREES':
                assert key
                manager.add_tree(os.path.join(reldir, value),
                        os.path.join(reldir, key))

            if name == 'SPHINX_PYTHON_PACKAGE_DIRS':
                manager.add_python_package_dir(os.path.join(reldir, value))

        return manager.generate_docs(format)
