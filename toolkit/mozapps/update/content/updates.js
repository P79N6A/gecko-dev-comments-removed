




'use strict';



const {classes: CoC, interfaces: CoI, results: CoR, utils: CoU} = Components;

CoU.import("resource://gre/modules/DownloadUtils.jsm", this);
CoU.import("resource://gre/modules/AddonManager.jsm", this);
CoU.import("resource://gre/modules/Services.jsm", this);
CoU.import("resource://gre/modules/UpdateTelemetry.jsm", this);

const XMLNS_XUL = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const PREF_APP_UPDATE_BACKGROUNDERRORS    = "app.update.backgroundErrors";
const PREF_APP_UPDATE_BILLBOARD_TEST_URL  = "app.update.billboard.test_url";
const PREF_APP_UPDATE_CERT_ERRORS         = "app.update.cert.errors";
const PREF_APP_UPDATE_ENABLED             = "app.update.enabled";
const PREF_APP_UPDATE_LOG                 = "app.update.log";
const PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED = "app.update.notifiedUnsupported";
const PREF_APP_UPDATE_TEST_LOOP           = "app.update.test.loop";
const PREF_APP_UPDATE_URL_MANUAL          = "app.update.url.manual";

const PREFBRANCH_APP_UPDATE_NEVER         = "app.update.never.";

const PREF_EM_HOTFIX_ID                   = "extensions.hotfix.id";
const PREF_PLUGINS_UPDATE_URL             = "plugins.update.url";

const UPDATE_TEST_LOOP_INTERVAL = 2000;

const URI_UPDATES_PROPERTIES  = "chrome://mozapps/locale/update/updates.properties";

const STATE_DOWNLOADING       = "downloading";
const STATE_PENDING           = "pending";
const STATE_PENDING_SVC       = "pending-service";
const STATE_APPLYING          = "applying";
const STATE_APPLIED           = "applied";
const STATE_APPLIED_SVC       = "applied-service";
const STATE_SUCCEEDED         = "succeeded";
const STATE_DOWNLOAD_FAILED   = "download-failed";
const STATE_FAILED            = "failed";

const SRCEVT_FOREGROUND       = 1;
const SRCEVT_BACKGROUND       = 2;

const CERT_ATTR_CHECK_FAILED_NO_UPDATE  = 100;
const CERT_ATTR_CHECK_FAILED_HAS_UPDATE = 101;
const BACKGROUNDCHECK_MULTIPLE_FAILURES = 110;

var gLogEnabled = false;
var gUpdatesFoundPageId;
















function LOG(module, string) {
  if (gLogEnabled) {
    dump("*** AUS:UI " + module + ":" + string + "\n");
    Services.console.logStringMessage("AUS:UI " + module + ":" + string);
  }
}






function openUpdateURL(event) {
  if (event.button == 0)
    openURL(event.target.getAttribute("url"));
}













function getPref(func, preference, defaultValue) {
  try {
    return Services.prefs[func](preference);
  }
  catch (e) {
    LOG("General", "getPref - failed to get preference: " + preference);
  }
  return defaultValue;
}




