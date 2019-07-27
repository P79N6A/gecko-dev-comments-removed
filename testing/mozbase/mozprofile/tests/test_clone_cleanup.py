






import os
import tempfile
import unittest

from mozprofile.profile import Profile


class CloneCleanupTest(unittest.TestCase):
    """
    test cleanup logic for the clone functionality
    see https://bugzilla.mozilla.org/show_bug.cgi?id=642843
    """

    def setUp(self):
        
        path = tempfile.mktemp()
        self.profile = Profile(path,
                          preferences={'foo': 'bar'},
                          restore=False)
        user_js = os.path.join(self.profile.profile, 'user.js')
        self.assertTrue(os.path.exists(user_js))

    def test_restore_true(self):
        
        clone = Profile.clone(self.profile.profile, restore=True)

        clone.cleanup()

        
        self.assertFalse(os.path.exists(clone.profile))

    def test_restore_false(self):
        
        clone = Profile.clone(self.profile.profile, restore=False)

        clone.cleanup()

        
        self.assertTrue(os.path.exists(clone.profile))

    def test_cleanup_on_garbage_collected(self):
        clone = Profile.clone(self.profile.profile)
        profile_dir = clone.profile
        self.assertTrue(os.path.exists(profile_dir))
        del clone
        
        self.assertFalse(os.path.exists(profile_dir))


if __name__ == '__main__':
    unittest.main()

