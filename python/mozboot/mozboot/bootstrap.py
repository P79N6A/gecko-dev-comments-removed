




from __future__ import print_function

import platform
import sys

from mozboot.centos import CentOSBootstrapper
from mozboot.debian import DebianBootstrapper
from mozboot.fedora import FedoraBootstrapper
from mozboot.freebsd import FreeBSDBootstrapper
from mozboot.gentoo import GentooBootstrapper
from mozboot.osx import OSXBootstrapper
from mozboot.openbsd import OpenBSDBootstrapper
from mozboot.ubuntu import UbuntuBootstrapper


FINISHED = '''
Your system should be ready to build Firefox! If you have not already,
obtain a copy of the source code by running:

    hg clone https://hg.mozilla.org/mozilla-central

Or, if you prefer Git:

    git clone git://github.com/mozilla/mozilla-central.git
'''


class Bootstrapper(object):
    """Main class that performs system bootstrap."""

    def bootstrap(self):
        cls = None
        args = {}

        if sys.platform.startswith('linux'):
            distro, version, dist_id = platform.linux_distribution()

            if distro == 'CentOS':
                cls = CentOSBootstrapper
            elif distro in ('Debian', 'debian'):
                cls = DebianBootstrapper
            elif distro == 'Fedora':
                cls = FedoraBootstrapper
            elif distro == 'Gentoo Base System':
                cls = GentooBootstrapper
            elif distro in ('Mint', 'LinuxMint'):
                
                
                if dist_id == 'debian':
                    cls = DebianBootstrapper
                else:
                    cls = UbuntuBootstrapper
            elif distro == 'Ubuntu':
                cls = UbuntuBootstrapper
            elif distro == 'Elementary':
                cls = UbuntuBootstrapper
            else:
                raise NotImplementedError('Bootstrap support for this Linux '
                                          'distro not yet available.')

            args['version'] = version
            args['dist_id'] = dist_id

        elif sys.platform.startswith('darwin'):
            
            osx_version = platform.mac_ver()[0]

            cls = OSXBootstrapper
            args['version'] = osx_version

        elif sys.platform.startswith('openbsd'):
            cls = OpenBSDBootstrapper
            args['version'] = platform.uname()[2]

        elif sys.platform.startswith('freebsd'):
            cls = FreeBSDBootstrapper
            args['version'] = platform.release()

        if cls is None:
            raise NotImplementedError('Bootstrap support is not yet available '
                                      'for your OS.')

        instance = cls(**args)
        instance.install_system_packages()
        instance.ensure_mercurial_modern()
        instance.ensure_python_modern()

        print(FINISHED)