var gUpdates = {
  



  update: null,

  


  addons: [],

  


  strings: null,

  


  brandName: null,

  


  wiz: null,

  



  _runUnload: true,

  




  _submitTelemetry: function(aPageID) {
    AUSTLMY.pingWizLastPageCode(aPageID);
  },

  



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
                       nextFinishButtonString, canAdvance, showCancel) {
    this.wiz.canAdvance = canAdvance;

    var bnf = this.wiz.getButton(this.wiz.onLastPage ? "finish" : "next");
    var be1 = this.wiz.getButton("extra1");
    var be2 = this.wiz.getButton("extra2");
    var bc = this.wiz.getButton("cancel");

    
    this._setButton(bnf, nextFinishButtonString);
    this._setButton(be1, extra1ButtonString);
    this._setButton(be2, extra2ButtonString);

    bnf.hidden = bnf.disabled = !nextFinishButtonString;
    be1.hidden = be1.disabled = !extra1ButtonString;
    be2.hidden = be2.disabled = !extra2ButtonString;
    bc.hidden = bc.disabled = !showCancel;

    
    
    var btn = this.wiz.getButton("back");
    btn.hidden = btn.disabled = true;

    
    
    btn = this.wiz.getButton(this.wiz.onLastPage ? "next" : "finish");
    btn.hidden = btn.disabled = true;
  },

  getAUSString: function(key, strings) {
    if (strings)
      return this.strings.getFormattedString(key, strings);
    return this.strings.getString(key);
  },

  never: function () {
    
    
    
    var neverPrefName = PREFBRANCH_APP_UPDATE_NEVER + this.update.appVersion;
    Services.prefs.setBoolPref(neverPrefName, true);
  },

  



  _pages: { },

  



  onWizardFinish: function() {
    this._runUnload = false;
    var pageid = document.documentElement.currentPage.pageid;
    if ("onWizardFinish" in this._pages[pageid])
      this._pages[pageid].onWizardFinish();
    this._submitTelemetry(pageid);
  },

  



  onWizardCancel: function() {
    this._runUnload = false;
    var pageid = document.documentElement.currentPage.pageid;
    if ("onWizardCancel" in this._pages[pageid])
      this._pages[pageid].onWizardCancel();
    this._submitTelemetry(pageid);
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

  



  _cacheButtonStrings: function (buttonName) {
    var button = this.wiz.getButton(buttonName);
    button.defaultLabel = button.label;
    button.defaultAccesskey = button.getAttribute("accesskey");
  },

  


  onLoad: function() {
    this.wiz = document.documentElement;

    gLogEnabled = getPref("getBoolPref", PREF_APP_UPDATE_LOG, false);

    this.strings = document.getElementById("updateStrings");
    var brandStrings = document.getElementById("brandStrings");
    this.brandName = brandStrings.getString("brandShortName");

    var pages = this.wiz.childNodes;
    for (var i = 0; i < pages.length; ++i) {
      var page = pages[i];
      if (page.localName == "wizardpage")
        this._pages[page.pageid] = eval(page.getAttribute("object"));
    }

    
    this._cacheButtonStrings("next");
    this._cacheButtonStrings("finish");
    this._cacheButtonStrings("extra1");
    this._cacheButtonStrings("extra2");

    
    this.getStartPageID(function(startPageID) {
      LOG("gUpdates", "onLoad - setting current page to startpage " + startPageID);
      gUpdates.wiz.currentPage = document.getElementById(startPageID);
    });
  },

  


  onUnload: function() {
    if (this._runUnload) {
      var cp = this.wiz.currentPage;
      if (cp.pageid != "finished" && cp.pageid != "finishedBackground")
        this.onWizardCancel();
    }
  },

  























  getStartPageID: function(aCallback) {
    if ("arguments" in window && window.arguments[0]) {
      var arg0 = window.arguments[0];
      if (arg0 instanceof CoI.nsIUpdate) {
        
        
        
        this.setUpdate(arg0);
        if (this.update.errorCode == CERT_ATTR_CHECK_FAILED_NO_UPDATE ||
            this.update.errorCode == CERT_ATTR_CHECK_FAILED_HAS_UPDATE ||
            this.update.errorCode == BACKGROUNDCHECK_MULTIPLE_FAILURES) {
          aCallback("errorextra");
          return;
        }

        if (this.update.unsupported) {
          aCallback("unsupported");
          return;
        }

        var p = this.update.selectedPatch;
        if (p) {
          let state = p.state;
          let patchFailed = this.update.getProperty("patchingFailed");
          if (patchFailed) {
            if (patchFailed == "partial" && this.update.patchCount == 2) {
              
              
              
              state = STATE_FAILED;
            }
            else {
              
              
              
              
              state = STATE_DOWNLOAD_FAILED;
            }
          }

          
          
          switch (state) {
            case STATE_PENDING:
            case STATE_PENDING_SVC:
            case STATE_APPLIED:
            case STATE_APPLIED_SVC:
              this.sourceEvent = SRCEVT_BACKGROUND;
              aCallback("finishedBackground");
              return;
            case STATE_DOWNLOADING:
              aCallback("downloading");
              return;
            case STATE_FAILED:
              window.getAttention();
              aCallback("errorpatching");
              return;
            case STATE_DOWNLOAD_FAILED:
            case STATE_APPLYING:
              aCallback("errors");
              return;
          }
        }
        if (this.update.licenseURL)
          this.wiz.getPageById(this.updatesFoundPageId).setAttribute("next", "license");

        var self = this;
        this.getShouldCheckAddonCompatibility(function(shouldCheck) {
          if (shouldCheck) {
            var incompatCheckPage = document.getElementById("incompatibleCheck");
            incompatCheckPage.setAttribute("next", self.updatesFoundPageId);
            aCallback(incompatCheckPage.id);
          }
          else {
            aCallback(self.updatesFoundPageId);
          }
        });
        return;
      }
      else if (arg0 == "installed") {
        aCallback("installed");
        return;
      }
    }
    else {
      var um = CoC["@mozilla.org/updates/update-manager;1"].
               getService(CoI.nsIUpdateManager);
      if (um.activeUpdate) {
        this.setUpdate(um.activeUpdate);
        aCallback("downloading");
        return;
      }
    }

    
    var billboardTestURL = getPref("getCharPref", PREF_APP_UPDATE_BILLBOARD_TEST_URL, null);
    if (billboardTestURL) {
      var updatesFoundBillboardPage = document.getElementById("updatesfoundbillboard");
      updatesFoundBillboardPage.setAttribute("next", "dummy");
      gUpdatesFoundBillboardPage.onExtra1 = function(){ gUpdates.wiz.cancel(); };
      gUpdatesFoundBillboardPage.onExtra2 = function(){ gUpdates.wiz.cancel(); };
      this.onWizardNext = function() { gUpdates.wiz.cancel(); };
      this.update = { billboardURL        : billboardTestURL,
                      brandName           : this.brandName,
                      displayVersion      : "Billboard Test 1.0",
                      showNeverForVersion : true,
                      type                : "major" };
      aCallback(updatesFoundBillboardPage.id);
    }
    else {
      aCallback("checking");
    }
  },

  getShouldCheckAddonCompatibility: function(aCallback) {
    
    if (!this.update) {
      aCallback(false);
      return;
    }

    if (!this.update.appVersion ||
        Services.vc.compare(this.update.appVersion, Services.appinfo.version) == 0) {
      aCallback(false);
      return;
    }

    try {
      var hotfixID = Services.prefs.getCharPref(PREF_EM_HOTFIX_ID);
    }
    catch (e) { }

    var self = this;
    AddonManager.getAllAddons(function(addons) {
      self.addons = [];
      addons.forEach(function(addon) {
        
        
        if (!("isCompatibleWith" in addon) || !("findUpdates" in addon)) {
          let errMsg = "Add-on doesn't implement either the isCompatibleWith " +
                       "or the findUpdates method!";
          if (addon.id)
            errMsg += " Add-on ID: " + addon.id;
          CoU.reportError(errMsg);
          return;
        }

        
        
        
        
        
        
        
        
        
        
        try {
          if (addon.type != "plugin" && addon.id != hotfixID &&
              !addon.appDisabled && !addon.userDisabled &&
              addon.scope != AddonManager.SCOPE_APPLICATION &&
              addon.isCompatible &&
              !addon.isCompatibleWith(self.update.appVersion,
                                      self.update.platformVersion))
            self.addons.push(addon);
        }
        catch (e) {
          CoU.reportError(e);
        }
      });

      aCallback(self.addons.length != 0);
    });
  },

  



  get updatesFoundPageId() {
    if (gUpdatesFoundPageId)
      return gUpdatesFoundPageId;
    return gUpdatesFoundPageId = this.update.billboardURL ? "updatesfoundbillboard"
                                                          : "updatesfoundbasic";
  },

  




  setUpdate: function(update) {
    this.update = update;
    if (this.update)
      this.update.QueryInterface(CoI.nsIWritablePropertyBag);
  }
};





