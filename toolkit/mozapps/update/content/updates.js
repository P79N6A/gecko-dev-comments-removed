







































Components.utils.import("resource://gre/modules/DownloadUtils.jsm");



const CoC = Components.classes;
const CoI = Components.interfaces;
const CoR = Components.results;

const XMLNS_XUL               = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const PREF_UPDATE_MANUAL_URL        = "app.update.url.manual";
const PREF_APP_UPDATE_LOG_BRANCH    = "app.update.log.";
const PREF_UPDATE_TEST_LOOP         = "app.update.test.loop";
const PREF_UPDATE_NEVER_BRANCH      = "app.update.never.";
const PREF_AUTO_UPDATE_ENABLED      = "app.update.enabled";
const PREF_PLUGINS_UPDATEURL        = "plugins.update.url";

const UPDATE_TEST_LOOP_INTERVAL     = 2000;

const URI_UPDATES_PROPERTIES  = "chrome://mozapps/locale/update/updates.properties";

const STATE_DOWNLOADING       = "downloading";
const STATE_PENDING           = "pending";
const STATE_APPLYING          = "applying";
const STATE_SUCCEEDED         = "succeeded";
const STATE_DOWNLOAD_FAILED   = "download-failed";
const STATE_FAILED            = "failed";

const SRCEVT_FOREGROUND       = 1;
const SRCEVT_BACKGROUND       = 2;

var gConsole    = null;
var gPref       = null;
var gLogEnabled = { };






function LOG(module, string) {
  if (module in gLogEnabled || "all" in gLogEnabled) {
    dump("*** AUS:UI " + module + ":" + string + "\n");
    gConsole.logStringMessage("AUS:UI " + module + ":" + string);
  }
}






function openUpdateURL(event) {
  if (event.button == 0)
    openURL(event.target.getAttribute("url"));
}













function getPref(func, preference, defaultValue) {
  try {
    return gPref[func](preference);
  }
  catch (e) {
    LOG("General", "getPref - failed to get preference: " + preference);
  }
  return defaultValue;
}




