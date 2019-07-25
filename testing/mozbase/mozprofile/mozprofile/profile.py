



__all__ = ['Profile', 'FirefoxProfile', 'ThunderbirdProfile']

import os
import time
import tempfile
import uuid
from addons import AddonManager
from permissions import Permissions
from shutil import rmtree

try:
    import simplejson
except ImportError:
    import json as simplejson

class Profile(object):
    """Handles all operations regarding profile. Created new profiles, installs extensions,
    sets preferences and handles cleanup."""

    def __init__(self,
                 profile=None, 
                 addons=None,  
                 addon_manifests=None,  
                 preferences=None, 
                 locations=None, 
                 proxy=None, 
                 restore=True 
                 ):

        
        self.restore = restore

        
        self.written_prefs = set()

        
        nonce = '%s %s' % (str(time.time()), uuid.uuid4())
        self.delimeters = ('#MozRunner Prefs Start %s' % nonce,'#MozRunner Prefs End %s' % nonce)

        
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
        self.permissions = Permissions(self.profile, locations)
        prefs_js, user_js = self.permissions.network_prefs(proxy)
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
                      addons=self.addon_manager.installed_addons,
                      addon_manifests=self.addon_manager.installed_manifests,
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

        if preferences:

            
            self.written_prefs.add(filename)


            if isinstance(preferences, dict):
                
                preferences = preferences.items()

            
            f.write('\n%s\n' % self.delimeters[0])
            _prefs = [(simplejson.dumps(k), simplejson.dumps(v) )
                      for k, v in preferences]
            for _pref in _prefs:
                f.write('user_pref(%s, %s);\n' % _pref)
            f.write('%s\n' % self.delimeters[1])
        f.close()

    def pop_preferences(self, filename):
        """
        pop the last set of preferences added
        returns True if popped
        """

        lines = file(os.path.join(self.profile, filename)).read().splitlines()
        def last_index(_list, value):
            """
            returns the last index of an item;
            this should actually be part of python code but it isn't
            """
            for index in reversed(range(len(_list))):
                if _list[index] == value:
                    return index
        s = last_index(lines, self.delimeters[0])
        e = last_index(lines, self.delimeters[1])

        
        if s is None:
            assert e is None, '%s found without %s' % (self.delimeters[1], self.delimeters[0])
            return False 
        elif e is None:
            assert s is None, '%s found without %s' % (self.delimeters[0], self.delimeters[1])

        
        assert e > s, '%s found at %s, while %s found at %s' % (self.delimeters[1], e, self.delimeters[0], s)

        
        cleaned_prefs = '\n'.join(lines[:s] + lines[e+1:])
        f = file(os.path.join(self.profile, 'user.js'), 'w')
        f.write(cleaned_prefs)
        f.close()
        return True

    def clean_preferences(self):
        """Removed preferences added by mozrunner."""
        for filename in self.written_prefs:
            if not os.path.exists(os.path.join(self.profile, filename)):
                
                break
            while True:
                if not self.pop_preferences(filename):
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
        """Cleanup operations for the profile."""
        if self.restore:
            if self.create_new:
                if os.path.exists(self.profile):
                    rmtree(self.profile, onerror=self._cleanup_error)
            else:
                self.clean_preferences()
                self.addon_manager.clean_addons()
                self.permissions.clean_db()

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
                   
                   'toolkit.startup.max_resumed_crashes' : -1,
                   
                   'focusmanager.testmode' : True,
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
