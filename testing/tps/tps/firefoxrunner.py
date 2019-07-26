




import copy
import httplib2
import os

import mozfile
import mozinstall
from mozprofile import Profile
from mozrunner import FirefoxRunner


class TPSFirefoxRunner(object):

    PROCESS_TIMEOUT = 240

    def __init__(self, binary):
        if binary is not None and ('http://' in binary or 'ftp://' in binary):
            self.url = binary
            self.binary = None
        else:
            self.url = None
            self.binary = binary

        self.runner = None
        self.installdir = None

    def __del__(self):
        if self.installdir:
            mozfile.remove(self.installdir, True)

    def download_url(self, url, dest=None):
        h = httplib2.Http()
        resp, content = h.request(url, 'GET')
        if dest == None:
            dest = os.path.basename(url)

        local = open(dest, 'wb')
        local.write(content)
        local.close()
        return dest

    def download_build(self, installdir='downloadedbuild', appname='firefox'):
        self.installdir = os.path.abspath(installdir)
        buildName = os.path.basename(self.url)
        pathToBuild = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                   buildName)

        
        if os.access(pathToBuild, os.F_OK):
            os.remove(pathToBuild)

        
        print 'downloading build'
        self.download_url(self.url, pathToBuild)

        
        print 'installing %s' % pathToBuild
        mozfile.remove(self.installdir, True)
        binary = mozinstall.install(src=pathToBuild, dest=self.installdir)

        
        os.remove(pathToBuild)

        return binary

    def run(self, profile=None, timeout=PROCESS_TIMEOUT, env=None, args=None):
        """Runs the given FirefoxRunner with the given Profile, waits
           for completion, then returns the process exit code
        """
        if profile is None:
            profile = Profile()
        self.profile = profile

        if self.binary is None and self.url:
            self.binary = self.download_build()

        if self.runner is None:
            self.runner = FirefoxRunner(profile=self.profile, binary=self.binary, env=env, cmdargs=args)

        self.runner.start(timeout=timeout)
        return self.runner.wait()