var gUpdates = {
  



  update: null,

  


  strings: null,

  


  brandName: null,

  


  wiz: null,

  



  _setButton: function(button, string) {
    if (string) {
      var label = this.getAUSString(string);
      if (label.indexOf("%S") != -1)
        label = label.replace(/%S/, this.brandName);
      button.label = label;
      button.setAttribute("accesskey",
                          this.getAUSString(string + ".accesskey"));
    } else {
      button.label = button.defaultLabel;
      button.setAttribute("accesskey", button.defaultAccesskey);
    }
  },

  




























  setButtons: function(extra1ButtonString, extra2ButtonString,
                       nextFinishButtonString, canAdvance) {
    this.wiz.canAdvance = canAdvance;

    var bnf = this.wiz.getButton((this.wiz.onLastPage ? "finish" : "next"));
    var be1 = this.wiz.getButton("extra1");
    var be2 = this.wiz.getButton("extra2");

    
    this._setButton(bnf, nextFinishButtonString);
    this._setButton(be1, extra1ButtonString);
    this._setButton(be2, extra2ButtonString);

    bnf.hidden = !nextFinishButtonString;
    be1.hidden = !extra1ButtonString;
    be2.hidden = !extra2ButtonString;
    
    this.wiz.getButton("back").hidden = true;
  },

  getAUSString: function(key, strings) {
    if (strings)
      return this.strings.getFormattedString(key, strings);
    return this.strings.getString(key);
  },

  never: function () {
    
    
    
    
    
    
    var neverPrefName = PREF_UPDATE_NEVER_BRANCH +
                       encodeURIComponent(gUpdates.update.version);
    gPref.setBoolPref(neverPrefName, true);
    this.wiz.cancel();
  },

  later: function () {
    
    this.wiz.cancel();
  },

  



  _pages: { },

  



  onWizardFinish: function() {
    var pageid = document.documentElement.currentPage.pageid;
    if ("onWizardFinish" in this._pages[pageid])
      this._pages[pageid].onWizardFinish();
  },

  



  onWizardCancel: function() {
    var pageid = document.documentElement.currentPage.pageid;
    if ("onWizardCancel" in this._pages[pageid])
      this._pages[pageid].onWizardCancel();
  },

  



  onWizardNext: function() {
    var cp = document.documentElement.currentPage;
    if (!cp)
      return;
    var pageid = cp.pageid;
    if ("onWizardNext" in this._pages[pageid])
      this._pages[pageid].onWizardNext();
  },

  











  sourceEvent: SRCEVT_FOREGROUND,

  



  errorMessage: "",

  



  _cacheButtonStrings: function (buttonName) {
    var button = this.wiz.getButton(buttonName);
    button.defaultLabel = button.label;
    button.defaultAccesskey = button.getAttribute("accesskey");
  },

  


  onLoad: function() {
    this.wiz = document.documentElement;

    gPref = CoC["@mozilla.org/preferences-service;1"].
            getService(CoI.nsIPrefBranch2);
    gConsole = CoC["@mozilla.org/consoleservice;1"].
               getService(CoI.nsIConsoleService);
    this._initLoggingPrefs();

    this.strings = document.getElementById("updateStrings");
    var brandStrings = document.getElementById("brandStrings");
    this.brandName = brandStrings.getString("brandShortName");

    var pages = gUpdates.wiz.childNodes;
    for (var i = 0; i < pages.length; ++i) {
      var page = pages[i];
      if (page.localName == "wizardpage")
        this._pages[page.pageid] = eval(page.getAttribute("object"));
    }

    
    this._cacheButtonStrings("next");
    this._cacheButtonStrings("finish");
    this._cacheButtonStrings("extra1");
    this._cacheButtonStrings("extra2");

    this.wiz.getButton("cancel").hidden = true;

    
    var startPage = this.startPage;
    LOG("gUpdates", "onLoad - setting current page to startpage " + startPage.id);
    gUpdates.wiz.currentPage = startPage;
  },

  



  _initLoggingPrefs: function() {
    try {
      var ps = CoC["@mozilla.org/preferences-service;1"].
               getService(CoI.nsIPrefService);
      var logBranch = ps.getBranch(PREF_APP_UPDATE_LOG_BRANCH);
      var modules = logBranch.getChildList("");

      for (var i = 0; i < modules.length; ++i) {
        if (logBranch.prefHasUserValue(modules[i]))
          gLogEnabled[modules[i]] = logBranch.getBoolPref(modules[i]);
      }
    }
    catch (e) {
    }
  },

  













  get startPage() {
    if ("arguments" in window && window.arguments[0]) {
      var arg0 = window.arguments[0];
      if (arg0 instanceof CoI.nsIUpdate) {
        
        
        
        this.setUpdate(arg0);
        var p = this.update.selectedPatch;
        if (p) {
          var state = p.state;
          if (state == STATE_DOWNLOADING) {
            var patchFailed = false;
            try {
              patchFailed = this.update.getProperty("patchingFailed");
            }
            catch (e) {
            }
            if (patchFailed == "partial") {
              
              
              
              state = STATE_FAILED;
            }
            else if (patchFailed == "complete") {
              
              
              
              
              state = STATE_DOWNLOAD_FAILED;
            }
          }

          
          
          switch (state) {
          case STATE_PENDING:
            this.sourceEvent = SRCEVT_BACKGROUND;
            return document.getElementById("finishedBackground");
          case STATE_DOWNLOADING:
            return document.getElementById("downloading");
          case STATE_FAILED:
            window.getAttention();
            return document.getElementById("errorpatching");
          case STATE_DOWNLOAD_FAILED:
          case STATE_APPLYING:
            return document.getElementById("errors");
          }
        }
        return document.getElementById("incompatibleCheck");
      }
      else if (arg0 == "installed") {
        return document.getElementById("installed");
      }
    }
    else {
      var um = CoC["@mozilla.org/updates/update-manager;1"].
               getService(CoI.nsIUpdateManager);
      if (um.activeUpdate) {
        this.setUpdate(um.activeUpdate);
        return document.getElementById("downloading");
      }
    }
    return document.getElementById("checking");
  },

  




  setUpdate: function(update) {
    this.update = update;
    if (this.update)
      this.update.QueryInterface(CoI.nsIWritablePropertyBag);
  }
}





var gCheckingPage = {
  



  _checker: null,

  


  onPageShow: function() {
    document.getElementById("checkingProgress").hidden = false;
    gUpdates.wiz.getButton("cancel").hidden = false;
    gUpdates.setButtons(null, null, null, false);
    this._checker = CoC["@mozilla.org/updates/update-checker;1"].
                    createInstance(CoI.nsIUpdateChecker);
    this._checker.checkForUpdates(this.updateListener, true);
  },

  



  onWizardCancel: function() {
    this._checker.stopChecking(CoI.nsIUpdateChecker.CURRENT_CHECK);
  },

  



  updateListener: {
    


    onProgress: function(request, position, totalSize) {
      var pm = document.getElementById("checkingProgress");
      checkingProgress.setAttribute("mode", "normal");
      checkingProgress.setAttribute("value", Math.floor(100 * (position / totalSize)));
    },

    


    onCheckComplete: function(request, updates, updateCount) {
      document.getElementById("checkingProgress").hidden = true;
      gUpdates.wiz.getButton("cancel").hidden = true;
      var aus = CoC["@mozilla.org/updates/update-service;1"].
                getService(CoI.nsIApplicationUpdateService);
      gUpdates.setUpdate(aus.selectUpdate(updates, updates.length));
      if (gUpdates.update) {
        LOG("gCheckingPage", "onCheckComplete - update found");
        gUpdates.wiz.currentPage.setAttribute("next", "incompatibleCheck");
      }
      else
        LOG("gCheckingPage", "onCheckComplete - no update found");

      gUpdates.wiz.canAdvance = true;
      gUpdates.wiz.advance();
    },

    


    onError: function(request, update) {
      LOG("gCheckingPage", "onError - proceeding to error page");
      document.getElementById("checkingProgress").hidden = true;
      gUpdates.wiz.getButton("cancel").hidden = true;
      gUpdates.setUpdate(update);
      gUpdates.wiz.currentPage = document.getElementById("errors");
    },

    


    QueryInterface: function(aIID) {
      if (!aIID.equals(CoI.nsIUpdateCheckListener) &&
          !aIID.equals(CoI.nsISupports))
        throw CoR.NS_ERROR_NO_INTERFACE;
      return this;
    }
  }
};




