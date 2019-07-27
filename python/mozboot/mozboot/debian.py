



from mozboot.base import BaseBootstrapper

class DebianBootstrapper(BaseBootstrapper):
    
    
    COMMON_PACKAGES = [
        'autoconf2.13',
        'build-essential',
        'ccache',
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
        'mercurial',
        'mesa-common-dev',
        'python-dbus',
        'python-dev',
        'python-setuptools',
        'unzip',
        'uuid',
        'yasm',
        'xvfb',
        'zip',
    ]

    
    DISTRO_PACKAGES = []

    def __init__(self, version, dist_id):
        BaseBootstrapper.__init__(self)

        self.version = version
        self.dist_id = dist_id

        self.packages = self.COMMON_PACKAGES + self.DISTRO_PACKAGES

    def install_system_packages(self):
        self.apt_install(*self.packages)

    def _update_package_manager(self):
        
        print 'lala'