var gCheckingPage = {
  



  _checker: null,

  


  onPageShow: function() {
    gUpdates.setButtons(null, null, null, false, true);
    gUpdates.wiz.getButton("cancel").focus();

    
    
    
    
    Services.prefs.deleteBranch(PREFBRANCH_APP_UPDATE_NEVER);

    
    
    if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_BACKGROUNDERRORS))
      Services.prefs.clearUserPref(PREF_APP_UPDATE_BACKGROUNDERRORS);

    
    
    if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED))
      Services.prefs.clearUserPref(PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED);

    this._checker = CoC["@mozilla.org/updates/update-checker;1"].
                    createInstance(CoI.nsIUpdateChecker);
    this._checker.checkForUpdates(this.updateListener, true);
  },

  



  onWizardCancel: function() {
    this._checker.stopChecking(CoI.nsIUpdateChecker.CURRENT_CHECK);
  },

  



  updateListener: {
    


    onCheckComplete: function(request, updates, updateCount) {
      var aus = CoC["@mozilla.org/updates/update-service;1"].
                getService(CoI.nsIApplicationUpdateService);
      gUpdates.setUpdate(aus.selectUpdate(updates, updates.length));
      if (gUpdates.update) {
        LOG("gCheckingPage", "onCheckComplete - update found");
        if (gUpdates.update.unsupported) {
          gUpdates.wiz.goTo("unsupported");
          return;
        }

        if (!aus.canApplyUpdates) {
          
          
          gUpdates.never();
          gUpdates.wiz.goTo("manualUpdate");
          return;
        }

        if (gUpdates.update.licenseURL) {
          
          
          gUpdates.wiz.getPageById(gUpdates.updatesFoundPageId).setAttribute("next", "license");
        }

        gUpdates.getShouldCheckAddonCompatibility(function(shouldCheck) {
          if (shouldCheck) {
            var incompatCheckPage = document.getElementById("incompatibleCheck");
            incompatCheckPage.setAttribute("next", gUpdates.updatesFoundPageId);
            gUpdates.wiz.goTo("incompatibleCheck");
          }
          else {
            gUpdates.wiz.goTo(gUpdates.updatesFoundPageId);
          }
        });
        return;
      }

      LOG("gCheckingPage", "onCheckComplete - no update found");
      gUpdates.wiz.goTo("noupdatesfound");
    },

    


    onError: function(request, update) {
      LOG("gCheckingPage", "onError - proceeding to error page");
      gUpdates.setUpdate(update);
      if (update.errorCode &&
          (update.errorCode == CERT_ATTR_CHECK_FAILED_NO_UPDATE ||
           update.errorCode == CERT_ATTR_CHECK_FAILED_HAS_UPDATE)) {
        gUpdates.wiz.goTo("errorextra");
      }
      else {
        gUpdates.wiz.goTo("errors");
      }
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
    var prefs = Services.prefs;
    if (prefs.getPrefType(PREF_PLUGINS_UPDATE_URL) == prefs.PREF_INVALID) {
      gUpdates.wiz.goTo("noupdatesfound");
      return;
    }

    this._url = Services.urlFormatter.formatURLPref(PREF_PLUGINS_UPDATE_URL);
    var link = document.getElementById("pluginupdateslink");
    link.setAttribute("href", this._url);


    var phs = CoC["@mozilla.org/plugin/host;1"].
                 getService(CoI.nsIPluginHost);
    var plugins = phs.getPluginTags();
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

    if (getPref("getBoolPref", PREF_APP_UPDATE_ENABLED, true))
      document.getElementById("noUpdatesAutoEnabled").hidden = false;
    else
      document.getElementById("noUpdatesAutoDisabled").hidden = false;

    gUpdates.setButtons(null, null, "okButton", true);
    gUpdates.wiz.getButton("finish").focus();
  }
};





