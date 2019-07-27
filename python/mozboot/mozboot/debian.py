



from mozboot.base import BaseBootstrapper

class DebianBootstrapper(BaseBootstrapper):
    
    
    COMMON_PACKAGES = [
        'autoconf2.13',
        'build-essential',
        'ccache',
        'mercurial',
        'python-dev',
        'python-setuptools',
        'unzip',
        'uuid',
        'zip',
    ]

    
    DISTRO_PACKAGES = []

    BROWSER_COMMON_PACKAGES = [
        'libasound2-dev',
        'libcurl4-openssl-dev',
        'libdbus-1-dev',
        'libdbus-glib-1-dev',
        'libgconf2-dev',
        'libgstreamer0.10-dev',
        'libgstreamer-plugins-base0.10-dev',
        'libgtk2.0-dev',
        'libiw-dev',
        'libnotify-dev',
        'libpulse-dev',
        'libxt-dev',
        'mesa-common-dev',
        'python-dbus',
        'yasm',
        'xvfb',
    ]

    
    BROWSER_DISTRO_PACKAGES = []

    def __init__(self, version, dist_id):
        BaseBootstrapper.__init__(self)

        self.version = version
        self.dist_id = dist_id

        self.packages = self.COMMON_PACKAGES + self.DISTRO_PACKAGES
        self.browser_packages = self.BROWSER_COMMON_PACKAGES + self.BROWSER_DISTRO_PACKAGES

    def install_system_packages(self):
        self.apt_install(*self.packages)

    def install_browser_packages(self):
        self.apt_install(*self.browser_packages)

    def _update_package_manager(self):
        self.run_as_root(['apt-get', 'update'])

