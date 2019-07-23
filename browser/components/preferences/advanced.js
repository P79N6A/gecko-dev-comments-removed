# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Firefox Preferences System.
#
# The Initial Developer of the Original Code is
# Ben Goodger.
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <ben@mozilla.org>
#   Jeff Walden <jwalden+code@mit.edu>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

var gAdvancedPane = {
  _inited: false,

  


  init: function ()
  {
    this._inited = true;
    var advancedPrefs = document.getElementById("advancedPrefs");
    var preference = document.getElementById("browser.preferences.advanced.selectedTabIndex");
    if (preference.value === null)
      return;
    advancedPrefs.selectedIndex = preference.value;
    
    this.updateAppUpdateItems();
    this.updateAutoItems();
    this.updateModeItems();
  },

  



  tabSelectionChanged: function ()
  {
    if (!this._inited)
      return;
    var advancedPrefs = document.getElementById("advancedPrefs");
    var preference = document.getElementById("browser.preferences.advanced.selectedTabIndex");
    preference.valueFromPreferences = advancedPrefs.selectedIndex;
  },
  
  

  

























  



  _storedSpellCheck: 0,

  




  readCheckSpelling: function ()
  {
    var pref = document.getElementById("layout.spellcheckDefault");
    this._storedSpellCheck = pref.value;

    return (pref.value != 0);
  },

  




  writeCheckSpelling: function ()
  {
    var checkbox = document.getElementById("checkSpelling");
    return checkbox.checked ? (this._storedSpellCheck == 2 ? 2 : 1) : 0;
  },

  


  showLanguages: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/languages.xul",
                                           "", null);
  },

  

  






  


  showConnections: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/connection.xul",
                                           "", null);
  },

  



  readCacheSize: function ()
  {
    var preference = document.getElementById("browser.cache.disk.capacity");
    return preference.value / 1000;
  },

  



  writeCacheSize: function ()
  {
    var cacheSize = document.getElementById("cacheSize");
    var intValue = parseInt(cacheSize.value, 10);
    return isNaN(intValue) ? 0 : intValue * 1000;
  },

  


  clearCache: function ()
  {
    var cacheService = Components.classes["@mozilla.org/network/cache-service;1"]
                         	       .getService(Components.interfaces.nsICacheService);
    try {
      cacheService.evictEntries(Components.interfaces.nsICache.STORE_ANYWHERE);
    } catch(ex) {}
  },

  

  





















  






























  updateAppUpdateItems: function () 
  {
    var aus = 
        Components.classes["@mozilla.org/updates/update-service;1"].
        getService(Components.interfaces.nsIApplicationUpdateService);

    var enabledPref = document.getElementById("app.update.enabled");
    var enableAppUpdate = document.getElementById("enableAppUpdate");

    enableAppUpdate.disabled = !aus.canUpdate || enabledPref.locked;
  },

  



  updateAutoItems: function () 
  {
    var enabledPref = document.getElementById("app.update.enabled");
    var autoPref = document.getElementById("app.update.auto");
    
    var updateModeLabel = document.getElementById("updateModeLabel");
    var updateMode = document.getElementById("updateMode");
    
    var disable = enabledPref.locked || !enabledPref.value ||
                  autoPref.locked;
    updateModeLabel.disabled = updateMode.disabled = disable;
  },

  



  updateModeItems: function () 
  {
    var enabledPref = document.getElementById("app.update.enabled");
    var autoPref = document.getElementById("app.update.auto");
    var modePref = document.getElementById("app.update.mode");
    
    var warnIncompatible = document.getElementById("warnIncompatible");
    
    var disable = enabledPref.locked || !enabledPref.value || autoPref.locked ||
                  !autoPref.value || modePref.locked;
    warnIncompatible.disabled = disable;
  },

  



  updateAddonUpdateUI: function ()
  {
    var enabledPref = document.getElementById("extensions.update.enabled");
    var enableAddonUpdate = document.getElementById("enableAddonUpdate");

    enableAddonUpdate.disabled = enabledPref.locked;
  },  

  






  _modePreference: -1,

  












  readAddonWarn: function ()
  {
    var preference = document.getElementById("app.update.mode");
    var doNotWarn = preference.value != 0;
    gAdvancedPane._modePreference = doNotWarn ? preference.value : 1;
    return doNotWarn;
  },

  




  writeAddonWarn: function ()
  {
    var warnIncompatible = document.getElementById("warnIncompatible");
    return !warnIncompatible.checked ? 0 : gAdvancedPane._modePreference;
  },

  


  showUpdates: function ()
  {
    var prompter = Components.classes["@mozilla.org/updates/update-prompt;1"]
                             .createInstance(Components.interfaces.nsIUpdatePrompt);
    prompter.showUpdateHistory(window);
  },
  
  

  















  


  showCertificates: function ()
  {
    document.documentElement.openWindow("mozilla:certmanager",
                                        "chrome://pippki/content/certManager.xul",
                                        "", null);
  },

  


  showCRLs: function ()
  {
    document.documentElement.openWindow("Mozilla:CRLManager", 
                                        "chrome://pippki/content/crlManager.xul",
                                        "", null);
  },

  


  showOCSP: function ()
  {
    document.documentElement.openSubDialog("chrome://mozapps/content/preferences/ocsp.xul",
                                           "", null);
  },

  


  showSecurityDevices: function ()
  {
    document.documentElement.openWindow("mozilla:devicemanager",
                                        "chrome://pippki/content/device_manager.xul",
                                        "", null);
  }
};
