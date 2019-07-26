



from mozrunner import Runner


class GeckoInstance(object):

    def __init__(self, host, port, bin, profile):
        self.marionette_host = host
        self.marionette_port = port
        self.bin = bin
        self.profile = profile
        self.runner = None

    def start(self):
        profile = self.profile
        if not profile:
            prefs = {"marionette.defaultPrefs.enabled": True,
                     "marionette.defaultPrefs.port": 2828,
                     "browser.warnOnQuit": False}
            profile = {"preferences": prefs, "restore":False}
        else:
            profile = {"profile": profile}
        print "starting runner"
        self.runner = Runner.create(binary=self.bin, profile_args=profile, cmdargs=['-no-remote'])
        self.runner.start()

    def close(self):
        self.runner.stop()
        self.runner.cleanup()