var gIncompatibleCheckPage = {
  


  _totalCount: 0,

  


  _completedCount: 0,

  


  _pBar: null,

  


  onPageShow: function() {
    LOG("gIncompatibleCheckPage", "onPageShow - checking for updates to " +
        "incompatible add-ons");

    gUpdates.setButtons(null, null, null, false, true);
    gUpdates.wiz.getButton("cancel").focus();
    this._pBar = document.getElementById("incompatibleCheckProgress");
    this._totalCount = gUpdates.addons.length;

    this._pBar.mode = "normal";
    gUpdates.addons.forEach(function(addon) {
      addon.findUpdates(this, AddonManager.UPDATE_WHEN_NEW_APP_DETECTED,
                        gUpdates.update.appVersion,
                        gUpdates.update.platformVersion);
    }, this);
  },

  
  onCompatibilityUpdateAvailable: function(addon) {
    
    
    for (var i = 0; i < gUpdates.addons.length; ++i) {
      if (gUpdates.addons[i].id == addon.id) {
        LOG("gIncompatibleCheckPage", "onCompatibilityUpdateAvailable - " +
            "found update for add-on ID: " + addon.id);
        gUpdates.addons.splice(i, 1);
        break;
      }
    }
  },

  onUpdateAvailable: function(addon, install) {
    
    
    
    let bs = CoC["@mozilla.org/extensions/blocklist;1"].
             getService(CoI.nsIBlocklistService);
    if (bs.isAddonBlocklisted(addon,
                              gUpdates.update.appVersion,
                              gUpdates.update.platformVersion))
      return;

    
    this.onCompatibilityUpdateAvailable(addon);
  },

  onUpdateFinished: function(addon) {
    ++this._completedCount;
    this._pBar.value = Math.ceil((this._completedCount / this._totalCount) * 100);

    if (this._completedCount < this._totalCount)
      return;

    if (gUpdates.addons.length == 0) {
      LOG("gIncompatibleCheckPage", "onUpdateFinished - updates were found " +
          "for all incompatible add-ons");
    }
    else {
      LOG("gIncompatibleCheckPage", "onUpdateFinished - there are still " +
          "incompatible add-ons");
      if (gUpdates.update.licenseURL) {
        document.getElementById("license").setAttribute("next", "incompatibleList");
      }
      else {
        
        
        gUpdates.wiz.getPageById(gUpdates.updatesFoundPageId).setAttribute("next", "incompatibleList");
      }
    }
    gUpdates.wiz.goTo(gUpdates.updatesFoundPageId);
  }
};





