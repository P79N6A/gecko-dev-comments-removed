



from mozboot.base import BaseBootstrapper


class OpenBSDBootstrapper(BaseBootstrapper):
    def __init__(self, version, **kwargs):
        BaseBootstrapper.__init__(self, **kwargs)

        self.packages = [
            'mercurial',
            'autoconf-2.13',
            'gmake',
            'gtar',
            'wget',
            'unzip',
            'zip',
        ]

        self.browser_packages = [
            'llvm',
            'yasm',
            'gtk+2',
            'dbus-glib',
            'gstreamer-plugins-base',
            'pulseaudio',
        ]

    def install_system_packages(self):
        
        self.run_as_root(['pkg_add', '-z'] + self.packages)

    def install_browser_packages(self):
        
        self.run_as_root(['pkg_add', '-z'] + self.browser_packages)
