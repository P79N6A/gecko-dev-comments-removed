

"""
test nonce in prefs delimeters
see https://bugzilla.mozilla.org/show_bug.cgi?id=722804
"""

import os
import tempfile
import unittest
import mozfile
from mozprofile.prefs import Preferences
from mozprofile.profile import Profile

class PreferencesNonceTest(unittest.TestCase):

    def test_nonce(self):

        
        path = tempfile.mktemp()
        self.addCleanup(mozfile.remove, path)
        profile = Profile(path,
                          preferences={'foo': 'bar'},
                          restore=False)
        user_js = os.path.join(profile.profile, 'user.js')
        self.assertTrue(os.path.exists(user_js))

        
        prefs = Preferences.read_prefs(user_js)
        self.assertEqual(dict(prefs), {'foo': 'bar'})

        del profile

        
        profile = Profile(path,
                          preferences={'fleem': 'baz'},
                          restore=True)
        prefs = Preferences.read_prefs(user_js)
        self.assertEqual(dict(prefs), {'foo': 'bar', 'fleem': 'baz'})

        
        
        profile.cleanup()
        prefs = Preferences.read_prefs(user_js)
        self.assertEqual(dict(prefs), {'foo': 'bar'})

if __name__ == '__main__':
    unittest.main()
