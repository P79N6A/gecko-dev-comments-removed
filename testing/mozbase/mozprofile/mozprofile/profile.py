



__all__ = ['Profile',
           'FirefoxProfile',
           'MetroFirefoxProfile',
           'ThunderbirdProfile']

import json
import os
import time
import tempfile
import types
import uuid

from addons import AddonManager
from mozfile import tree
from permissions import Permissions
from prefs import Preferences
from shutil import copytree, rmtree
from webapps import WebappCollection


class Profile(object):
    """Handles all operations regarding profile. Created new profiles, installs extensions,
    sets preferences and handles cleanup.

    :param profile: Path to the profile
    :param addons: String of one or list of addons to install
    :param addon_manifests: Manifest for addons, see http://ahal.ca/blog/2011/bulk-installing-fx-addons/
    :param apps: Dictionary or class of webapps to install
    :param preferences: Dictionary or class of preferences
    :param locations: ServerLocations object
    :param proxy: setup a proxy
    :param restore: If true remove all added addons and preferences when cleaning up
    """

    def __init__(self, profile=None, addons=None, addon_manifests=None, apps=None,
                 preferences=None, locations=None, proxy=None, restore=True):

        
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

        
        self.addon_manager = AddonManager(self.profile, restore=self.restore)
        self.addon_manager.install_addons(addons, addon_manifests)

        
        self.webapps = WebappCollection(profile=self.profile, apps=apps)
        self.webapps.update_manifests()

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

    @classmethod
    def clone(cls, path_from, path_to=None, **kwargs):
        """Instantiate a temporary profile via cloning
        - path: path of the basis to clone
        - kwargs: arguments to the profile constructor
        """
        if not path_to:
            tempdir = tempfile.mkdtemp() 
            rmtree(tempdir) 
            path_to = tempdir
        copytree(path_from, path_to)

        def cleanup_clone(fn):
            """Deletes a cloned profile when restore is True"""
            def wrapped(self):
                fn(self)
                if self.restore and os.path.exists(self.profile):
                        rmtree(self.profile, onerror=self._cleanup_error)
            return wrapped

        c = cls(path_to, **kwargs)
        c.__del__ = c.cleanup = types.MethodType(cleanup_clone(cls.cleanup), c)
        return c

    def create_new_profile(self):
        """Create a new clean temporary profile which is a simple empty folder"""
        return tempfile.mkdtemp(suffix='.mozrunner')


    

    def set_preferences(self, preferences, filename='user.js'):
        """Adds preferences dict to profile preferences"""

        
        prefs_file = os.path.join(self.profile, filename)
        f = open(prefs_file, 'a')

        if preferences:

            
            self.written_prefs.add(filename)

            
            f.write('\n%s\n' % self.delimeters[0])

            
            Preferences.write(f, preferences)

            
            f.write('%s\n' % self.delimeters[1])

        f.close()

    def pop_preferences(self, filename):
        """
        pop the last set of preferences added
        returns True if popped
        """

        path = os.path.join(self.profile, filename)
        with file(path) as f:
            lines = f.read().splitlines()
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
        with file(path, 'w') as f:
            f.write(cleaned_prefs)
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
                self.webapps.clean()

    __del__ = cleanup

    

    def summary(self, return_parts=False):
        """
        returns string summarizing profile information.
        if return_parts is true, return the (Part_name, value) list
        of tuples instead of the assembled string
        """

        parts = [('Path', self.profile)] 

        
        parts.append(('Files', '\n%s' % tree(self.profile)))

        
        for prefs_file in ('user.js', 'prefs.js'):
            path = os.path.join(self.profile, prefs_file)
            if os.path.exists(path):

                
                
                
                section_prefs = ['network.proxy.autoconfig_url']
                line_length = 80
                line_length_buffer = 10 
                line_length_buffer += len(': ')
                def format_value(key, value):
                    if key not in section_prefs:
                        return value
                    max_length = line_length - len(key) - line_length_buffer
                    if len(value) > max_length:
                        value = '%s...' % value[:max_length]
                    return value

                prefs = Preferences.read_prefs(path)
                if prefs:
                    prefs = dict(prefs)
                    parts.append((prefs_file,
                    '\n%s' %('\n'.join(['%s: %s' % (key, format_value(key, prefs[key]))
                                        for key in sorted(prefs.keys())
                                        ]))))

                    
                    
                    
                    network_proxy_autoconfig = prefs.get('network.proxy.autoconfig_url')
                    if network_proxy_autoconfig and network_proxy_autoconfig.strip():
                        network_proxy_autoconfig = network_proxy_autoconfig.strip()
                        lines = network_proxy_autoconfig.replace(';', ';\n').splitlines()
                        lines = [line.strip() for line in lines]
                        origins_string = 'var origins = ['
                        origins_end = '];'
                        if origins_string in lines[0]:
                            start = lines[0].find(origins_string)
                            end = lines[0].find(origins_end, start);
                            splitline = [lines[0][:start],
                                         lines[0][start:start+len(origins_string)-1],
                                         ]
                            splitline.extend(lines[0][start+len(origins_string):end].replace(',', ',\n').splitlines())
                            splitline.append(lines[0][end:])
                            lines[0:1] = [i.strip() for i in splitline]
                        parts.append(('Network Proxy Autoconfig, %s' % (prefs_file),
                                      '\n%s' % '\n'.join(lines)))

        if return_parts:
            return parts

        retval = '%s\n' % ('\n\n'.join(['[%s]: %s' % (key, value)
                                        for key, value in parts]))
        return retval

    __str__ = summary