var gPluginsPage = {
  


  _url: null,
  
  


  onPageShow: function() {
    if (gPref.getPrefType(PREF_PLUGINS_UPDATEURL) == gPref.PREF_INVALID) {
      gUpdates.wiz.goTo("noupdatesfound");
      return;
    }
    
    var formatter = CoC["@mozilla.org/toolkit/URLFormatterService;1"].
                       getService(CoI.nsIURLFormatter);
    this._url = formatter.formatURLPref(PREF_PLUGINS_UPDATEURL);
    var link = document.getElementById("pluginupdateslink");
    link.setAttribute("href", this._url);


    var phs = CoC["@mozilla.org/plugin/host;1"].
                 getService(CoI.nsIPluginHost);
    var plugins = phs.getPluginTags({});
    var blocklist = CoC["@mozilla.org/extensions/blocklist;1"].
                      getService(CoI.nsIBlocklistService);

    var hasOutdated = false;
    for (let i = 0; i < plugins.length; i++) {
      let pluginState = blocklist.getPluginBlocklistState(plugins[i]);
      if (pluginState == CoI.nsIBlocklistService.STATE_OUTDATED) {
        hasOutdated = true;
        break;
      }
    }
    if (!hasOutdated) {
      gUpdates.wiz.goTo("noupdatesfound");
      return;
    }

    gUpdates.setButtons(null, null, "okButton", true);
    gUpdates.wiz.getButton("finish").focus();
  },
  
  


  onWizardFinish: function() {
    openURL(this._url);
  }
};




var gNoUpdatesPage = {
  


  onPageShow: function() {
    LOG("gNoUpdatesPage", "onPageShow - could not select an appropriate " +
        "update. Either there were no updates or |selectUpdate| failed");

    if (getPref("getBoolPref", PREF_AUTO_UPDATE_ENABLED, true))
      document.getElementById("noUpdatesAutoEnabled").hidden = false;
    else
      document.getElementById("noUpdatesAutoDisabled").hidden = false;

    gUpdates.setButtons(null, null, "okButton", true);
    gUpdates.wiz.getButton("finish").focus();
  }
};