var gManualUpdatePage = {
  onPageShow: function() {
    var manualURL = Services.urlFormatter.formatURLPref(PREF_APP_UPDATE_URL_MANUAL);
    var manualUpdateLinkLabel = document.getElementById("manualUpdateLinkLabel");
    manualUpdateLinkLabel.value = manualURL;
    manualUpdateLinkLabel.setAttribute("url", manualURL);

    gUpdates.setButtons(null, null, "okButton", true);
    gUpdates.wiz.getButton("finish").focus();
  }
};





var gUnsupportedPage = {
  onPageShow: function() {
    Services.prefs.setBoolPref(PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED, true);
    if (gUpdates.update.detailsURL) {
      let unsupportedLinkLabel = document.getElementById("unsupportedLinkLabel");
      unsupportedLinkLabel.setAttribute("url", gUpdates.update.detailsURL);
    }

    gUpdates.setButtons(null, null, "okButton", true);
    gUpdates.wiz.getButton("finish").focus();
  }
};





var gUpdatesFoundBasicPage = {
  


  onPageShow: function() {
    gUpdates.wiz.canRewind = false;
    var update = gUpdates.update;
    gUpdates.setButtons("askLaterButton",
                        update.showNeverForVersion ? "noThanksButton" : null,
                        "updateButton_" + update.type, true);
    var btn = gUpdates.wiz.getButton("next");
    btn.focus();

    var updateName = update.name;
    if (update.channel == "nightly") {
      updateName = gUpdates.getAUSString("updateNightlyName",
                                         [gUpdates.brandName,
                                          update.displayVersion,
                                          update.buildID]);
    }
    var updateNameElement = document.getElementById("updateName");
    updateNameElement.value = updateName;

    var introText = gUpdates.getAUSString("intro_" + update.type,
                                          [gUpdates.brandName, update.displayVersion]);
    var introElem = document.getElementById("updatesFoundInto");
    introElem.setAttribute("severity", update.type);
    introElem.textContent = introText;

    var updateMoreInfoURL = document.getElementById("updateMoreInfoURL");
    if (update.detailsURL)
      updateMoreInfoURL.setAttribute("url", update.detailsURL);
    else
      updateMoreInfoURL.hidden = true;

    var updateTitle = gUpdates.getAUSString("updatesfound_" + update.type +
                                            ".title");
    document.getElementById("updatesFoundBasicHeader").setAttribute("label", updateTitle);
  },

  onExtra1: function() {
    gUpdates.wiz.cancel();
  },

  onExtra2: function() {
    gUpdates.never();
    gUpdates.wiz.cancel();
  }
};