class FirefoxProfile(Profile):
    """Specialized Profile subclass for Firefox"""

    preferences = {
                   'app.update.enabled' : False,
                   
                   'browser.sessionstore.resume_from_crash': False,
                   
                   'browser.shell.checkDefaultBrowser' : False,
                   
                   'browser.tabs.warnOnClose' : False,
                   
                   'browser.warnOnQuit': False,
                   
                   'datareporting.healthreport.documentServerURI' : 'http://%(server)s/healthreport/',
                   
                   
                   
                   'extensions.enabledScopes' : 5,
                   'extensions.autoDisableScopes' : 10,
                   
                   'extensions.installDistroAddons' : False,
                   
                   'extensions.showMismatchUI' : False,
                   
                   'extensions.update.enabled'    : False,
                   
                   'extensions.update.notifyUser' : False,
                   
                   'focusmanager.testmode' : True,
                   
                   'geo.provider.testing' : True,
                   
                   'security.notification_enable_delay' : 0,
                   
                   'toolkit.startup.max_resumed_crashes' : -1,
                   
                   'toolkit.telemetry.enabled' : False,
                   'toolkit.telemetry.enabledPreRelease' : False,
                   }

class MetroFirefoxProfile(Profile):
    """Specialized Profile subclass for Firefox Metro"""

    preferences = {
                   'app.update.enabled' : False,
                   'app.update.metro.enabled' : False,
                   
                   'browser.sessionstore.resume_from_crash': False,
                   
                   'browser.shell.checkDefaultBrowser' : False,
                   
                   'datareporting.healthreport.documentServerURI' : 'http://%(server)s/healthreport/',
                   
                   
                   
                   'extensions.enabledScopes' : 5,
                   'extensions.autoDisableScopes' : 10,
                   
                   'extensions.installDistroAddons' : False,
                   
                   'extensions.showMismatchUI' : False,
                   
                   'extensions.strictCompatibility' : False,
                   
                   'extensions.update.enabled'    : False,
                   
                   'extensions.update.notifyUser' : False,
                   
                   'focusmanager.testmode' : True,
                   
                   'security.notification_enable_delay' : 0,
                   
                   'toolkit.startup.max_resumed_crashes' : -1,
                   
                   'toolkit.telemetry.enabled' : False,
                   'toolkit.telemetry.enabledPreRelease' : False,
                   }

class ThunderbirdProfile(Profile):
    """Specialized Profile subclass for Thunderbird"""

    preferences = {'extensions.update.enabled'    : False,
                   'extensions.update.notifyUser' : False,
                   'browser.shell.checkDefaultBrowser' : False,
                   'browser.tabs.warnOnClose' : False,
                   'browser.warnOnQuit': False,
                   'browser.sessionstore.resume_from_crash': False,
                   
                   'mail.provider.enabled': False,
                   }