var gIncompatibleCheckPage = {
  


  addons: [],

  


  _totalCount: 0,

  


  _completedCount: 0,

  


  _pBar: null,

  


  onPageShow: function() {
    var ai = CoC["@mozilla.org/xre/app-info;1"].getService(CoI.nsIXULAppInfo);
    var vc = CoC["@mozilla.org/xpcom/version-comparator;1"].
             getService(CoI.nsIVersionComparator);
    if (!gUpdates.update.extensionVersion ||
        vc.compare(gUpdates.update.extensionVersion, ai.version) == 0) {
      
      gUpdates.wiz.advance();
      return;
    }

    var em = CoC["@mozilla.org/extensions/manager;1"].
             getService(CoI.nsIExtensionManager);
    this.addons = em.getIncompatibleItemList(gUpdates.update.extensionVersion,
                                             gUpdates.update.platformVersion,
                                             CoI.nsIUpdateItem.TYPE_ANY, false,
                                             { });
    if (this.addons.length > 0) {
      
      
      var addons = em.getIncompatibleItemList(null, null,
                                              CoI.nsIUpdateItem.TYPE_ANY, false,
                                              { });
      for (var i = 0; i < addons.length; ++i) {
        for (var j = 0; j < this.addons.length; ++j) {
          if (addons[i].id == this.addons[j].id) {
            this.addons.splice(j, 1);
            break;
          }
        }
      }
    }

    if (this.addons.length == 0) {
      
      gUpdates.wiz.advance();
      return;
    }

    LOG("gIncompatibleCheckPage", "onPageShow - checking for updates to " +
        "incompatible add-ons");

    gUpdates.wiz.getButton("cancel").hidden = false;
    gUpdates.setButtons(null, null, null, false);
    gUpdates.wiz.getButton("cancel").focus();
    this._pBar = document.getElementById("incompatibleCheckProgress");
    this._pBar.hidden = false;
    this._totalCount = this.addons.length;

    var em = CoC["@mozilla.org/extensions/manager;1"].
             getService(CoI.nsIExtensionManager);
    em.update(this.addons, this.addons.length,
              CoI.nsIExtensionManager.UPDATE_NOTIFY_NEWVERSION, this,
              CoI.nsIExtensionManager.UPDATE_WHEN_NEW_APP_DETECTED,
              gUpdates.update.extensionVersion, gUpdates.update.platformVersion);
  },

  


  onUpdateStarted: function() {
    this._pBar.mode = "normal";
  },

  


  onUpdateEnded: function() {
    if (this.addons.length == 0) {
      LOG("gIncompatibleCheckPage", "onUpdateEnded - updates were found " +
          "for all incompatible add-ons");
    }
    else {
      LOG("gIncompatibleCheckPage", "onUpdateEnded - there are still " +
          "incompatible add-ons");
    }

    this._pBar.hidden = true;
    gUpdates.wiz.getButton("cancel").hidden = true;
    gUpdates.wiz.canAdvance = true;
    gUpdates.wiz.advance();
  },

  


  onAddonUpdateStarted: function(addon) {
  },

  


  onAddonUpdateEnded: function(addon, status) {
    ++this._completedCount;
    this._pBar.value = Math.ceil((this._completedCount / this._totalCount) * 100);

    if (status != CoI.nsIAddonUpdateCheckListener.STATUS_UPDATE &&
        status != CoI.nsIAddonUpdateCheckListener.STATUS_VERSIONINFO)
      return;

    for (var i = 0; i < this.addons.length; ++i) {
      if (this.addons[i].id == addon.id) {
        LOG("gIncompatibleCheckPage", "onAddonUpdateEnded - found update " +
            "for add-on ID: " + addon.id);
        this.addons.splice(i, 1);
        break;
      }
    }
  },

  QueryInterface: function(iid) {
    if (!iid.equals(CoI.nsIAddonUpdateCheckListener) &&
        !iid.equals(CoI.nsISupports))
      throw CoR.NS_ERROR_NO_INTERFACE;
    return this;
  }
};





var gUpdatesAvailablePage = {
  


  _loaded: false,

  


  onPageShow: function() {
    var severity = gUpdates.update.type;
    gUpdates.setButtons("askLaterButton",
                        severity == "major" ? "noThanksButton" : null,
                        "updateButton_" + severity, true);
    var btn = gUpdates.wiz.getButton("next");
    btn.className += " heed";
    btn.focus();

    if (this._loaded)
      return;

    if (!gUpdates.update.licenseURL) {
      if (gIncompatibleCheckPage.addons.length == 0)
        gUpdates.wiz.currentPage.setAttribute("next", "downloading");
      else
        gUpdates.wiz.currentPage.setAttribute("next", "incompatibleList");
    }

    var updateName = gUpdates.update.name;
    if (gUpdates.update.channel == "nightly") {
      updateName = gUpdates.getAUSString("updateName", [gUpdates.brandName,
                                                        gUpdates.update.version]);
      updateName = updateName + " " + gUpdates.update.buildID + " nightly";
    }
    var updateNameElement = document.getElementById("updateName");
    updateNameElement.value = updateName;
    var updateTypeElement = document.getElementById("updateType");
    updateTypeElement.setAttribute("severity", severity);

    var moreInfoContent = document.getElementById("moreInfoContent");
    var intro;
    if (severity == "major") {
      
      intro = gUpdates.getAUSString("intro_major_app_and_version",
                                    [gUpdates.brandName, gUpdates.update.version]);
      var remoteContent = document.getElementById("updateMoreInfoContent");
      
      
      
      remoteContent.update_name = gUpdates.brandName;
      remoteContent.update_version = gUpdates.update.version;
      remoteContent.url = gUpdates.update.detailsURL;

      moreInfoContent.hidden = false;
      document.getElementById("moreInfoURL").hidden = true;
      document.getElementById("updateName").hidden = true;
      document.getElementById("updateNameSep").hidden = true;
      document.getElementById("upgradeEvangelism").hidden = true;
      document.getElementById("upgradeEvangelismSep").hidden = true;

      
      
      
      
      
      
      
      var neverPrefName = PREF_UPDATE_NEVER_BRANCH +
                          encodeURIComponent(gUpdates.update.version);
      gPref.setBoolPref(neverPrefName, false);
    }
    else {
      
      intro = gUpdates.getAUSString("intro_minor_app", [gUpdates.brandName]);
      
      
      moreInfoContent.parentNode.removeChild(moreInfoContent);
      var updateMoreInfoURL = document.getElementById("updateMoreInfoURL");
      updateMoreInfoURL.setAttribute("url", gUpdates.update.detailsURL);
    }
    updateTypeElement.textContent = intro;

    var updateTitle = gUpdates.getAUSString("updatesfound_" + severity +
                                            ".title");
    gUpdates.wiz.currentPage.setAttribute("label", updateTitle);
    
    
    gUpdates.wiz._adjustWizardHeader();

    this._loaded = true;
  },

  onExtra1: function() {
    this.onWizardCancel();
    gUpdates.later();
  },

  onExtra2: function() {
    this.onWizardCancel();
    gUpdates.never();
  },

  onWizardNext: function() {
    var regex = new RegExp('\\s*heed');
    var btn = gUpdates.wiz.getButton("next");
    btn.className = btn.className.replace(regex, "");
  },

  


  onWizardCancel: function() {
    try {
      var remoteContent = document.getElementById("updateMoreInfoContent");
      if (remoteContent)
        remoteContent.stopDownloading();
    }
    catch (e) {
      LOG("gUpdatesAvailablePage", "onWizardCancel - " +
          "moreInfoContent.stopDownloading() failed: " + e);
    }
  }
};






