










































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var gPrivacyPane = {

  


  _autoStartPrivateBrowsing: false,

  



  init: function ()
  {
    this._updateHistoryDaysUI();
    this._updateSanitizeSettingsButton();
    this.initializeHistoryMode();
    this.updateHistoryModePane();
    this.updatePrivacyMicroControls();
    this.initAutoStartPrivateBrowsingObserver();
  },

  

  










  prefsForDefault: [
    "browser.history_expire_days",
    "browser.history_expire_days_min",
    "browser.download.manager.retention",
    "browser.formfill.enable",
    "network.cookie.cookieBehavior",
    "network.cookie.lifetimePolicy",
    "privacy.sanitize.sanitizeOnShutdown"
  ],

  






  dependentControls: [
    "rememberHistoryDays",
    "rememberAfter",
    "rememberDownloads",
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
      pref.value = false;

      
      let rememberHistoryCheckbox = document.getElementById("rememberHistoryDays");
      if (!rememberHistoryCheckbox.checked) {
        rememberHistoryCheckbox.checked = true;
        this.onchangeHistoryDaysCheck();
      }

      
      if (!document.getElementById("rememberDownloads").checked)
        document.getElementById("browser.download.manager.retention").value = 2;

      
      document.getElementById("browser.formfill.enable").value = true;

      
      document.getElementById("network.cookie.cookieBehavior").value = 0;
      
      document.getElementById("network.cookie.lifetimePolicy").value = 0;

      
      document.getElementById("privacy.sanitize.sanitizeOnShutdown").value = false;
      break;
    case "dontremember":
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

      
      document.getElementById("rememberHistoryDays").checked = disabled ? false :
        document.getElementById("browser.history_expire_days").value > 0;
      this.onchangeHistoryDaysCheck();
      document.getElementById("rememberDownloads").checked = disabled ? false :
        this.readDownloadRetention();
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
                              .QueryInterface(Components.interfaces.nsIPrefBranch2);
    prefService.addObserver("browser.privatebrowsing.autostart",
                            this.autoStartPrivateBrowsingObserver,
                            true);
  },

  autoStartPrivateBrowsingObserver:
  {
    QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIObserver,
                                           Components.interfaces.nsISupportsWeakReference]),

    observe: function PPP_observe(aSubject, aTopic, aData)
    {
      let privateBrowsingService = Components.classes["@mozilla.org/privatebrowsing;1"].
        getService(Components.interfaces.nsIPrivateBrowsingService);

      
      let prefValue = document.getElementById("browser.privatebrowsing.autostart").value;
      let keepCurrentSession = document.getElementById("browser.privatebrowsing.keep_current_session");
      keepCurrentSession.value = true;
      
      
      if (prefValue && privateBrowsingService.privateBrowsingEnabled)
        privateBrowsingService.privateBrowsingEnabled = false;
      privateBrowsingService.privateBrowsingEnabled = prefValue;
      keepCurrentSession.reset();
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

  




























  




  _updateHistoryDaysUI: function ()
  {
    var pref = document.getElementById("browser.history_expire_days");
    var mirror = document.getElementById("browser.history_expire_days.mirror");
    var pref_min = document.getElementById("browser.history_expire_days_min");
    var textbox = document.getElementById("historyDays");
    var checkbox = document.getElementById("rememberHistoryDays");

    
    if (mirror.value === null || mirror.value != pref.value || 
        (mirror.value == pref.value && mirror.value == 0) )
      mirror.value = pref.value ? pref.value : pref.defaultValue;

    checkbox.checked = (pref.value > 0);
    textbox.disabled = !checkbox.checked;
  },

  




  onchangeHistoryDaysCheck: function ()
  {
    var pref = document.getElementById("browser.history_expire_days");
    var mirror = document.getElementById("browser.history_expire_days.mirror");
    var textbox = document.getElementById("historyDays");
    var checkbox = document.getElementById("rememberHistoryDays");

    if (!this._autoStartPrivateBrowsing)
      pref.value = checkbox.checked ? mirror.value : 0;
    textbox.disabled = !checkbox.checked;
  },

  




  onkeyupHistoryDaysText: function ()
  {
    var textbox = document.getElementById("historyDays");
    var checkbox = document.getElementById("rememberHistoryDays");
    
    checkbox.checked = textbox.value != 0;
  },

  




  readDownloadRetention: function ()
  {
    var pref = document.getElementById("browser.download.manager.retention");
    return (pref.value == 2);
  },

  



  writeDownloadRetention: function ()
  {
    var checkbox = document.getElementById("rememberDownloads");
    return checkbox.checked ? 2 : 0;
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
