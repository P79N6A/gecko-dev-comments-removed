








































__all__ = ['Profile', 'FirefoxProfile', 'ThunderbirdProfile']

import os
import tempfile
from addons import AddonManager
from permissions import PermissionsManager
from shutil import rmtree

try:
    import simplejson
except ImportError:
    import json as simplejson

class Profile(object):
    """Handles all operations regarding profile. Created new profiles, installs extensions,
    sets preferences and handles cleanup."""

    def __init__(self, profile=None, addons=None, addon_manifests=None, preferences=None, locations=None, proxy=False, restore=True):

        
        self.restore = restore

        
        self.create_new = not profile
        if profile:
            
            self.profile = os.path.abspath(os.path.expanduser(profile))
            if not os.path.exists(self.profile):
                os.makedirs(self.profile)
        else:
            self.profile = self.create_new_profile()

        
        if hasattr(self.__class__, 'preferences'):
            
            self.set_preferences(self.__class__.preferences)
        self._preferences = preferences
        if preferences:
            
            if isinstance(preferences, dict):
                
                preferences = preferences.items()
            
            assert not [i for i in preferences
                        if len(i) != 2]
        else:
            preferences = []
        self.set_preferences(preferences)

        
        self._locations = locations 
        self._proxy = proxy
        self.permission_manager = PermissionsManager(self.profile, locations)
        prefs_js, user_js = self.permission_manager.getNetworkPreferences(proxy)
        self.set_preferences(prefs_js, 'prefs.js')
        self.set_preferences(user_js)

        
        self.addon_manager = AddonManager(self.profile)
        self.addon_manager.install_addons(addons, addon_manifests)

    def exists(self):
        """returns whether the profile exists or not"""
        return os.path.exists(self.profile)

    def reset(self):
        """
        reset the profile to the beginning state
        """
        self.cleanup()
        if self.create_new:
            profile = None
        else:
            profile = self.profile
        self.__init__(profile=profile,
                      addons=self.addon_manager.addons,
                      addon_manifests=self.addon_manager.manifests,
                      preferences=self._preferences,
                      locations=self._locations,
                      proxy = self._proxy)

    def create_new_profile(self):
        """Create a new clean profile in tmp which is a simple empty folder"""
        profile = tempfile.mkdtemp(suffix='.mozrunner')
        return profile


    

    def set_preferences(self, preferences, filename='user.js'):
        """Adds preferences dict to profile preferences"""

        
        prefs_file = os.path.join(self.profile, filename)
        f = open(prefs_file, 'a')

        if isinstance(preferences, dict):
            
            preferences = preferences.items()

        
        if preferences:
            f.write('\n#MozRunner Prefs Start\n')
            _prefs = [(simplejson.dumps(k), simplejson.dumps(v) )
                      for k, v in preferences]
            for _pref in _prefs:
                f.write('user_pref(%s, %s);\n' % _pref)
            f.write('#MozRunner Prefs End\n')
        f.close()

    def pop_preferences(self):
        """
        pop the last set of preferences added
        returns True if popped
        """

        
        delimeters = ('#MozRunner Prefs Start', '#MozRunner Prefs End')

        lines = file(os.path.join(self.profile, 'user.js')).read().splitlines()
        def last_index(_list, value):
            """
            returns the last index of an item;
            this should actually be part of python code but it isn't
            """
            for index in reversed(range(len(_list))):
                if _list[index] == value:
                    return index
        s = last_index(lines, delimeters[0])
        e = last_index(lines, delimeters[1])

        
        if s is None:
            assert e is None, '%s found without %s' % (delimeters[1], delimeters[0])
            return False 
        elif e is None:
            assert e is None, '%s found without %s' % (delimeters[0], delimeters[1])

        
        assert e > s, '%s found at %s, while %s found at %s' (delimeter[1], e, delimeter[0], s)

        
        cleaned_prefs = '\n'.join(lines[:s] + lines[e+1:])
        f = file(os.path.join(self.profile, 'user.js'), 'w')
        return True

    def clean_preferences(self):
        """Removed preferences added by mozrunner."""
        while True:
            if not self.pop_preferences():
                break

    

    def _cleanup_error(self, function, path, excinfo):
        """ Specifically for windows we need to handle the case where the windows
            process has not yet relinquished handles on files, so we do a wait/try
            construct and timeout if we can't get a clear road to deletion
        """
        try:
            from exceptions import WindowsError
            from time import sleep
            def is_file_locked():
                return excinfo[0] is WindowsError and excinfo[1].winerror == 32

            if excinfo[0] is WindowsError and excinfo[1].winerror == 32:
                
                
                count = 0
                while count < 10:
                    sleep(1)
                    try:
                        function(path)
                        break
                    except:
                        count += 1
        except ImportError:
            
            pass


    def cleanup(self):
        """Cleanup operations on the profile."""
        if self.restore:
            if self.create_new:
                if os.path.exists(self.profile):
                    rmtree(self.profile, onerror=self._cleanup_error)
            else:
                self.clean_preferences()
                self.addon_manager.clean_addons()
                self.permission_manager.clean_permissions()

    __del__ = cleanup

class FirefoxProfile(Profile):
    """Specialized Profile subclass for Firefox"""
    preferences = {
                   'app.update.enabled' : False,
                   
                   'browser.sessionstore.resume_from_crash': False,
                   
                   'browser.shell.checkDefaultBrowser' : False,
                   
                   'browser.tabs.warnOnClose' : False,
                   
                   'browser.warnOnQuit': False,
                   
                   
                   
                   'extensions.enabledScopes' : 5,
                   'extensions.autoDisableScopes' : 10,
                   
                   'extensions.installDistroAddons' : False,
                   
                   'extensions.showMismatchUI' : False,
                   
                   'extensions.update.enabled'    : False,
                   
                   'extensions.update.notifyUser' : False,
                   }

class ThunderbirdProfile(Profile):
    preferences = {'extensions.update.enabled'    : False,
                   'extensions.update.notifyUser' : False,
                   'browser.shell.checkDefaultBrowser' : False,
                   'browser.tabs.warnOnClose' : False,
                   'browser.warnOnQuit': False,
                   'browser.sessionstore.resume_from_crash': False,
                   
                   'mail.provider.enabled': False,
                   }
