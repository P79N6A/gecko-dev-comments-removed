





import unittest
import os
from mozprofile import Profile


class TestProfile(unittest.TestCase):
    def test_with_profile_should_cleanup(self):
        with Profile() as profile:
            self.assertTrue(os.path.exists(profile.profile))
        
        self.assertFalse(os.path.exists(profile.profile))

    def test_with_profile_should_cleanup_even_on_exception(self):
        with self.assertRaises(ZeroDivisionError):
            with Profile() as profile:
                self.assertTrue(os.path.exists(profile.profile))
                1/0  
        
        self.assertFalse(os.path.exists(profile.profile))


if __name__ == '__main__':
    unittest.main()
