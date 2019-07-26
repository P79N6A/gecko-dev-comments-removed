



from mozboot.base import BaseBootstrapper

class FreeBSDBootstrapper(BaseBootstrapper):
    def __init__(self, version):
        BaseBootstrapper.__init__(self)
        self.version = int(version.split('.')[0])

        self.packages = [
            'autoconf213',
            'dbus-glib',
            'gmake',
            'gstreamer-plugins',
            'gtk2',
            'libGL',
            'mercurial',
            'pkgconf',
            'pulseaudio',
            'v4l_compat',
            'yasm',
            'zip',
        ]

        
        if self.version < 9:
            self.packages.append('gcc')

    def pkg_install(self, *packages):
        if self.which('pkg'):
            command = ['pkg', 'install']
        else:
            command = ['pkg_add', '-Fr']

        command.extend(packages)
        self.run_as_root(command)

    def install_system_packages(self):
        self.pkg_install(*self.packages)

    def upgrade_mercurial(self, current):
        self.pkg_install('mercurial')
