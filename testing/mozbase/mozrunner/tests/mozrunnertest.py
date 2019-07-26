



import os
import unittest

import mozprofile
import mozrunner


@unittest.skipIf(not os.environ.get('BROWSER_PATH'),
                 'No binary has been specified.')
class MozrunnerTestCase(unittest.TestCase):

    def setUp(self):
        self.pids = []
        self.threads = [ ]

        self.profile = mozprofile.FirefoxProfile()
        self.runner = mozrunner.FirefoxRunner(self.profile)

    def tearDown(self):
        for thread in self.threads:
            thread.join()

        self.runner.cleanup()

        
        for pid in self.pids:
            
            
            pass
