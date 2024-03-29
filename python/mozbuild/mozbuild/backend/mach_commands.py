



from __future__ import absolute_import, print_function, unicode_literals

import argparse
import os
import sys
import subprocess
import which

from mozbuild.base import (
    MachCommandBase,
)

from mach.decorators import (
    CommandArgument,
    CommandProvider,
    Command,
)

@CommandProvider
class MachCommands(MachCommandBase):
    @Command('ide', category='devenv',
        description='Generate a project and launch an IDE.')
    @CommandArgument('ide', choices=['eclipse', 'visualstudio', 'androidstudio', 'intellij'])
    @CommandArgument('args', nargs=argparse.REMAINDER)
    def eclipse(self, ide, args):
        if ide == 'eclipse':
            backend = 'CppEclipse'
        elif ide == 'visualstudio':
            backend = 'VisualStudio'
        elif ide == 'androidstudio' or ide == 'intellij':
            
            backend = 'RecursiveMake'

        if ide == 'eclipse':
            try:
                which.which('eclipse')
            except which.WhichError:
                print('Eclipse CDT 8.4 or later must be installed in your PATH.')
                print('Download: http://www.eclipse.org/cdt/downloads.php')
                return 1
        elif ide == 'androidstudio' or ide =='intellij':
            studio = ['studio'] if ide == 'androidstudio' else ['idea']
            if sys.platform != 'darwin':
                try:
                    which.which(studio[0])
                except:
                    self.print_ide_error(ide)
                    return 1
            else:
                
                for d in self.get_mac_ide_preferences(ide):
                    if os.path.isdir(d):
                        studio = ['open', '-a', d]
                        break
                else:
                    print('Android Studio or IntelliJ IDEA 14 is not installed in /Applications.')
                    return 1

        
        
        
        res = self._mach_context.commands.dispatch('build', self._mach_context)
        if res != 0:
            return 1

        if ide == 'androidstudio' or 'intellij':
            res = self._mach_context.commands.dispatch('package', self._mach_context)
            if res != 0:
                return 1
            res = self._mach_context.commands.dispatch('gradle-install', self._mach_context)
            if res != 0:
                 return 1
        else:
            
            python = self.virtualenv_manager.python_path
            config_status = os.path.join(self.topobjdir, 'config.status')
            args = [python, config_status, '--backend=%s' % backend]
            res = self._run_command_in_objdir(args=args, pass_thru=True, ensure_exit_code=False)
            if res != 0:
                return 1


        if ide == 'eclipse':
            eclipse_workspace_dir = self.get_eclipse_workspace_path()
            process = subprocess.check_call(['eclipse', '-data', eclipse_workspace_dir])
        elif ide == 'visualstudio':
            visual_studio_workspace_dir = self.get_visualstudio_workspace_path()
            process = subprocess.check_call(['explorer.exe', visual_studio_workspace_dir])
        elif ide == 'androidstudio' or ide == 'intellij':
            gradle_dir = None
            if self.is_gradle_project_already_imported():
                gradle_dir = self.get_gradle_project_path()
            else:
                gradle_dir = self.get_gradle_import_path()
            process = subprocess.check_call(studio + [gradle_dir])

    def get_eclipse_workspace_path(self):
        from mozbuild.backend.cpp_eclipse import CppEclipseBackend
        return CppEclipseBackend.get_workspace_path(self.topsrcdir, self.topobjdir)

    def get_visualstudio_workspace_path(self):
        return os.path.join(self.topobjdir, 'msvc', 'mozilla.sln')

    def get_gradle_project_path(self):
        return os.path.join(self.topobjdir, 'mobile', 'android', 'gradle')

    def get_gradle_import_path(self):
        return os.path.join(self.get_gradle_project_path(), 'build.gradle')

    def is_gradle_project_already_imported(self):
        gradle_project_path = os.path.join(self.get_gradle_project_path(), '.idea')
        return os.path.exists(gradle_project_path)

    def get_mac_ide_preferences(self, ide):
        if sys.platform == 'darwin':
            if ide == 'androidstudio':
                return ['/Applications/Android Studio.app']
            else:
                return [
                    '/Applications/IntelliJ IDEA 14 EAP.app',
                    '/Applications/IntelliJ IDEA 14.app',
                    '/Applications/IntelliJ IDEA 14 CE EAP.app',
                    '/Applications/IntelliJ IDEA 14 CE.app']

    def print_ide_error(self, ide):
        if ide == 'androidstudio':
            print('Android Studio is not installed in your PATH.')
            print('You can generate a command-line launcher from Android Studio->Tools->Create Command-line launcher with script name \'studio\'')
        elif ide == 'intellij':
            print('IntelliJ is not installed in your PATH.')
            print('You can generate a command-line launcher from IntelliJ IDEA->Tools->Create Command-line launcher with script name \'idea\'')
