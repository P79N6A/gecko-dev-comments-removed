




Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var gPrivacyPane = {

  


  _autoStartPrivateBrowsing: false,

  


  _shouldPromptForRestart: true,

  



  init: function ()
  {
    this._updateSanitizeSettingsButton();
    this.initializeHistoryMode();
    this.updateHistoryModePane();
    this.updatePrivacyMicroControls();
    this.initAutoStartPrivateBrowsingObserver();

    window.addEventListener("unload", this.removeASPBObserver.bind(this), false);
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

      
      document.getElementById("network.cookie.cookieBehavior").value = 0;
      
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

  

  


  initAutoStartPrivateBrowsingObserver: function PPP_initAutoStartPrivateBrowsingObserver()
  {
    let prefService = document.getElementById("privacyPreferences")
                              .service
                              .QueryInterface(Components.interfaces.nsIPrefBranch);
    prefService.addObserver("browser.privatebrowsing.autostart",
                            this.autoStartPrivateBrowsingObserver,
                            false);
  },

  


  removeASPBObserver: function PPP_removeASPBObserver()
  {
    let prefService = document.getElementById("privacyPreferences")
                              .service
                              .QueryInterface(Components.interfaces.nsIPrefBranch);
    prefService.removeObserver("browser.privatebrowsing.autostart",
                               this.autoStartPrivateBrowsingObserver);
  },

  autoStartPrivateBrowsingObserver:
  {
    QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIObserver]),

    observe: function PPP_observe(aSubject, aTopic, aData)
    {
#ifdef MOZ_PER_WINDOW_PRIVATE_BROWSING
      if (!gPrivacyPane._shouldPromptForRestart) {
        
        gPrivacyPane._shouldPromptForRestart = true;
        return;
      }

      const Cc = Components.classes, Ci = Components.interfaces;
      let pref = document.getElementById("browser.privatebrowsing.autostart");
      let brandName = document.getElementById("bundleBrand").getString("brandShortName");
      let bundle = document.getElementById("bundlePreferences");
      let msg = bundle.getFormattedString(pref.value ?
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
          let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                             .getService(Ci.nsIAppStartup);
          appStartup.quit(Ci.nsIAppStartup.eAttemptQuit |  Ci.nsIAppStartup.eRestart);
          return;
        }
      }
      gPrivacyPane._shouldPromptForRestart = false;
      pref.value = !pref.value;

      let mode = document.getElementById("historyMode");
      if (mode.value != "custom") {
        mode.selectedIndex = pref.value ? 1 : 0;
        mode.doCommand();
      } else {
        let rememberHistoryCheckbox = document.getElementById("rememberHistory");
        rememberHistory.checked = pref.value;
      }
#else
      
      let prefValue = document.getElementById("browser.privatebrowsing.autostart").value;
      let keepCurrentSession = document.getElementById("browser.privatebrowsing.keep_current_session");
      keepCurrentSession.value = true;

      let privateBrowsingService = Components.classes["@mozilla.org/privatebrowsing;1"].
        getService(Components.interfaces.nsIPrivateBrowsingService);

      
      
      if (prefValue && privateBrowsingService.privateBrowsingEnabled)
        privateBrowsingService.privateBrowsingEnabled = false;
      privateBrowsingService.privateBrowsingEnabled = prefValue;

      keepCurrentSession.reset();
#endif
    }
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
    var acceptThirdParty = document.getElementById("acceptThirdParty");
    var keepUntil = document.getElementById("keepUntil");
    var menu = document.getElementById("keepCookiesUntil");

    
    var acceptCookies = (pref.value != 2);

    acceptThirdParty.disabled = !acceptCookies;
    keepUntil.disabled = menu.disabled = this._autoStartPrivateBrowsing || !acceptCookies;
    
    return acceptCookies;
  },

  readAcceptThirdPartyCookies: function ()
  {
    var pref = document.getElementById("network.cookie.cookieBehavior");
    return pref.value == 0;
  },

  



  writeAcceptCookies: function ()
  {
    var accept = document.getElementById("acceptCookies");
    var acceptThirdParty = document.getElementById("acceptThirdParty");

    
    if (accept.checked)
      acceptThirdParty.checked = true;

    return accept.checked ? (acceptThirdParty.checked ? 0 : 1) : 2;
  },

  writeAcceptThirdPartyCookies: function ()
  {
    var accept = document.getElementById("acceptCookies");
    var acceptThirdParty = document.getElementById("acceptThirdParty");
    return accept.checked ? (acceptThirdParty.checked ? 0 : 1) : 2;
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
    document.documentElement.openWindow("Browser:Permissions",
                                        "chrome://browser/content/preferences/permissions.xul",
                                        "", params);
  },

  

  
  showCookies: function (aCategory)
  {
    document.documentElement.openWindow("Browser:Cookies",
                                        "chrome://browser/content/preferences/cookies.xul",
                                        "", null);
  },

  

  







  


  showClearPrivateDataSettings: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/sanitize.xul",
                                           "", null);
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
  },

  



  _updateSanitizeSettingsButton: function () {
    var settingsButton = document.getElementById("clearDataSettings");
    var sanitizeOnShutdownPref = document.getElementById("privacy.sanitize.sanitizeOnShutdown");
    
    settingsButton.disabled = !sanitizeOnShutdownPref.value;  	
   }

};
