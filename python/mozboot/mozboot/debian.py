



import os
import sys

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

    
    
    MOBILE_ANDROID_COMMON_PACKAGES = [
        'zlib1g-dev',  
        'openjdk-7-jdk',
        'ant',
        'wget',  
        'libncurses5:i386',  
        'libstdc++6:i386',
        'zlib1g:i386',
    ]

    
    MOBILE_ANDROID_DISTRO_PACKAGES = []

    def __init__(self, version, dist_id, **kwargs):
        BaseBootstrapper.__init__(self, **kwargs)

        self.version = version
        self.dist_id = dist_id

        self.packages = self.COMMON_PACKAGES + self.DISTRO_PACKAGES
        self.browser_packages = self.BROWSER_COMMON_PACKAGES + self.BROWSER_DISTRO_PACKAGES
        self.mobile_android_packages = self.MOBILE_ANDROID_COMMON_PACKAGES + self.MOBILE_ANDROID_DISTRO_PACKAGES


    def install_system_packages(self):
        self.apt_install(*self.packages)

    def install_browser_packages(self):
        self.apt_install(*self.browser_packages)

    def install_mobile_android_packages(self):
        import android

        
        
        
        

        
        
        
        
        
        self.run_as_root(['dpkg', '--add-architecture', 'i386'])
        
        self.apt_update()
        self.apt_install(*self.mobile_android_packages)

        
        
        
        mozbuild_path = os.environ.get('MOZBUILD_STATE_PATH', os.path.expanduser(os.path.join('~', '.mozbuild')))
        self.sdk_path = os.environ.get('ANDROID_SDK_HOME', os.path.join(mozbuild_path, 'android-sdk-linux'))
        self.ndk_path = os.environ.get('ANDROID_NDK_HOME', os.path.join(mozbuild_path, 'android-ndk-r8e'))
        self.sdk_url = 'https://dl.google.com/android/android-sdk_r24.0.1-linux.tgz'
        is_64bits = sys.maxsize > 2**32
        if is_64bits:
            self.ndk_url = 'https://dl.google.com/android/ndk/android-ndk-r8e-linux-x86_64.tar.bz2'
        else:
            self.ndk_url = 'https://dl.google.com/android/ndk/android-ndk-r8e-linux-x86.tar.bz2'
        android.ensure_android_sdk_and_ndk(path=mozbuild_path,
                                           sdk_path=self.sdk_path, sdk_url=self.sdk_url,
                                           ndk_path=self.ndk_path, ndk_url=self.ndk_url)

        
        
        android_tool = os.path.join(self.sdk_path, 'tools', 'android')
        android.ensure_android_packages(android_tool=android_tool)

    def suggest_mobile_android_mozconfig(self):
        import android

        
        sdk_path = os.path.join(self.sdk_path, 'platforms', android.ANDROID_PLATFORM)
        android.suggest_mozconfig(sdk_path=sdk_path,
                                  ndk_path=self.ndk_path)

    def _update_package_manager(self):
        self.apt_update()
