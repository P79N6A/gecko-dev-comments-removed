



var gPrivacyPane = {

  


  _autoStartPrivateBrowsing: false,

  


  _shouldPromptForRestart: true,

  



  init: function ()
  {
    function setEventListener(aId, aEventType, aCallback)
    {
      document.getElementById(aId)
              .addEventListener(aEventType, aCallback.bind(gPrivacyPane));
    }

    this._updateSanitizeSettingsButton();
    this.initializeHistoryMode();
    this.updateHistoryModePane();
    this.updatePrivacyMicroControls();
    this.initAutoStartPrivateBrowsingReverter();

    setEventListener("browser.urlbar.default.behavior", "change",
      document.getElementById('browser.urlbar.autocomplete.enabled')
              .updateElements
    );
    setEventListener("privacy.sanitize.sanitizeOnShutdown", "change",
                     gPrivacyPane._updateSanitizeSettingsButton);
    setEventListener("browser.privatebrowsing.autostart", "change",
                     gPrivacyPane.updatePrivacyMicroControls);
    setEventListener("historyMode", "command", function () {
      gPrivacyPane.updateHistoryModePane();
      gPrivacyPane.updateHistoryModePrefs();
      gPrivacyPane.updatePrivacyMicroControls();
      gPrivacyPane.updateAutostart();
    });
    setEventListener("historyRememberClear", "click", function () {
      gPrivacyPane.clearPrivateDataNow(false);
      return false;
    });
    setEventListener("historyRememberCookies", "click", function () {
      gPrivacyPane.showCookies();
      return false;
    });
    setEventListener("historyDontRememberClear", "click", function () {
      gPrivacyPane.clearPrivateDataNow(true);
      return false;
    });
    setEventListener("privateBrowsingAutoStart", "command",
                     gPrivacyPane.updateAutostart);
    setEventListener("cookieExceptions", "command",
                     gPrivacyPane.showCookieExceptions);
    setEventListener("showCookiesButton", "command",
                     gPrivacyPane.showCookies);
    setEventListener("clearDataSettings", "command",
                     gPrivacyPane.showClearPrivateDataSettings);
  },

  

  










  prefsForDefault: [
    "places.history.enabled",
    "browser.formfill.enable",
    "network.cookie.cookieBehavior",
    "network.cookie.lifetimePolicy",
    "privacy.sanitize.sanitizeOnShutdown"
  ],

  






  dependentControls: [
    "rememberHistory",
    "rememberForms",
    "keepUntil",
    "keepCookiesUntil",
    "alwaysClear",
    "clearDataSettings"
  ],

  






  _checkDefaultValues: function(aPrefs) {
    for (let i = 0; i < aPrefs.length; ++i) {
      let pref = document.getElementById(aPrefs[i]);
      if (pref.value != pref.defaultValue)
        return false;
    }
    return true;
  },

  


  initializeHistoryMode: function PPP_initializeHistoryMode()
  {
    let mode;
    let getVal = function (aPref)
      document.getElementById(aPref).value;

    if (this._checkDefaultValues(this.prefsForDefault)) {
      if (getVal("browser.privatebrowsing.autostart"))
        mode = "dontremember";
      else
        mode = "remember";
    }
    else
      mode = "custom";

    document.getElementById("historyMode").value = mode;
  },

  


  updateHistoryModePane: function PPP_updateHistoryModePane()
  {
    let selectedIndex = -1;
    switch (document.getElementById("historyMode").value) {
    case "remember":
      selectedIndex = 0;
      break;
    case "dontremember":
      selectedIndex = 1;
      break;
    case "custom":
      selectedIndex = 2;
      break;
    }
    document.getElementById("historyPane").selectedIndex = selectedIndex;
  },

  


  setTrackingPrefs: function PPP_setTrackingPrefs()
  {
    let dntRadioGroup = document.getElementById("doNotTrackSelection"),
        dntValuePref = document.getElementById("privacy.donottrackheader.value"),
        dntEnabledPref = document.getElementById("privacy.donottrackheader.enabled");

    
    
    if (dntRadioGroup.selectedItem.value == -1) {
      dntEnabledPref.value = false;
      return dntValuePref.value;
    }

    dntEnabledPref.value = true;
    return dntRadioGroup.selectedItem.value;
  },

  


  getTrackingPrefs: function PPP_getTrackingPrefs()
  {
    
    let dntValue = Services.prefs.getIntPref("privacy.donottrackheader.value"),
        dntEnabled = Services.prefs.getBoolPref("privacy.donottrackheader.enabled");

    
    
    if (dntEnabled)
      return dntValue;

    return document.getElementById("dntnopref").value;
  },

  



  updateHistoryModePrefs: function PPP_updateHistoryModePrefs()
  {
    let pref = document.getElementById("browser.privatebrowsing.autostart");
    switch (document.getElementById("historyMode").value) {
    case "remember":
      if (pref.value)
        pref.value = false;

      
      let rememberHistoryCheckbox = document.getElementById("rememberHistory");
      if (!rememberHistoryCheckbox.checked)
        rememberHistoryCheckbox.checked = true;

      
      document.getElementById("browser.formfill.enable").value = true;

#ifdef RELEASE_BUILD
      
      document.getElementById("network.cookie.cookieBehavior").value = 0;
#else
      
      document.getElementById("network.cookie.cookieBehavior").value = 3;
#endif
      
      document.getElementById("network.cookie.lifetimePolicy").value = 0;

      
      document.getElementById("privacy.sanitize.sanitizeOnShutdown").value = false;
      break;
    case "dontremember":
      if (!pref.value)
        pref.value = true;
      break;
    }
  },

  



  updatePrivacyMicroControls: function PPP_updatePrivacyMicroControls()
  {
    if (document.getElementById("historyMode").value == "custom") {
      let disabled = this._autoStartPrivateBrowsing =
        document.getElementById("privateBrowsingAutoStart").checked;
      this.dependentControls
          .forEach(function (aElement)
                   document.getElementById(aElement).disabled = disabled);

      
      this.readAcceptCookies();
      document.getElementById("keepCookiesUntil").value = disabled ? 2 :
        document.getElementById("network.cookie.lifetimePolicy").value;

      
      document.getElementById("alwaysClear").checked = disabled ? false :
        document.getElementById("privacy.sanitize.sanitizeOnShutdown").value;

      
      document.getElementById("rememberHistory").checked = disabled ? false :
        document.getElementById("places.history.enabled").value;
      document.getElementById("rememberForms").checked = disabled ? false :
        document.getElementById("browser.formfill.enable").value;

      if (!disabled) {
        
        this._updateSanitizeSettingsButton();
      }
    }
  },

  

  


  initAutoStartPrivateBrowsingReverter: function PPP_initAutoStartPrivateBrowsingReverter()
  {
    let mode = document.getElementById("historyMode");
    let autoStart = document.getElementById("privateBrowsingAutoStart");
    this._lastMode = mode.selectedIndex;
    this._lastCheckState = autoStart.hasAttribute('checked');
  },

  _lastMode: null,
  _lastCheckState: null,
  updateAutostart: function PPP_updateAutostart() {
      let mode = document.getElementById("historyMode");
      let autoStart = document.getElementById("privateBrowsingAutoStart");
      let pref = document.getElementById("browser.privatebrowsing.autostart");
      if ((mode.value == "custom" && this._lastCheckState == autoStart.checked) ||
          (mode.value == "remember" && !this._lastCheckState) ||
          (mode.value == "dontremember" && this._lastCheckState)) {
          
          this._lastMode = mode.selectedIndex;
          this._lastCheckState = autoStart.hasAttribute('checked');
          return;
      }

      if (!this._shouldPromptForRestart) {
        
        return;
      }

      const Cc = Components.classes, Ci = Components.interfaces;
      let brandName = document.getElementById("bundleBrand").getString("brandShortName");
      let bundle = document.getElementById("bundlePreferences");
      let msg = bundle.getFormattedString(autoStart.checked ?
                                          "featureEnableRequiresRestart" : "featureDisableRequiresRestart",
                                          [brandName]);
      let title = bundle.getFormattedString("shouldRestartTitle", [brandName]);
      let prompts = Cc["@mozilla.org/embedcomp/prompt-service;1"].getService(Ci.nsIPromptService);
      let shouldProceed = prompts.confirm(window, title, msg)
      if (shouldProceed) {
        let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"]
                           .createInstance(Ci.nsISupportsPRBool);
        Services.obs.notifyObservers(cancelQuit, "quit-application-requested",
                                     "restart");
        shouldProceed = !cancelQuit.data;

        if (shouldProceed) {
          pref.value = autoStart.hasAttribute('checked');
          let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                             .getService(Ci.nsIAppStartup);
          appStartup.quit(Ci.nsIAppStartup.eAttemptQuit |  Ci.nsIAppStartup.eRestart);
          return;
        }
      }

      this._shouldPromptForRestart = false;

      if (this._lastCheckState) {
        autoStart.checked = "checked";
      } else {
        autoStart.removeAttribute('checked');
      }
      pref.value = autoStart.hasAttribute('checked');
      mode.selectedIndex = this._lastMode;
      mode.doCommand();

      this._shouldPromptForRestart = true;
  },

  

  



  readSuggestionPref: function PPP_readSuggestionPref()
  {
    let getVal = function(aPref)
      document.getElementById("browser.urlbar." + aPref).value;

    
    if (!getVal("autocomplete.enabled"))
      return -1;

    
    return getVal("default.behavior") & 3;
  },

  



  writeSuggestionPref: function PPP_writeSuggestionPref()
  {
    let menuVal = document.getElementById("locationBarSuggestion").value;
    let enabled = menuVal != -1;

    
    if (enabled) {
      
      let behavior = document.getElementById("browser.urlbar.default.behavior");
      behavior.value = behavior.value >> 2 << 2 | menuVal;
    }

    
    return enabled;
  },

  









  

  
















  




  readAcceptCookies: function ()
  {
    var pref = document.getElementById("network.cookie.cookieBehavior");
    var acceptThirdPartyLabel = document.getElementById("acceptThirdPartyLabel");
    var acceptThirdPartyMenu = document.getElementById("acceptThirdPartyMenu");
    var keepUntil = document.getElementById("keepUntil");
    var menu = document.getElementById("keepCookiesUntil");

    
    var acceptCookies = (pref.value != 2);

    acceptThirdPartyLabel.disabled = acceptThirdPartyMenu.disabled = !acceptCookies;
    keepUntil.disabled = menu.disabled = this._autoStartPrivateBrowsing || !acceptCookies;
    
    return acceptCookies;
  },

  



  writeAcceptCookies: function ()
  {
    var accept = document.getElementById("acceptCookies");
    var acceptThirdPartyMenu = document.getElementById("acceptThirdPartyMenu");

#ifdef RELEASE_BUILD
    
    if (accept.checked)
      acceptThirdPartyMenu.selectedIndex = 0;

    return accept.checked ? 0 : 2;
#else
    
    if (accept.checked)
      acceptThirdPartyMenu.selectedIndex = 1;

    return accept.checked ? 3 : 2;
#endif
  },
  
  


  readAcceptThirdPartyCookies: function ()
  {
    var pref = document.getElementById("network.cookie.cookieBehavior");
    switch (pref.value)
    {
      case 0:
        return "always";
      case 1:
        return "never";
      case 2:
        return "never";
      case 3:
        return "visited";
      default:
        return undefined;
    }
  },
  
  writeAcceptThirdPartyCookies: function ()
  {
    var accept = document.getElementById("acceptThirdPartyMenu").selectedItem;
    switch (accept.value)
    {
      case "always":
        return 0;
      case "visited":
        return 3;
      case "never":
        return 1;
      default:
        return undefined;
    }
  },

  


  showCookieExceptions: function ()
  {
    var bundlePreferences = document.getElementById("bundlePreferences");
    var params = { blockVisible   : true,
                   sessionVisible : true,
                   allowVisible   : true,
                   prefilledHost  : "",
                   permissionType : "cookie",
                   windowTitle    : bundlePreferences.getString("cookiepermissionstitle"),
                   introText      : bundlePreferences.getString("cookiepermissionstext") };
    gSubDialog.open("chrome://browser/content/preferences/permissions.xul",
                    null, params);
  },

  


  showCookies: function (aCategory)
  {
    gSubDialog.open("chrome://browser/content/preferences/cookies.xul");
  },

  

  







  


  showClearPrivateDataSettings: function ()
  {
    gSubDialog.open("chrome://browser/content/preferences/sanitize.xul");
  },


  



  clearPrivateDataNow: function (aClearEverything)
  {
    var ts = document.getElementById("privacy.sanitize.timeSpan");
    var timeSpanOrig = ts.value;
    if (aClearEverything)
      ts.value = 0;

    const Cc = Components.classes, Ci = Components.interfaces;
    var glue = Cc["@mozilla.org/browser/browserglue;1"]
                 .getService(Ci.nsIBrowserGlue);
    glue.sanitize(window);

    
    if (aClearEverything)
      ts.value = timeSpanOrig;
    Services.obs.notifyObservers(null, "clear-private-data", null);
  },

  



  _updateSanitizeSettingsButton: function () {
    var settingsButton = document.getElementById("clearDataSettings");
    var sanitizeOnShutdownPref = document.getElementById("privacy.sanitize.sanitizeOnShutdown");

    settingsButton.disabled = !sanitizeOnShutdownPref.value;
   }

};