var gUpdatesFoundBillboardPage = {
  


  _billboardLoaded: false,

  


  onPageShow: function() {
    var update = gUpdates.update;
    gUpdates.setButtons("askLaterButton",
                        update.showNeverForVersion ? "noThanksButton" : null,
                        "updateButton_" + update.type, true);
    gUpdates.wiz.getButton("next").focus();

    if (this._billboardLoaded)
      return;

    var remoteContent = document.getElementById("updateMoreInfoContent");
    remoteContent.addEventListener("load",
                                   gUpdatesFoundBillboardPage.onBillboardLoad,
                                   false);
    
    
    
    remoteContent.update_name = gUpdates.brandName;
    remoteContent.update_version = update.displayVersion;

    var billboardTestURL = getPref("getCharPref", PREF_APP_UPDATE_BILLBOARD_TEST_URL, null);
    if (billboardTestURL) {
      
      
      var scheme = Services.io.newURI(billboardTestURL, null, null).scheme;
      if (scheme == "file")
        remoteContent.testFileUrl = update.billboardURL;
      else
        remoteContent.url = update.billboardURL;
    }
    else
      remoteContent.url = update.billboardURL;

    this._billboardLoaded = true;
  },

  


  onBillboardLoad: function(aEvent) {
    var remoteContent = document.getElementById("updateMoreInfoContent");
    
    var state = remoteContent.getAttribute("state");
    if (state == "loading" || aEvent.originalTarget != remoteContent)
      return;

    remoteContent.removeEventListener("load", gUpdatesFoundBillboardPage.onBillboardLoad, false);
    if (state == "error") {
      gUpdatesFoundPageId = "updatesfoundbasic";
      var next = gUpdates.wiz.getPageById("updatesfoundbillboard").getAttribute("next");
      gUpdates.wiz.getPageById(gUpdates.updatesFoundPageId).setAttribute("next", next);
      gUpdates.wiz.goTo(gUpdates.updatesFoundPageId);
    }
  },

  onExtra1: function() {
    this.onWizardCancel();
    gUpdates.wiz.cancel();
  },

  onExtra2: function() {
    this.onWizardCancel();
    gUpdates.never();
    gUpdates.wiz.cancel();
  },

  


  onWizardCancel: function() {
    try {
      var remoteContent = document.getElementById("updateMoreInfoContent");
      if (remoteContent)
        remoteContent.stopDownloading();
    }
    catch (e) {
      LOG("gUpdatesFoundBillboardPage", "onWizardCancel - " +
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
      licenseGroup.focus();
      return;
    }

    gUpdates.wiz.canAdvance = false;

    
    document.getElementById("acceptDeclineLicense").disabled = true;
    gUpdates.update.setProperty("licenseAccepted", "false");

    licenseContent.addEventListener("load", gLicensePage.onLicenseLoad, false);
    
    
    
    licenseContent.update_name = gUpdates.brandName;
    licenseContent.update_version = gUpdates.update.displayVersion;
    licenseContent.url = gUpdates.update.licenseURL;
  },

  


  onLicenseLoad: function(aEvent) {
    var licenseContent = document.getElementById("licenseContent");
    
    
    
    var state = licenseContent.getAttribute("state");
    if (state == "loading" || aEvent.originalTarget != licenseContent)
      return;

    licenseContent.removeEventListener("load", gLicensePage.onLicenseLoad, false);

    if (state == "error") {
      gUpdates.wiz.goTo("manualUpdate");
      return;
    }

    gLicensePage._licenseLoaded = true;
    document.getElementById("acceptDeclineLicense").disabled = false;
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
    gUpdates.wiz.goTo(gUpdates.updatesFoundPageId);
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
    var listbox = document.getElementById("incompatibleListbox");
    if (listbox.children.length > 0)
      return;

    var intro = gUpdates.getAUSString("incompatAddons_" + gUpdates.update.type,
                                      [gUpdates.brandName,
                                       gUpdates.update.displayVersion]);
    document.getElementById("incompatibleListDesc").textContent = intro;

    var addons = gUpdates.addons;
    for (var i = 0; i < addons.length; ++i) {
      var listitem = document.createElement("listitem");
      var addonLabel = gUpdates.getAUSString("addonLabel", [addons[i].name,
                                                            addons[i].version]);
      listitem.setAttribute("label", addonLabel);
      listbox.appendChild(listitem);
    }
  },

  


  onExtra1: function() {
    gUpdates.wiz.goTo(gUpdates.update.licenseURL ? "license"
                                                 : gUpdates.updatesFoundPageId);
  }
};





var gDownloadingPage = {
  


  _downloadStatus: null,
  _downloadProgress: null,
  _pauseButton: null,

  


  _paused: false,

  


  _label_downloadStatus: null,

  


  _lastSec: Infinity,
  _startTime: null,
  _pausedStatus: "",

  _hiding: false,

  


  _updateApplyingObserver: false,

  


  onPageShow: function() {
    this._downloadStatus = document.getElementById("downloadStatus");
    this._downloadProgress = document.getElementById("downloadProgress");
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

    this._startTime = Date.now();

    try {
      
      
      gUpdates.update.QueryInterface(CoI.nsIWritablePropertyBag);
      gUpdates.update.setProperty("foregroundDownload", "true");

      
      
      aus.pauseDownload();
      var state = aus.downloadUpdate(gUpdates.update, false);
      if (state == "failed") {
        
        
        
        
        this.cleanUp();
        gUpdates.wiz.goTo("errors");
        return;
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
      if (this._downloadProgress.mode != "normal")
        this._downloadProgress.mode = "normal";
      this._pauseButton.setAttribute("tooltiptext",
                                     gUpdates.getAUSString("pauseButtonResume"));
      this._pauseButton.setAttribute("paused", "true");
      var p = u.selectedPatch.QueryInterface(CoI.nsIPropertyBag);
      var status = p.getProperty("status");
      if (status) {
        let pausedStatus = gUpdates.getAUSString("downloadPausedStatus", [status]);
        this._setStatus(pausedStatus);
      }
    }
    else {
      if (this._downloadProgress.mode != "undetermined")
        this._downloadProgress.mode = "undetermined";
      this._pauseButton.setAttribute("paused", "false");
      this._pauseButton.setAttribute("tooltiptext",
                                     gUpdates.getAUSString("pauseButtonPause"));
      this._setStatus(this._label_downloadStatus);
    }
  },

  


  _setUpdateApplying: function() {
    this._downloadProgress.mode = "undetermined";
    this._pauseButton.hidden = true;
    let applyingStatus = gUpdates.getAUSString("applyingUpdate");
    this._setStatus(applyingStatus);

    Services.obs.addObserver(this, "update-staged", false);
    this._updateApplyingObserver = true;
  },

  


  cleanUp: function() {
    var aus = CoC["@mozilla.org/updates/update-service;1"].
              getService(CoI.nsIApplicationUpdateService);
    aus.removeDownloadListener(this);

    if (this._updateApplyingObserver) {
      Services.obs.removeObserver(this, "update-staged");
      this._updateApplyingObserver = false;
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

  



  onWizardCancel: function() {
    if (this._hiding)
      return;

    this.cleanUp();
  },

  


  onHide: function() {
    
    
    this._hiding = true;

    
    
    
    this.cleanUp();

    var aus = CoC["@mozilla.org/updates/update-service;1"].
              getService(CoI.nsIApplicationUpdateService);
    var um = CoC["@mozilla.org/updates/update-manager;1"].
             getService(CoI.nsIUpdateManager);
    um.activeUpdate = gUpdates.update;

    
    
    var downloadInBackground = true;
    if (this._paused) {
      var title = gUpdates.getAUSString("resumePausedAfterCloseTitle");
      var message = gUpdates.getAUSString("resumePausedAfterCloseMsg",
                                          [gUpdates.brandName]);
      var ps = Services.prompt;
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
    
    
    if (this._paused)
      return;

    if (this._downloadProgress.mode != "undetermined")
      this._downloadProgress.mode = "undetermined";
    this._setStatus(this._label_downloadStatus);
  },

  










  onProgress: function(request, context, progress, maxProgress) {
    let status = this._updateDownloadStatus(progress, maxProgress);
    var currentProgress = Math.round(100 * (progress / maxProgress));

    var p = gUpdates.update.selectedPatch;
    p.QueryInterface(CoI.nsIWritablePropertyBag);
    p.setProperty("progress", currentProgress);
    p.setProperty("status", status);

    
    
    if (this._paused)
      return;

    if (this._downloadProgress.mode != "normal")
      this._downloadProgress.mode = "normal";
    if (this._downloadProgress.value != currentProgress)
      this._downloadProgress.value = currentProgress;
    if (this._pauseButton.disabled)
      this._pauseButton.disabled = false;

    
    
    
    
    
    
    if (progress == maxProgress &&
        this._downloadStatus.textContent == this._label_downloadStatus)
      return;

    this._setStatus(status);
  },

  










  onStatus: function(request, context, status, statusText) {
    this._setStatus(statusText);
  },

  








  onStopRequest: function(request, context, status) {
    if (this._downloadProgress.mode != "normal")
      this._downloadProgress.mode = "normal";

    var u = gUpdates.update;
    switch (status) {
      case CoR.NS_ERROR_CORRUPTED_CONTENT:
      case CoR.NS_ERROR_UNEXPECTED:
        if (u.selectedPatch.state == STATE_DOWNLOAD_FAILED &&
            (u.isCompleteUpdate || u.patchCount != 2)) {
          
          
          this.cleanUp();
          gUpdates.wiz.goTo("errors");
          break;
        }
        
        

        
        
        this._downloadProgress.mode = "undetermined";
        this._pauseButton.disabled = true;
        document.getElementById("verificationFailed").hidden = false;
        break;
      case CoR.NS_BINDING_ABORTED:
        LOG("gDownloadingPage", "onStopRequest - pausing download");
        
        break;
      case CoR.NS_OK:
        LOG("gDownloadingPage", "onStopRequest - patch verification succeeded");
        
        
        let aus = CoC["@mozilla.org/updates/update-service;1"].
                  getService(CoI.nsIApplicationUpdateService);
        if (aus.canStageUpdates) {
          this._setUpdateApplying();
        } else {
          this.cleanUp();
          gUpdates.wiz.goTo("finished");
        }
        break;
      default:
        LOG("gDownloadingPage", "onStopRequest - transfer failed");
        
        this.cleanUp();
        gUpdates.wiz.goTo("errors");
        break;
    }
  },

  


  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "update-staged") {
      if (aData == STATE_DOWNLOADING) {
        
        
        this._setStatus("downloading");
        return;
      }
      this.cleanUp();
      if (aData == STATE_APPLIED ||
          aData == STATE_APPLIED_SVC ||
          aData == STATE_PENDING ||
          aData == STATE_PENDING_SVC) {
        
        
        gUpdates.wiz.goTo("finished");
      } else {
        gUpdates.wiz.goTo("errors");
      }
    }
  },

  


  QueryInterface: function(iid) {
    if (!iid.equals(CoI.nsIRequestObserver) &&
        !iid.equals(CoI.nsIProgressEventSink) &&
        !iid.equals(CoI.nsIObserver) &&
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
    var manualURL = Services.urlFormatter.formatURLPref(PREF_APP_UPDATE_URL_MANUAL);
    var errorLinkLabel = document.getElementById("errorLinkLabel");
    errorLinkLabel.value = manualURL;
    errorLinkLabel.setAttribute("url", manualURL);
  }
};





var gErrorExtraPage = {
  


  onPageShow: function() {
    gUpdates.setButtons(null, null, "okButton", true);
    gUpdates.wiz.getButton("finish").focus();
    let secHistogram = CoC["@mozilla.org/base/telemetry;1"].
                                  getService(CoI.nsITelemetry).
                                  getHistogramById("SECURITY_UI");

    if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_CERT_ERRORS))
      Services.prefs.clearUserPref(PREF_APP_UPDATE_CERT_ERRORS);

    if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_BACKGROUNDERRORS))
      Services.prefs.clearUserPref(PREF_APP_UPDATE_BACKGROUNDERRORS);

    if (gUpdates.update.errorCode == CERT_ATTR_CHECK_FAILED_HAS_UPDATE) {
      document.getElementById("errorCertAttrHasUpdateLabel").hidden = false;
      secHistogram.add(CoI.nsISecurityUITelemetry.WARNING_INSECURE_UPDATE);
    }
    else {
      if (gUpdates.update.errorCode == CERT_ATTR_CHECK_FAILED_NO_UPDATE){
        document.getElementById("errorCertCheckNoUpdateLabel").hidden = false;
        secHistogram.add(CoI.nsISecurityUITelemetry.WARNING_NO_SECURE_UPDATE);
      }
      else
        document.getElementById("genericBackgroundErrorLabel").hidden = false;
      var manualURL = Services.urlFormatter.formatURLPref(PREF_APP_UPDATE_URL_MANUAL);
      var errorLinkLabel = document.getElementById("errorExtraLinkLabel");
      errorLinkLabel.value = manualURL;
      errorLinkLabel.setAttribute("url", manualURL);
      errorLinkLabel.hidden = false;
    }
  }
};




