



from marionette_test import MarionetteTestCase

class TestGetStatus(MarionetteTestCase):
    def test_getStatus(self):
        status = self.marionette.status()
        self.assertTrue("os" in status)
        status_os = status['os']
        self.assertTrue("version" in status_os)
        self.assertTrue("name" in status_os)
        self.assertTrue("arch" in status_os)
        self.assertTrue("build" in status)
        status_build = status['build']
        self.assertTrue("revision" in status_build)
        self.assertTrue("time" in status_build)
        self.assertTrue("version" in status_build)
