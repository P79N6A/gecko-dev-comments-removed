



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
        self.virtualenv_manager.install_pip_package('mdn-sphinx-theme==0.4')

        from moztreedocs import SphinxManager

        if outdir == '<DEFAULT>':
            outdir = os.path.join(self.topobjdir, 'docs')

        manager = SphinxManager(self.topsrcdir, os.path.join(self.topsrcdir,
            'tools', 'docs'), outdir)

        
        
        def remove_gyp_dirs(context):
            context['GYP_DIRS'][:] = []

        reader = BuildReader(self.config_environment,
            sandbox_post_eval_cb=remove_gyp_dirs)

        for context in reader.walk_topsrcdir():
            for dest_dir, source_dir in context['SPHINX_TREES'].items():
                manager.add_tree(os.path.join(context.relsrcdir,
                    source_dir), dest_dir)

            for entry in context['SPHINX_PYTHON_PACKAGE_DIRS']:
                manager.add_python_package_dir(os.path.join(context.relsrcdir,
                    entry))

        return manager.generate_docs(format)