var gLicensePage = {
  


  _licenseLoaded: false,

  


  onPageShow: function() {
    gUpdates.setButtons("backButton", null, "acceptTermsButton", false);

    var licenseContent = document.getElementById("licenseContent");
    if (this._licenseLoaded || licenseContent.getAttribute("state") == "error") {
      this.onAcceptDeclineRadio();
      var licenseGroup = document.getElementById("acceptDeclineLicense");
      if (licenseGroup.selectedIndex == 0)
        gUpdates.wiz.getButton("next").focus();
      else
        licenseGroup.focus();
      return;
    }

    gUpdates.wiz.getButton("extra1").disabled = true;
    if (gIncompatibleCheckPage.addons.length == 0)
      gUpdates.wiz.currentPage.setAttribute("next", "downloading");

    
    document.getElementById("acceptDeclineLicense").disabled = true;
    gUpdates.update.setProperty("licenseAccepted", "false");

    licenseContent.addEventListener("load", gLicensePage.onLicenseLoad, false);
    
    
    
    licenseContent.update_name = gUpdates.brandName;
    licenseContent.update_version = gUpdates.update.version;
    licenseContent.url = gUpdates.update.licenseURL;
  },

  


  onLicenseLoad: function() {
    var licenseContent = document.getElementById("licenseContent");
    
    
    
    var state = licenseContent.getAttribute("state");
    if (state == "loading")
      return;

    licenseContent.removeEventListener("load", gLicensePage.onLicenseLoad, false);

    var errorLoading = (state == "error");
    document.getElementById("acceptDeclineLicense").disabled = errorLoading;
    gLicensePage._licenseLoaded = !errorLoading;
    gUpdates.wiz.getButton("extra1").disabled = false;
  },

  


  onAcceptDeclineRadio: function() {
    
    
    
    if (!this._licenseLoaded)
      return;

    var selectedIndex = document.getElementById("acceptDeclineLicense")
                                .selectedIndex;
    
    var licenseAccepted = (selectedIndex == 0);
    gUpdates.wiz.canAdvance = licenseAccepted;
  },

  onExtra1: function() {
    gUpdates.wiz.currentPage = document.getElementById("updatesfound");
  },

  


  onWizardNext: function() {
    try {
      gUpdates.update.setProperty("licenseAccepted", "true");
      var um = CoC["@mozilla.org/updates/update-manager;1"].
               getService(CoI.nsIUpdateManager);
      um.saveUpdates();
    }
    catch (e) {
      LOG("gLicensePage", "onWizardNext - nsIUpdateManager:saveUpdates() " +
          "failed: " + e);
    }
  },

  


  onWizardCancel: function() {
    try {
      var licenseContent = document.getElementById("licenseContent");
      
      if (licenseContent)
        licenseContent.stopDownloading();
    }
    catch (e) {
      LOG("gLicensePage", "onWizardCancel - " +
          "licenseContent.stopDownloading() failed: " + e);
    }
  }
};






var gIncompatibleListPage = {
  


  onPageShow: function() {
    gUpdates.setButtons("backButton", null, "okButton", true);
    var listbox = document.getElementById("incompatibleList.listbox");
    if (listbox.children.length > 0)
      return;

    var severity = gUpdates.update.type;
    var intro;
    if (severity == "major")
      intro = gUpdates.getAUSString("incompatibleAddons_" + severity,
                                    [gUpdates.brandName, gUpdates.update.version,
                                     gUpdates.brandName]);
    else
      intro = gUpdates.getAUSString("incompatibleAddons_" + severity,
                                    [gUpdates.brandName]);

    document.getElementById("incompatibleListDesc").textContent = intro;

    var addons = gIncompatibleCheckPage.addons;
    for (var i = 0; i < addons.length; ++i) {
      var listitem = document.createElement("listitem");
      listitem.setAttribute("label", addons[i].name + " " + addons[i].version);
      listbox.appendChild(listitem);
    }
  },

  onExtra1: function() {
    var updatesfoundPage = document.getElementById("updatesfound");
    if (updatesfoundPage.getAttribute("next") == "license")
      gUpdates.wiz.currentPage = document.getElementById("license");
    else
      gUpdates.wiz.currentPage = updatesfoundPage;
  }
};





