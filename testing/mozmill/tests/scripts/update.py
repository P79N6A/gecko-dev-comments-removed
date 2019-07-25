





































import copy
import datetime
import mozmill
import optparse
import os
import sys

try:
    import json
except:
    import simplejson as json

abs_path = os.path.dirname(os.path.abspath(__file__))


sys.path.append(os.path.join(abs_path, "libs"))
from install import Installer
from prefs import UpdateChannel

class SoftwareUpdateCLI(mozmill.RestartCLI):
    app_binary = {'darwin' : '', 'linux2' : '/firefox', 'win32' : '/firefox.exe'}
    test_path = abs_path + "/../firefox/softwareUpdate/"

    
    parser_options = copy.copy(mozmill.RestartCLI.parser_options)
    parser_options.pop(("-s", "--shell"))
    parser_options.pop(("-t", "--test",))
    parser_options.pop(("-u", "--usecode"))

    parser_options[("-c", "--channel",)] = dict(dest="channel", default=None, 
                                           help="Update channel (betatest, beta, nightly, releasetest, release)")
    parser_options[("--no-fallback",)] = dict(dest="no_fallback", default=None,
                                              action = "store_true",
                                              help="No fallback update should be performed")
    parser_options[("-t", "--type",)] = dict(dest="type", default="minor",
                                             nargs = 1,
                                             help="Type of the update (minor, major)")
    parser_options[("-i", "--install",)] = dict(dest="install", default=None,
                                                nargs = 1,
                                                help="Installation folder for the build")

    def __init__(self):
        super(SoftwareUpdateCLI, self).__init__()
        self.options.shell = None
        self.options.usecode = None
        self.options.plugins = None

        
        if len(self.args) < 1:
            print "No arguments specified. Please run with --help to see all options"
            sys.exit(1)

        
        
        if self.options.install and os.path.isdir(self.args[0]):
            folder = self.args.pop(0)
            files = os.listdir(folder)

            if files is None: files = []
            for file in files:
                full_path = os.path.join(folder, file)
                if os.path.isfile(full_path):
                    self.args.append(full_path)

        
        if self.options.type != "minor" and self.options.type != "major":
            self.options.type = "minor"

    def prepare_channel(self):
        channel = UpdateChannel()
        channel.setFolder(self.options.folder)

        if self.options.channel is None:
            self.channel = channel.getChannel()
        else:
            channel.setChannel(self.options.channel)
            self.channel = self.options.channel

    def prepare_build(self, binary):
        ''' Prepare the build for the test run '''
        if self.options.install is not None:
            self.options.folder = Installer().install(binary, self.options.install)
            self.options.binary = self.options.folder + self.app_binary[sys.platform]
        else:
            folder = os.path.dirname(binary)
            self.options.folder = folder if not os.path.isdir(binary) else binary
            self.options.binary = binary

    def cleanup_build(self):
        
        if self.options.install:
            Installer().uninstall(self.options.folder)

    def build_wiki_entry(self, results):
        entry = "* %s => %s, %s, %s, %s, %s, %s, '''%s'''\n" \
                "** %s ID:%s\n** %s ID:%s\n" \
                "** Passed %d :: Failed %d :: Skipped %d\n" % \
                (results.get("preVersion", ""),
                 results.get("postVersion", ""),
                 results.get("type"),
                 results.get("preLocale", ""),
                 results.get("updateType", "unknown type"),
                 results.get("channel", ""),
                 datetime.date.today(),
                 "PASS" if results.get("success", False) else "FAIL",
                 results.get("preUserAgent", ""), results.get("preBuildId", ""),
                 results.get("postUserAgent", ""), results.get("postBuildId", ""),
                 len(results.get("passes")),
                 len(results.get("fails")),
                 len(results.get("skipped")))
        return entry

    def run_test(self, binary, is_fallback = False, *args, **kwargs):
        try:
            self.prepare_build(binary)
            self.prepare_channel()

            self.mozmill.passes = []
            self.mozmill.fails = []
            self.mozmill.skipped = []
            self.mozmill.alltests = []

            self.mozmill.persisted = {}
            self.mozmill.persisted["channel"] = self.channel
            self.mozmill.persisted["type"] = self.options.type

            if is_fallback:
                self.options.test = self.test_path + "testFallbackUpdate/"
            else:
                self.options.test = self.test_path + "testDirectUpdate/"

            super(SoftwareUpdateCLI, self)._run(*args, **kwargs)
        except Exception, e:
            print e

        self.cleanup_build()

        
        if self.mozmill.fails:
            self.mozmill.persisted["success"] = False

        self.mozmill.persisted["passes"] = self.mozmill.passes
        self.mozmill.persisted["fails"] = self.mozmill.fails
        self.mozmill.persisted["skipped"] = self.mozmill.skipped

        return self.mozmill.persisted

    def run(self, *args, **kwargs):
        ''' Run software update tests for all specified builds '''

        
        self.wiki = []
        for binary in self.args:
            direct = self.run_test(binary, False)
            result_direct = direct.get("success", False);

            if not self.options.no_fallback:
                fallback = self.run_test(binary, True)
                result_fallback = fallback.get("success", False)
            else:
                result_fallback = False

            if not (result_direct and result_fallback):
                self.wiki.append(self.build_wiki_entry(direct))
            if not self.options.no_fallback:
                self.wiki.append(self.build_wiki_entry(fallback))

        
        print "\nResults:\n========"
        for result in self.wiki:
            print result

if __name__ == "__main__":
    SoftwareUpdateCLI().run()
