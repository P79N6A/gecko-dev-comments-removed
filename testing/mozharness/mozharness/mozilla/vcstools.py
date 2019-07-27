





"""vcstools.py

Author: Armen Zambrano G.
"""
import os
import stat

from mozharness.base.script import PreScriptAction
from mozharness.base.vcs.vcsbase import VCSScript

VCS_TOOLS = ('hgtool.py', 'gittool.py')


class VCSToolsScript(VCSScript):
    ''' This script allows us to fetch hgtool.py and gittool.py if
    we're running the script on developer mode.
    '''
    @PreScriptAction('checkout')
    def _pre_checkout(self, action):
        dirs = self.query_abs_dirs()

        if self.config.get('developer_mode'):
            
            
            for vcs_tool in VCS_TOOLS:
                file_path = self.query_exe(vcs_tool)
                if not os.path.exists(file_path):
                    self.download_file(
                        url=self.config[vcs_tool],
                        file_name=file_path,
                        parent_dir=os.path.dirname(file_path),
                        create_parent_dir=True,
                    )
                    self.chmod(file_path, 0755)
        else:
            for vcs_tool in VCS_TOOLS:
                file_path = self.which(vcs_tool)
                if file_path is None:
                    file_path = self.query_exe(vcs_tool)

                    if not self.is_exe(file_path):
                        self.critical("%s is not executable." % file_path)

                    self.fatal("This machine is missing %s, if this is your "
                               "local machine you can use --cfg "
                               "developer_config.py" % vcs_tool)