var gDownloadingPage = {
  


  _downloadName: null,
  _downloadStatus: null,
  _downloadProgress: null,
  _downloadThrobber: null,
  _pauseButton: null,

  


  _paused: false,

  


  _label_downloadStatus: null,

  


  _lastSec: Infinity,
  _startTime: Date.now(),
  _pausedStatus: "",

  


  onPageShow: function() {
    this._downloadName = document.getElementById("downloadName");
    this._downloadStatus = document.getElementById("downloadStatus");
    this._downloadProgress = document.getElementById("downloadProgress");
    this._downloadProgress.hidden = false;
    this._downloadThrobber = document.getElementById("downloadThrobber");
    this._pauseButton = document.getElementById("pauseButton");
    this._label_downloadStatus = this._downloadStatus.textContent;

    this._pauseButton.setAttribute("tooltiptext",
                                   gUpdates.getAUSString("pauseButtonPause"));

    
    this._pauseButton.focus();
    this._pauseButton.disabled = true;

    var aus = CoC["@mozilla.org/updates/update-service;1"].
              getService(CoI.nsIApplicationUpdateService);

    var um = CoC["@mozilla.org/updates/update-manager;1"].
             getService(CoI.nsIUpdateManager);
    var activeUpdate = um.activeUpdate;
    if (activeUpdate)
      gUpdates.setUpdate(activeUpdate);

    if (!gUpdates.update) {
      LOG("gDownloadingPage", "onPageShow - no valid update to download?!");
      return;
    }

    try {
      
      
      gUpdates.update.QueryInterface(CoI.nsIWritablePropertyBag);
      gUpdates.update.setProperty("foregroundDownload", "true");

      
      
      aus.pauseDownload();
      var state = aus.downloadUpdate(gUpdates.update, false);
      if (state == "failed") {
        
        
        
        
        this.showVerificationError();
      }
      else {
        
        aus.addDownloadListener(this);
      }

      if (activeUpdate)
        this._setUIState(!aus.isDownloading);
    }
    catch(e) {
      LOG("gDownloadingPage", "onPageShow - error: " + e);
    }

    var link = document.getElementById("downloadDetailsLink");
    link.setAttribute("url", gUpdates.update.detailsURL);

    gUpdates.setButtons("hideButton", null, null, false);
    gUpdates.wiz.getButton("extra1").focus();
  },

  


  _setStatus: function(status) {
    
    
    if (this._downloadStatus.textContent == status)
      return;
    while (this._downloadStatus.hasChildNodes())
      this._downloadStatus.removeChild(this._downloadStatus.firstChild);
    this._downloadStatus.appendChild(document.createTextNode(status));
  },

  









  _updateDownloadStatus: function(aCurr, aMax) {
    let status;

    
    let rate = aCurr / (Date.now() - this._startTime) * 1000;
    [status, this._lastSec] =
      DownloadUtils.getDownloadStatus(aCurr, aMax, rate, this._lastSec);

    
    this._pausedStatus = DownloadUtils.getTransferTotal(aCurr, aMax);

    return status;
  },

  




  _setUIState: function(paused) {
    var u = gUpdates.update;
    if (paused) {
      if (this._downloadThrobber.hasAttribute("state"))
        this._downloadThrobber.removeAttribute("state");
      if (this._downloadProgress.mode != "normal")
        this._downloadProgress.mode = "normal";
      this._downloadName.value = gUpdates.getAUSString("pausedName", [u.name]);
      this._pauseButton.setAttribute("tooltiptext",
                                     gUpdates.getAUSString("pauseButtonResume"));
      this._pauseButton.setAttribute("paused", "true");
      var p = u.selectedPatch.QueryInterface(CoI.nsIPropertyBag);
      var status = p.getProperty("status");
      if (status) {
        let pausedStatus = gUpdates.getAUSString("pausedStatus", [status]);
        this._setStatus(pausedStatus);
      }
    }
    else {
      if (!(this._downloadThrobber.hasAttribute("state") &&
           (this._downloadThrobber.getAttribute("state") == "loading")))
        this._downloadThrobber.setAttribute("state", "loading");
      if (this._downloadProgress.mode != "undetermined")
        this._downloadProgress.mode = "undetermined";
      this._downloadName.value = gUpdates.getAUSString("downloadingPrefix",
                                                       [u.name]);
      this._pauseButton.setAttribute("paused", "false");
      this._pauseButton.setAttribute("tooltiptext",
                                     gUpdates.getAUSString("pauseButtonPause"));
      this._setStatus(this._label_downloadStatus);
    }
  },

  


  onPause: function() {
    var aus = CoC["@mozilla.org/updates/update-service;1"].
              getService(CoI.nsIApplicationUpdateService);
    if (this._paused)
      aus.downloadUpdate(gUpdates.update, false);
    else {
      var patch = gUpdates.update.selectedPatch;
      patch.QueryInterface(CoI.nsIWritablePropertyBag);
      patch.setProperty("status", this._pausedStatus);
      aus.pauseDownload();
    }
    this._paused = !this._paused;

    
    this._setUIState(this._paused);
  },

  


  onHide: function() {
    
    
    
    var aus = CoC["@mozilla.org/updates/update-service;1"].
              getService(CoI.nsIApplicationUpdateService);
    aus.removeDownloadListener(this);

    var um = CoC["@mozilla.org/updates/update-manager;1"].
             getService(CoI.nsIUpdateManager);
    um.activeUpdate = gUpdates.update;

    
    
    var downloadInBackground = true;
    if (this._paused) {
      var title = gUpdates.getAUSString("resumePausedAfterCloseTitle");
      var message = gUpdates.getAUSString("resumePausedAfterCloseMsg",
                                          [gUpdates.brandName]);
      var ps = CoC["@mozilla.org/embedcomp/prompt-service;1"].
               getService(CoI.nsIPromptService);
      var flags = ps.STD_YES_NO_BUTTONS;
      
      
      
      
      window.focus();
      var rv = ps.confirmEx(window, title, message, flags, null, null, null,
                            null, { });
      if (rv == CoI.nsIPromptService.BUTTON_POS_0)
        downloadInBackground = false;
    }
    if (downloadInBackground) {
      
      LOG("gDownloadingPage", "onHide - continuing download in background " +
          "at full speed");
      aus.downloadUpdate(gUpdates.update, false);
    }
    gUpdates.wiz.cancel();
  },

  






  onStartRequest: function(request, context) {
    if (request instanceof CoI.nsIIncrementalDownload)
      LOG("gDownloadingPage", "onStartRequest - spec: " + request.URI.spec);
    
    
    if (this._paused)
      return;

    if (!(this._downloadThrobber.hasAttribute("state") &&
          (this._downloadThrobber.getAttribute("state") == "loading")))
      this._downloadThrobber.setAttribute("state", "loading");
    if (this._downloadProgress.mode != "undetermined")
      this._downloadProgress.mode = "undetermined";
    this._setStatus(this._label_downloadStatus);
  },

  










  onProgress: function(request, context, progress, maxProgress) {
    LOG("gDownloadingPage", "onProgress - progress: " + progress + "/" +
        maxProgress);

    var name = gUpdates.getAUSString("downloadingPrefix", [gUpdates.update.name]);
    let status = this._updateDownloadStatus(progress, maxProgress);
    var currentProgress = Math.round(100 * (progress / maxProgress));

    var p = gUpdates.update.selectedPatch;
    p.QueryInterface(CoI.nsIWritablePropertyBag);
    p.setProperty("progress", currentProgress);
    p.setProperty("status", status);

    
    
    if (this._paused)
      return;

    if (!(this._downloadThrobber.hasAttribute("state") &&
         (this._downloadThrobber.getAttribute("state") == "loading")))
      this._downloadThrobber.setAttribute("state", "loading");
    if (this._downloadProgress.mode != "normal")
      this._downloadProgress.mode = "normal";
    this._downloadProgress.value = currentProgress;
    this._pauseButton.disabled = false;
    this._downloadName.value = name;
    this._setStatus(status);
  },

  










  onStatus: function(request, context, status, statusText) {
    LOG("gDownloadingPage", "onStatus - status: " + status + ", text: " +
        statusText);
    this._setStatus(statusText);
  },

  








  onStopRequest: function(request, context, status) {
    if (request instanceof CoI.nsIIncrementalDownload)
      LOG("gDownloadingPage", "onStopRequest - spec: " + request.URI.spec +
          ", status: " + status);

    if (this._downloadThrobber.hasAttribute("state"))
      this._downloadThrobber.removeAttribute("state");
    if (this._downloadProgress.mode != "normal")
      this._downloadProgress.mode = "normal";

    var u = gUpdates.update;
    switch (status) {
    case CoR.NS_ERROR_UNEXPECTED:
      if (u.selectedPatch.state == STATE_DOWNLOAD_FAILED &&
          u.isCompleteUpdate) {
        
        
        this._downloadProgress.hidden = true;
        gUpdates.wiz.currentPage = document.getElementById("errors");
      }
      else {
        
        

        
        
        this._downloadProgress.mode = "undetermined";
        this._pauseButton.disabled = true;

        var verificationFailed = document.getElementById("verificationFailed");
        verificationFailed.hidden = false;

        return;
      }
      break;
    case CoR.NS_BINDING_ABORTED:
      LOG("gDownloadingPage", "onStopRequest - pausing download");
      
      
      return;
    case CoR.NS_OK:
      LOG("gDownloadingPage", "onStopRequest - patch verification succeeded");
      this._downloadProgress.hidden = true;
      gUpdates.wiz.canAdvance = true;
      gUpdates.wiz.advance();
      break;
    default:
      LOG("gDownloadingPage", "onStopRequest - transfer failed");
      
      this._downloadProgress.hidden = true;
      gUpdates.wiz.currentPage = document.getElementById("errors");
      break;
    }

    var aus = CoC["@mozilla.org/updates/update-service;1"].
              getService(CoI.nsIApplicationUpdateService);
    aus.removeDownloadListener(this);
  },

  


  QueryInterface: function(iid) {
    if (!iid.equals(CoI.nsIRequestObserver) &&
        !iid.equals(CoI.nsIProgressEventSink) &&
        !iid.equals(CoI.nsISupports))
      throw CoR.NS_ERROR_NO_INTERFACE;
    return this;
  }
};




