




































var PreferencesView = {
  _currentLocale: null,
  _languages: null,
  _msg: null,
  _restartCount: 0,

  _messageActions: function ev__messageActions(aData) {
    if (aData == "prefs-restart-app") {
      
      var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
      var cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
      os.notifyObservers(cancelQuit, "quit-application-requested", "restart");

      
      if (cancelQuit.data == false) {
        let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
        appStartup.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
      }
    }
  },

  showMessage: function ev_showMessage(aMsg, aValue, aButtonLabel, aShowCloseButton, aNotifyData) {
    let notification = this._msg.getNotificationWithValue(aValue);
    if (notification)
      return;

    let self = this;
    let buttons = null;
    if (aButtonLabel) {
      buttons = [ {
        label: aButtonLabel,
        accessKey: "",
        data: aNotifyData,
        callback: function(aNotification, aButton) {
          self._messageActions(aButton.data);
          return true;
        }
      } ];
    }

    this._msg.appendNotification(aMsg, aValue, "", this._msg.PRIORITY_WARNING_LOW, buttons).hideclose = !aShowCloseButton;
  },

  showRestart: function ev_showRestart() {
    
    this._restartCount++;

    if (this._msg) {
      let strings = Elements.browserBundle;
      this.showMessage(strings.getString("notificationRestart.label"), "restart-app",
                       strings.getString("notificationRestart.button"), false, "prefs-restart-app");
    }
  },

  hideRestart: function ev_hideRestart() {
    this._restartCount--;
    if (this._restartCount == 0 && this._msg) {
      let notification = this._msg.getNotificationWithValue("restart-app");
      if (notification)
        notification.close();
    }
  },

  init: function ev_init() {
    if (this._msg)
      return;

    this._msg = document.getElementById("prefs-messages");

    let self = this;
    let panels = document.getElementById("panel-items");
    panels.addEventListener("select",
                            function(aEvent) {
                              if (panels.selectedPanel.id == "prefs-container")
                                self._delayedInit();
                            },
                            false);
  },

  _delayedInit: function ev__delayedInit() {
    if (this._languages)
      return;

#ifdef WINCE
    let phone = Cc["@mozilla.org/phone/support;1"].getService(Ci.nsIPhoneSupport);
    document.getElementById("prefs-default-browser").value = phone.isDefaultBrowser(false);
#endif

    this._languages = document.getElementById("prefs-languages");
    this._loadLocales();

    this._loadHomePage();
  },

  _loadLocales: function _loadLocales() {
    
    let chrome = Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIXULChromeRegistry);
    chrome.QueryInterface(Ci.nsIToolkitChromeRegistry);
    
    let selectedLocale = chrome.getSelectedLocale("browser");
    let availableLocales = chrome.getLocalesForPackage("browser");
    
    let bundles = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
    let strings = bundles.createBundle("chrome://browser/content/languages.properties");

    
    let selectedItem = null;
    let localeCount = 0;
    while (availableLocales.hasMore()) {
      let locale = availableLocales.getNext();
      try {
        var label = strings.GetStringFromName(locale);
      } catch (e) {
        label = locale;
      }
      let item = this._languages.appendItem(label, locale);
      if (locale == selectedLocale) {
        this._currentLocale = locale;
        selectedItem = item;
      }
      localeCount++;
    }

    
    let autoDetect = false;
    try {
      autoDetect = gPrefService.getBoolPref("intl.locale.matchOS");
    }
    catch (e) {}
    
    
    if (autoDetect)
      this._languages.selectedItem = document.getElementById("prefs-languages-auto");
    else
      this._languages.selectedItem = selectedItem;
    
    
    if (localeCount == 1)
      document.getElementById("prefs-uilanguage").hidden = true;
  },
  
  updateLocale: function updateLocale() {
    
    let newLocale = this._languages.selectedItem.value;
    
    if (newLocale == "auto") {
      if (gPrefService.prefHasUserValue("general.useragent.locale"))
        gPrefService.clearUserPref("general.useragent.locale");
      gPrefService.setBoolPref("intl.locale.matchOS", true);
    } else {
      gPrefService.setBoolPref("intl.locale.matchOS", false);
      gPrefService.setCharPref("general.useragent.locale", newLocale);
    }

    
    if (this._currentLocale == newLocale)
      this.hideRestart();
    else
      this.showRestart();
  },

  _loadHomePage: function _loadHomePage() {
    let url = Browser.getHomePage();
    let value = "default";
    let display = url;

    switch (url) {
      case "about:blank":
        value = "none";
        display = "";
        break;
      case "about:home":
        value = "default";
        display = "";
        break;
      default:
        value = "custom";
        break;
    }

    
    document.getElementById("prefs-homepage").setAttribute("desc", display);
  
    
    let options = document.getElementById("prefs-homepage-options");
    if (value == "custom") {
      
      options.selectedIndex = -1;
      options.setAttribute("label", Elements.browserBundle.getString("homepage.custom2"));
    } else {
      
      options.value = value;
    }
  },

  updateHomePage: function updateHomePage() {
    let url = "about:home";
    let options = document.getElementById("prefs-homepage-options");
    let value = options.selectedItem.value;
    let display = "";

    switch (value) {
      case "none":
        url = "about:blank";
        break;
      case "default":
        url = "about:home";
        break;
      case "custom":
        url = Browser.selectedBrowser.currentURI.spec;
        display = url;
        break;
    }

    
    document.getElementById("prefs-homepage").setAttribute("desc", display);

    let options = document.getElementById("prefs-homepage-options");
    if (value == "custom") {
      
      options.selectedIndex = -1;
      options.setAttribute("label", Elements.browserBundle.getString("homepage.custom2"));
    }

    
    let pls = Cc["@mozilla.org/pref-localizedstring;1"].createInstance(Ci.nsIPrefLocalizedString);
    pls.data = url;
    gPrefService.setComplexValue("browser.startup.homepage", Ci.nsIPrefLocalizedString, pls);
  }
};
