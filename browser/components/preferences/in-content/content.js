



var gContentPane = {
  init: function ()
  {
    function setEventListener(aId, aEventType, aCallback)
    {
      document.getElementById(aId)
              .addEventListener(aEventType, aCallback.bind(gContentPane));
    }

    
    this._rebuildFonts();
    var menulist = document.getElementById("defaultFont");
    if (menulist.selectedIndex == -1) {
      menulist.insertItemAt(0, "", "", "");
      menulist.selectedIndex = 0;
    }

    
    const prefName = "browser.translation.ui.show";
    if (Services.prefs.getBoolPref(prefName)) {
      let row = document.getElementById("translationBox");
      row.removeAttribute("hidden");
    }

    setEventListener("font.language.group", "change",
      gContentPane._rebuildFonts);
    setEventListener("popupPolicyButton", "command",
      gContentPane.showPopupExceptions);
    setEventListener("advancedFonts", "command",
      gContentPane.configureFonts);
    setEventListener("colors", "command",
      gContentPane.configureColors);
    setEventListener("chooseLanguage", "command",
      gContentPane.showLanguages);
    setEventListener("translationAttributionImage", "click",
      gContentPane.openTranslationProviderAttribution);
    setEventListener("translateButton", "command",
      gContentPane.showTranslationExceptions);

    let drmInfoURL =
      Services.urlFormatter.formatURLPref("app.support.baseURL") + "drm-content";
    document.getElementById("playDRMContentLink").setAttribute("href", drmInfoURL);
    let emeUIEnabled = Services.prefs.getBoolPref("browser.eme.ui.enabled");
    
    if (navigator.platform.toLowerCase().startsWith("win")) {
      emeUIEnabled = emeUIEnabled && parseFloat(Services.sysinfo.get("version")) >= 6;
    }
    if (!emeUIEnabled) {
      
      
      document.getElementById("drmGroup").setAttribute("style", "display: none !important");
    }
  },

  

  



  updateButtons: function (aButtonID, aPreferenceID)
  {
    var button = document.getElementById(aButtonID);
    var preference = document.getElementById(aPreferenceID);
    button.disabled = preference.value != true;
    return undefined;
  },

  

  






  

  



  showPopupExceptions: function ()
  {
    var bundlePreferences = document.getElementById("bundlePreferences");
    var params = { blockVisible: false, sessionVisible: false, allowVisible: true,
                   prefilledHost: "", permissionType: "popup" }
    params.windowTitle = bundlePreferences.getString("popuppermissionstitle");
    params.introText = bundlePreferences.getString("popuppermissionstext");

    gSubDialog.open("chrome://browser/content/preferences/permissions.xul",
                    "resizable=yes", params);
  },

  

  


  _rebuildFonts: function ()
  {
    var preferences = document.getElementById("contentPreferences");
    
    preferences.hidden = false;
    
    preferences.clientHeight;
    var langGroupPref = document.getElementById("font.language.group");
    this._selectDefaultLanguageGroup(langGroupPref.value,
                                     this._readDefaultFontTypeForLanguage(langGroupPref.value) == "serif");
  },

  


  _selectDefaultLanguageGroup: function (aLanguageGroup, aIsSerif)
  {
    const kFontNameFmtSerif         = "font.name.serif.%LANG%";
    const kFontNameFmtSansSerif     = "font.name.sans-serif.%LANG%";
    const kFontNameListFmtSerif     = "font.name-list.serif.%LANG%";
    const kFontNameListFmtSansSerif = "font.name-list.sans-serif.%LANG%";
    const kFontSizeFmtVariable      = "font.size.variable.%LANG%";

    var preferences = document.getElementById("contentPreferences");
    var prefs = [{ format   : aIsSerif ? kFontNameFmtSerif : kFontNameFmtSansSerif,
                   type     : "fontname",
                   element  : "defaultFont",
                   fonttype : aIsSerif ? "serif" : "sans-serif" },
                 { format   : aIsSerif ? kFontNameListFmtSerif : kFontNameListFmtSansSerif,
                   type     : "unichar",
                   element  : null,
                   fonttype : aIsSerif ? "serif" : "sans-serif" },
                 { format   : kFontSizeFmtVariable,
                   type     : "int",
                   element  : "defaultFontSize",
                   fonttype : null }];
    for (var i = 0; i < prefs.length; ++i) {
      var preference = document.getElementById(prefs[i].format.replace(/%LANG%/, aLanguageGroup));
      if (!preference) {
        preference = document.createElement("preference");
        var name = prefs[i].format.replace(/%LANG%/, aLanguageGroup);
        preference.id = name;
        preference.setAttribute("name", name);
        preference.setAttribute("type", prefs[i].type);
        preferences.appendChild(preference);
      }

      if (!prefs[i].element)
        continue;

      var element = document.getElementById(prefs[i].element);
      if (element) {
        element.setAttribute("preference", preference.id);

        if (prefs[i].fonttype)
          FontBuilder.buildFontList(aLanguageGroup, prefs[i].fonttype, element);

        preference.setElementValue(element);
      }
    }
  },

  



  _readDefaultFontTypeForLanguage: function (aLanguageGroup)
  {
    const kDefaultFontType = "font.default.%LANG%";
    var defaultFontTypePref = kDefaultFontType.replace(/%LANG%/, aLanguageGroup);
    var preference = document.getElementById(defaultFontTypePref);
    if (!preference) {
      preference = document.createElement("preference");
      preference.id = defaultFontTypePref;
      preference.setAttribute("name", defaultFontTypePref);
      preference.setAttribute("type", "string");
      preference.setAttribute("onchange", "gContentPane._rebuildFonts();");
      document.getElementById("contentPreferences").appendChild(preference);
    }
    return preference.value;
  },

  


  
  configureFonts: function ()
  {
    gSubDialog.open("chrome://browser/content/preferences/fonts.xul", "resizable=no");
  },

  



  configureColors: function ()
  {
    gSubDialog.open("chrome://browser/content/preferences/colors.xul", "resizable=no");
  },

  

  


  showLanguages: function ()
  {
    gSubDialog.open("chrome://browser/content/preferences/languages.xul");
  },

  



  showTranslationExceptions: function ()
  {
    gSubDialog.open("chrome://browser/content/preferences/translation.xul");
  },

  openTranslationProviderAttribution: function ()
  {
    Components.utils.import("resource:///modules/translation/Translation.jsm");
    Translation.openProviderAttribution();
  }
};