var gErrorsPage = {
  


  onPageShow: function() {
    
    gUpdates.setButtons(null, null, "okButton", true);
    gUpdates.wiz.getButton("finish").focus();

    var statusText = gUpdates.update.statusText;
    LOG("gErrorsPage" , "onPageShow - update.statusText: " + statusText);

    var errorReason = document.getElementById("errorReason");
    errorReason.value = statusText;
    var formatter = CoC["@mozilla.org/toolkit/URLFormatterService;1"].
                    getService(CoI.nsIURLFormatter);
    var manualURL = formatter.formatURLPref(PREF_UPDATE_MANUAL_URL);
    var errorLinkLabel = document.getElementById("errorLinkLabel");
    errorLinkLabel.value = manualURL;
    errorLinkLabel.setAttribute("url", manualURL);
  },

  


  onWizardFinish: function() {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    var aus = CoC["@mozilla.org/updates/update-service;1"].
              getService(CoI.nsIApplicationUpdateService);
    aus.removeDownloadListener(gDownloadingPage);
  }
};





var gFinishedPage = {
  


  onPageShow: function() {
    gUpdates.setButtons("restartLaterButton", null, "restartNowButton",
                        true);
    var btn = gUpdates.wiz.getButton("finish");
    btn.className += " heed";
    btn.focus();
  },

  


  onPageShowBackground: function() {
    this.onPageShow();
    var updateFinishedName = document.getElementById("updateFinishedName");
    updateFinishedName.value = gUpdates.update.name;

    var link = document.getElementById("finishedBackgroundLink");
    link.setAttribute("url", gUpdates.update.detailsURL);
    
    
    link.disabled = false;

    if (getPref("getBoolPref", PREF_UPDATE_TEST_LOOP, false)) {
      setTimeout(function () {
                   gUpdates.wiz.getButton("finish").click();
                 }, UPDATE_TEST_LOOP_INTERVAL);
    }
  },

  



  onWizardFinish: function() {
    
    LOG("gFinishedPage" , "onWizardFinish - restarting the application");

    
    
    
    
    
    
    
    
    
    gUpdates.wiz.getButton("finish").disabled = true;
    gUpdates.wiz.getButton("extra1").disabled = true;

    
    var os = CoC["@mozilla.org/observer-service;1"].
             getService(CoI.nsIObserverService);
    var cancelQuit = CoC["@mozilla.org/supports-PRBool;1"].
                     createInstance(CoI.nsISupportsPRBool);
    os.notifyObservers(cancelQuit, "quit-application-requested", "restart");

    
    if (cancelQuit.data)
      return;

    
    CoC["@mozilla.org/toolkit/app-startup;1"].getService(CoI.nsIAppStartup).
    quit(CoI.nsIAppStartup.eAttemptQuit | CoI.nsIAppStartup.eRestart);
  },

  



  onExtra1: function() {
    
    gUpdates.wiz.cancel();
  }
};




var gInstalledPage = {
  


  onPageShow: function() {
    var ai = CoC["@mozilla.org/xre/app-info;1"].getService(CoI.nsIXULAppInfo);

    var branding = document.getElementById("brandStrings");
    try {
      var url = branding.getFormattedString("whatsNewURL", [ai.version]);
      var whatsnewLink = document.getElementById("whatsnewLink");
      whatsnewLink.setAttribute("url", url);
      whatsnewLink.hidden = false;
    }
    catch (e) {
    }

    gUpdates.setButtons(null, null, "okButton", true);
    gUpdates.wiz.getButton("finish").focus();
  }
};





function tryToClose() {
  var cp = gUpdates.wiz.currentPage;
  if (cp.pageid != "finished" && cp.pageid != "finishedBackground")
    gUpdates.onWizardCancel();
  return true;
}







function setCurrentPage(pageid) {
  gUpdates.wiz.currentPage = document.getElementById(pageid);
}
