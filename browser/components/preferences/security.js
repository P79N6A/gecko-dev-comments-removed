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
# Jeff Walden <jwalden+code@mit.edu>.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ryan Flint <rflint@dslr.net>
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

var gSecurityPane = {
  _pane: null,

  


  init: function ()
  {
    this._pane = document.getElementById("paneSecurity");
    this._initMasterPasswordUI();
  },

  

  








  



  readWarnAddonInstall: function ()
  {
    var warn = document.getElementById("xpinstall.whitelist.required");
    var exceptions = document.getElementById("addonExceptions");

    exceptions.disabled = !warn.value;

    
    return undefined;
  },

  


  showAddonExceptions: function ()
  {
    var bundlePrefs = document.getElementById("bundlePreferences");

    var params = this._addonParams;
    if (!params.windowTitle || !params.introText) {
      params.windowTitle = bundlePrefs.getString("addons_permissions_title");
      params.introText = bundlePrefs.getString("addonspermissionstext");
    }

    document.documentElement.openWindow("Browser:Permissions",
                                        "chrome://browser/content/preferences/permissions.xul",
                                        "", params);
  },

  


  _addonParams:
    {
      blockVisible: false,
      sessionVisible: false,
      allowVisible: true,
      prefilledHost: "",
      permissionType: "install"
    },

  

  
















  



  readCheckPhish: function ()
  {
    var phishEnabled = document.getElementById("browser.safebrowsing.enabled").value;

    var checkPhish = document.getElementById("checkPhishChoice");
    var loadList = document.getElementById("onloadProvider");
    var onloadAfter = document.getElementById("onloadAfter");

    checkPhish.disabled = onloadAfter.disabled = !phishEnabled;
    loadList.disabled = !phishEnabled;

    
    return undefined;
  },

  










  _userAgreedToPhishingEULA: function (providerNum)
  {
    
    const prefName = "browser.safebrowsing.provider." +
                     providerNum +
                     ".privacy.optedIn";
    var pref = document.getElementById(prefName);

    if (!pref) {
      pref = document.createElement("preference");
      pref.setAttribute("type", "bool");
      pref.id = prefName;
      pref.setAttribute("name", prefName);
      document.getElementById("securityPreferences").appendChild(pref);
    }

    
    
    if (!pref.value) {
      var rv = { userAgreed: false, providerNum: providerNum };
      document.documentElement.openSubDialog("chrome://browser/content/preferences/phishEULA.xul",
                                             "resizable", rv);

      
      if (rv.userAgreed)
        pref.value = true;

      return rv.userAgreed;
    }

    
    return true;
  },

  




  writePhishChoice: function ()
  {
    var radio = document.getElementById("checkPhishChoice");
    var provider = document.getElementById("browser.safebrowsing.dataProvider");

    
    if (radio.value == "true" &&
        !this._userAgreedToPhishingEULA(provider.value)) {
      radio.value = "false";
      return false;
    }

    
    return undefined;
  },

  



  onSBChange: function ()
  {
    var phishEnabled = document.getElementById("browser.safebrowsing.enabled").value;
    var remoteLookup = document.getElementById("browser.safebrowsing.remoteLookups");
    var providerNum = document.getElementById("onloadProvider").value;

    if (phishEnabled && remoteLookup.value &&
        !this._userAgreedToPhishingEULA(providerNum))
      remoteLookup.value = false;
  },

  



  readOnloadPhishProvider: function ()
  {
    const Cc = Components.classes, Ci = Components.interfaces;
    const onloadPopupId = "onloadPhishPopup";
    var popup = document.getElementById(onloadPopupId);

    if (!popup) {
      var providerBranch = Cc["@mozilla.org/preferences-service;1"]
                             .getService(Ci.nsIPrefService)
                             .getBranch("browser.safebrowsing.provider.");

      
      
      
      var kids = providerBranch.getChildList("", {});
      var providers = [];
      var hasPrivacyPolicy = {};
      for (var i = 0; i < kids.length; i++) {
        var curr = kids[i];
        var matchesName = curr.match(/^(\d+)\.name$/);
        var matchesPolicy = curr.match(/^(\d+)\.privacy\.url$/);

        
        if (!matchesName && !matchesPolicy)
          continue;

        if (matchesName)
          providers.push(matchesName[1]);
        else
          hasPrivacyPolicy[matchesPolicy[1]] = true;
      }

      
      for (var i = 0; i < providers.length; i++) {
        
        if (!(providers[i] in hasPrivacyPolicy))
          continue;

        
        try {
          var providerNum = providers[i];
          var url = providerBranch.getCharPref(providerNum + ".privacy.url");
          var fallbackurl = providerBranch.getCharPref(providerNum +
                                                       ".privacy.fallbackurl");
          var scheme = Cc["@mozilla.org/network/io-service;1"].
                       getService(Ci.nsIIOService).
                       extractScheme(fallbackurl);
          if (scheme != "chrome")
            throw "fallbackurl scheme must be chrome";
        }
        catch (e) {
          
          continue;
        }

        if (!popup) {
          popup = document.createElement("menupopup");
          popup.id = onloadPopupId;
        }

        var providerName = providerBranch.getCharPref(providerNum + ".name");

        var item = document.createElement("menuitem");
        item.setAttribute("value", providerNum);
        item.setAttribute("label", providerName);
        item.setAttribute("oncommand", "gSecurityPane.onProviderChanged();");
        popup.appendChild(item);
      }

      var onloadProviders = document.getElementById("onloadProvider");
      onloadProviders.appendChild(popup);
    }

    
    return undefined;
  },

  



  onProviderChanged: function ()
  {
    var pref = document.getElementById("browser.safebrowsing.dataProvider");
    var remoteLookup = document.getElementById("browser.safebrowsing.remoteLookups");

    remoteLookup.value = this._userAgreedToPhishingEULA(pref.value);
  },

  

  






  



  readSavePasswords: function ()
  {
    var pref = document.getElementById("signon.rememberSignons");
    var excepts = document.getElementById("passwordExceptions");

    excepts.disabled = !pref.value;

    
    return undefined;
  },

  



  showPasswordExceptions: function ()
  {
    document.documentElement.openWindow("Toolkit:PasswordManagerExceptions",
                                        "chrome://passwordmgr/content/passwordManagerExceptions.xul",
                                        "", null);
  },

  





  _initMasterPasswordUI: function ()
  {
    var noMP = !this._masterPasswordSet();

    
    

    var checkbox = document.getElementById("useMasterPassword");
    checkbox.checked = !noMP;
  },

  


  _masterPasswordSet: function ()
  {
    const Cc = Components.classes, Ci = Components.interfaces;
    var secmodDB = Cc["@mozilla.org/security/pkcs11moduledb;1"]
                     .getService(Ci.nsIPKCS11ModuleDB);
    var slot = secmodDB.findSlotByName("");
    if (slot) {
      var status = slot.status;
      var hasMP = status != Ci.nsIPKCS11Slot.SLOT_UNINITIALIZED &&
                  status != Ci.nsIPKCS11Slot.SLOT_READY;
      return hasMP;
    } else {
      
      return false;
    }
  },

  




  updateMasterPasswordButton: function ()
  {
    var checkbox = document.getElementById("useMasterPassword");
    var button = document.getElementById("changeMasterPassword");
    button.disabled = !checkbox.checked;

    
    
    
    
    
    if (!checkbox.checked)
      this._removeMasterPassword();
    else
      this.changeMasterPassword();

    this._initMasterPasswordUI();
  },

  




  _removeMasterPassword: function ()
  {
    var secmodDB = Components.classes["@mozilla.org/security/pkcs11moduledb;1"]
                              .getService(Components.interfaces.nsIPKCS11ModuleDB);
    if (secmodDB.isFIPSEnabled) {
      var bundle = document.getElementById("bundlePreferences");
      promptService.alert(window,
                          bundle.getString("pw_change_failed_title"),
                          bundle.getString("pw_change2empty_in_fips_mode"));
    }
    else {
      document.documentElement.openSubDialog("chrome://mozapps/content/preferences/removemp.xul",
                                             "", null);
    }
    this._initMasterPasswordUI();
  },

  


  changeMasterPassword: function ()
  {
    document.documentElement.openSubDialog("chrome://mozapps/content/preferences/changemp.xul",
                                           "", null);
    this._initMasterPasswordUI();
  },

  



  showPasswords: function ()
  {
    document.documentElement.openWindow("Toolkit:PasswordManager",
                                        "chrome://passwordmgr/content/passwordManager.xul",
                                        "", null);
  },


  

  




  showWarningMessageSettings: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/securityWarnings.xul",
                                           "", null);
  }

};
