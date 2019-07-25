




































var PreferencesView = {
  _currentLocale: null,
  _msg: null,

  _messageActions: function pv__messageActions(aData) {
    if (aData == "prefs-restart-app") {
      
      var cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");

      
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
    if (this._msg) {
      let strings = Strings.browser;
      this.showMessage(strings.GetStringFromName("notificationRestart.normal"), "restart-app",
                       strings.GetStringFromName("notificationRestart.button"), false, "prefs-restart-app");
    }
  },

  hideRestart: function ev_hideRestart() {
    if (this._msg) {
      let notification = this._msg.getNotificationWithValue("restart-app");
      if (notification)
        notification.close();
    }
  },

  delayedInit: function pv__delayedInit() {
    if (this._msg)
      return;

    this._msg = document.getElementById("prefs-messages");
    this._loadLocales();

    this._loadHomePage();

    MasterPasswordUI.updatePreference();
    WeaveGlue.init();

    Services.prefs.addObserver("general.useragent.locale", this, false);
  },

  observe: function(aSubject, aTopic, aData) {
    if (aData == "general.useragent.locale") {
      this.showRestart();
      this._loadLocales();
    }
  },

  _loadLocales: function _loadLocales() {
    
    let chrome = Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIXULChromeRegistry);
    chrome.QueryInterface(Ci.nsIToolkitChromeRegistry);

    let selectedLocale = chrome.getSelectedLocale("browser");

    
    
    if (Services.prefs.prefHasUserValue("general.useragent.locale"))
      selectedLocale = Services.prefs.getCharPref("general.useragent.locale");

    let availableLocales = chrome.getLocalesForPackage("browser");

    let strings = Services.strings.createBundle("chrome://browser/content/languages.properties");

    let selectedItem = null;
    let selectedLabel = selectedLocale;
    while (availableLocales.hasMore()) {
      let locale = availableLocales.getNext();
      try {
        var label = strings.GetStringFromName(locale);
      } catch (e) {
        label = locale;
      }
      if (locale == selectedLocale) {
        selectedLabel = label;
        this._currentLocale = locale;
        break;
      }
    }
    document.getElementById("prefs-uilanguage-button").setAttribute("label", selectedLabel);
  },

  showLocalePicker: function showLocalePicker() {
    Services.ww.openWindow(window, "chrome://browser/content/localePicker.xul", "_browser", "chrome,dialog=no,all", null);
  },

  _showHomePageHint: function _showHomePageHint(aHint) {
    if (aHint)
      document.getElementById("prefs-homepage").setAttribute("desc", aHint);
    else
      document.getElementById("prefs-homepage").removeAttribute("desc");
  },

  _loadHomePage: function _loadHomePage() {
    let url = Browser.getHomePage();
    let value = "default";
    let display = url;
    try {
      display = Services.prefs.getComplexValue("browser.startup.homepage.title", Ci.nsIPrefLocalizedString).data;
    } catch (e) { }

    switch (url) {
      case "about:empty":
        value = "none";
        display = null;
        break;
      case "about:home":
        value = "default";
        display = null;
        break;
      default:
        value = "custom";
        break;
    }

    
    this._showHomePageHint(display);

    
    let options = document.getElementById("prefs-homepage-options");
    if (value == "custom") {
      
      options.appendItem(Strings.browser.GetStringFromName("homepage.custom2"), "custom");
    }

    
    options.value = value;
  },

  updateHomePageList: function updateHomePageMenuList() {
    
    
    let currentUrl = Browser.selectedBrowser.currentURI.spec;
    let currentHomepage = Browser.getHomePage();
    let isHomepage = (currentHomepage == currentUrl);
    let itemRow = document.getElementById("prefs-homepage-currentpage");
    if (currentHomepage == "about:home") {
      itemRow.disabled = isHomepage;
      itemRow.hidden = false;
    } else {
      itemRow.hidden = isHomepage;
      itemRow.disabled = false;
    }
  },

  updateHomePage: function updateHomePage() {
    let options = document.getElementById("prefs-homepage-options");
    let value = options.selectedItem.value;

    let url = "about:home";
    let display = null;

    switch (value) {
      case "none":
        url = "about:empty";
        break;
      case "default":
        url = "about:home";
        break;
      case "currentpage":
        
        let currentURL = Browser.selectedBrowser.currentURI.spec;
        if (currentURL == "about:home") {
          value = "default";
        } else {
          url = currentURL;
          display = Browser.selectedBrowser.contentTitle || currentURL;
        }
        break;
      case "custom":
        
        
        
        return;
    }

    
    this._showHomePageHint(display);

    
    let helper = null;
    let items = options.menupopup.getElementsByAttribute("value", "custom");
    if (items && items.length)
      helper = items[0];

    
    if (value == "currentpage") {
      
      
      if (!helper)
        helper = options.appendItem(Strings.browser.GetStringFromName("homepage.custom2"), "custom");

      options.selectedItem = helper;
    } else {
      if (helper)
        options.menupopup.removeChild(helper);

      options.selectedItem = options.menupopup.getElementsByAttribute("value", value)[0];
    }

    
    let pls = Cc["@mozilla.org/pref-localizedstring;1"].createInstance(Ci.nsIPrefLocalizedString);
    pls.data = url;
    Services.prefs.setComplexValue("browser.startup.homepage", Ci.nsIPrefLocalizedString, pls);

    
    pls.data = display;
    Services.prefs.setComplexValue("browser.startup.homepage.title", Ci.nsIPrefLocalizedString, pls);
  }
};