var gErrorPatchingPage = {
  


  onPageShow: function() {
    gUpdates.setButtons(null, null, "okButton", true);
  },

  onWizardNext: function() {
    switch (gUpdates.update.selectedPatch.state) {
      case STATE_PENDING:
      case STATE_PENDING_SVC:
        gUpdates.wiz.goTo("finished");
        break;
      case STATE_DOWNLOADING:
        gUpdates.wiz.goTo("downloading");
        break;
      case STATE_DOWNLOAD_FAILED:
        gUpdates.wiz.goTo("errors");
        break;
    }
  }
};





var gFinishedPage = {
  


  onPageShow: function() {
    gUpdates.setButtons("restartLaterButton", null, "restartNowButton",
                        true);
    gUpdates.wiz.getButton("finish").focus();
  },

  


  onPageShowBackground: function() {
    this.onPageShow();
    var updateFinishedName = document.getElementById("updateFinishedName");
    updateFinishedName.value = gUpdates.update.name;

    var link = document.getElementById("finishedBackgroundLink");
    if (gUpdates.update.detailsURL) {
      link.setAttribute("url", gUpdates.update.detailsURL);
      
      
      link.disabled = false;
    }
    else
      link.hidden = true;

    if (getPref("getBoolPref", PREF_APP_UPDATE_TEST_LOOP, false)) {
      setTimeout(function () {
                   gUpdates.wiz.getButton("finish").click();
                 }, UPDATE_TEST_LOOP_INTERVAL);
    }
  },

  



  onWizardFinish: function() {
    
    LOG("gFinishedPage" , "onWizardFinish - restarting the application");

    
    
    
    
    
    
    
    
    
    gUpdates.wiz.getButton("finish").disabled = true;
    gUpdates.wiz.getButton("extra1").disabled = true;

    
    var cancelQuit = CoC["@mozilla.org/supports-PRBool;1"].
                     createInstance(CoI.nsISupportsPRBool);
    Services.obs.notifyObservers(cancelQuit, "quit-application-requested",
                                 "restart");

    
    if (cancelQuit.data)
      return;

    
    if (Services.appinfo.inSafeMode) {
      let env = CoC["@mozilla.org/process/environment;1"].
                getService(CoI.nsIEnvironment);
      env.set("MOZ_SAFE_MODE_RESTART", "1");
    }

    
    CoC["@mozilla.org/toolkit/app-startup;1"].getService(CoI.nsIAppStartup).
    quit(CoI.nsIAppStartup.eAttemptQuit | CoI.nsIAppStartup.eRestart);
  },

  



  onExtra1: function() {
    gUpdates.wiz.cancel();
  }
};




var gInstalledPage = {
  


  onPageShow: function() {
    var branding = document.getElementById("brandStrings");
    try {
      
      var url = branding.getFormattedString("whatsNewURL", [Services.appinfo.version]);
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







function setCurrentPage(pageid) {
  gUpdates.wiz.currentPage = document.getElementById(pageid);
}
