






































const nsIUpdateItem           = Components.interfaces.nsIUpdateItem;
const nsIIncrementalDownload  = Components.interfaces.nsIIncrementalDownload;

const XMLNS_XUL               = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

const PREF_UPDATE_MANUAL_URL        = "app.update.url.manual";
const PREF_UPDATE_NAGTIMER_DL       = "app.update.nagTimer.download";
const PREF_UPDATE_NAGTIMER_RESTART  = "app.update.nagTimer.restart";
const PREF_APP_UPDATE_LOG_BRANCH    = "app.update.log.";
const PREF_UPDATE_TEST_LOOP         = "app.update.test.loop";
const PREF_UPDATE_NEVER_BRANCH      = "app.update.never.";

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
    dump("*** " + module + ":" + string + "\n");
    gConsole.logStringMessage(string);
  }
}













function getPref(func, preference, defaultValue) {
  try {
    return gPref[func](preference);
  }
  catch (e) {
    LOG("General", "Failed to get preference " + preference);
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
      var label = this.strings.getString(string);
      if (label.indexOf("%S") != -1)
        label = label.replace(/%S/, this.brandName);
      button.label = label;
      button.setAttribute("accesskey",
                          this.strings.getString(string + ".accesskey"));
    } else {
      button.label = button.defaultLabel;
      button.setAttribute("accesskey", button.defaultAccesskey);
    }
  },
  
  





































  setButtons: function(backButtonString, backButtonDisabled,
                       nextButtonString, nextButtonDisabled,
                       finishButtonString, finishButtonDisabled,
                       cancelButtonString, cancelButtonDisabled,
                       hideBackAndCancelButtons,
                       extraButton1String, extraButton1Disabled,
                       extraButton2String, extraButton2Disabled) {
    var bb = this.wiz.getButton("back");
    var bn = this.wiz.getButton("next");
    var bf = this.wiz.getButton("finish");
    var bc = this.wiz.getButton("cancel");
    var be1 = this.wiz.getButton("extra1");
    var be2 = this.wiz.getButton("extra2");

    this._setButton(bb, backButtonString);
    this._setButton(bn, nextButtonString);
    this._setButton(bf, finishButtonString);
    this._setButton(bc, cancelButtonString);
    this._setButton(be1, extraButton1String);
    this._setButton(be2, extraButton2String);

    
    this.wiz.canRewind  = !backButtonDisabled;
    
    if (this.wiz.onLastPage)
      this.wiz.canAdvance = !finishButtonDisabled;
    else
      this.wiz.canAdvance = !nextButtonDisabled;

    bf.disabled = finishButtonDisabled;
    bc.disabled = cancelButtonDisabled;
    be1.disabled = extraButton1Disabled;
    be2.disabled = extraButton2Disabled;

    
    
    bc.hidden   = hideBackAndCancelButtons;  
    bb.hidden   = hideBackAndCancelButtons;

    
    be1.hidden = extraButton1String == null;
    be2.hidden = extraButton2String == null;
  },
  
  never: function () {
    
    
    
    
    
    
    
    
    var neverPrefName = PREF_UPDATE_NEVER_BRANCH + encodeURIComponent(gUpdates.update.version);
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
    
    gPref = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefBranch2);
    gConsole = Components.classes["@mozilla.org/consoleservice;1"]
                         .getService(Components.interfaces.nsIConsoleService);  
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
  
    
    this._cacheButtonStrings("back");
    this._cacheButtonStrings("next");
    this._cacheButtonStrings("finish");
    this._cacheButtonStrings("cancel");
    this._cacheButtonStrings("extra1");
    this._cacheButtonStrings("extra2");
    
    
    gUpdates.wiz.currentPage = this.startPage;
  },

  



  _initLoggingPrefs: function() {
    try {
      var ps = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefService);
      var logBranch = ps.getBranch(PREF_APP_UPDATE_LOG_BRANCH);
      var modules = logBranch.getChildList("", { value: 0 });

      for (var i = 0; i < modules.length; ++i) {
        if (logBranch.prefHasUserValue(modules[i]))
          gLogEnabled[modules[i]] = logBranch.getBoolPref(modules[i]);
      }
    }
    catch (e) {
    }
  },
    
  













  get startPage() {
    if (window.arguments) {
      var arg0 = window.arguments[0];
      if (arg0 instanceof Components.interfaces.nsIUpdate) {
        
        
        
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
          case STATE_SUCCEEDED:
            return document.getElementById("installed");
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
        return document.getElementById("updatesfound");
      }
    }
    else {
      var um = 
          Components.classes["@mozilla.org/updates/update-manager;1"].
          getService(Components.interfaces.nsIUpdateManager);
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
      this.update.QueryInterface(Components.interfaces.nsIWritablePropertyBag);
  },
  
  








  registerNagTimer: function(timerID, timerInterval, methodName) {
    
    var tm = 
        Components.classes["@mozilla.org/updates/timer-manager;1"].
        getService(Components.interfaces.nsIUpdateTimerManager);
    
    










    function Callback(update, methodName) {
      this._update = update;
      this._methodName = methodName;
      this._prompter = 
        Components.classes["@mozilla.org/updates/update-prompt;1"].
        createInstance(Components.interfaces.nsIUpdatePrompt);      
    }
    Callback.prototype = {
      


      _update: null,
      
      


      _prompter: null,
      
      


      _methodName: "",
      
      




      notify: function(timerCallback) {
        if (methodName in this._prompter) 
          this._prompter[methodName](null, this._update);
      }
    }
    tm.registerTimer(timerID, (new Callback(gUpdates.update, methodName)), 
                     timerInterval);
  }
}





var gCheckingPage = {
  



  _checker: null,
  
  


  onPageShow: function() {
    gUpdates.setButtons(null, true, null, true, null, true, 
                        null, false, false, null, 
                        false, null, false);
    this._checker = 
      Components.classes["@mozilla.org/updates/update-checker;1"].
      createInstance(Components.interfaces.nsIUpdateChecker);
    this._checker.checkForUpdates(this.updateListener, true);
  },
  
  



  onWizardCancel: function() {
    if (this._checker) {
      const nsIUpdateChecker = Components.interfaces.nsIUpdateChecker;
      this._checker.stopChecking(nsIUpdateChecker.CURRENT_CHECK);
    }
  },
  
  



  updateListener: {
    


    onProgress: function(request, position, totalSize) {
      var pm = document.getElementById("checkingProgress");
      checkingProgress.setAttribute("mode", "normal");
      checkingProgress.setAttribute("value", Math.floor(100 * (position/totalSize)));
    },

    


    onCheckComplete: function(request, updates, updateCount) {
      var aus = Components.classes["@mozilla.org/updates/update-service;1"]
                          .getService(Components.interfaces.nsIApplicationUpdateService);
      gUpdates.setUpdate(aus.selectUpdate(updates, updates.length));
      if (!gUpdates.update) {
        LOG("UI:CheckingPage", 
            "Could not select an appropriate update, either because there " + 
            "were none, or |selectUpdate| failed.");
        var checking = document.getElementById("checking");
        checking.setAttribute("next", "noupdatesfound");
      }
      gUpdates.wiz.canAdvance = true;
      gUpdates.wiz.advance();
    },

    


    onError: function(request, update) {
      LOG("UI:CheckingPage", "UpdateCheckListener: error");

      gUpdates.setUpdate(update);

      gUpdates.wiz.currentPage = document.getElementById("errors");
    },
    
    


    QueryInterface: function(aIID) {
      if (!aIID.equals(Components.interfaces.nsIUpdateCheckListener) &&
          !aIID.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
    }
  }
};




var gNoUpdatesPage = {
  


  onPageShow: function() {
    gUpdates.setButtons(null, true, null, true, null, false, "hideButton",
                        true, false, null, false, null, false);
    gUpdates.wiz.getButton("finish").focus();
  }
};






var gUpdatesAvailablePage = {
  


  _incompatibleItems: null,

  


  _updateMoreInfoContent: null,
  
  


  onPageShow: function() {
    
    
    
    
    
    
    gUpdates.wiz.getButton("next").disabled = true;
    gUpdates.wiz.getButton("back").disabled = true;

    var updateName = gUpdates.strings.getFormattedString("updateName", 
      [gUpdates.brandName, gUpdates.update.version]);
    if (gUpdates.update.channel == "nightly")
      updateName = updateName + " nightly (" + gUpdates.update.buildID + ")";
    var updateNameElement = document.getElementById("updateName");
    updateNameElement.value = updateName;
    var severity = gUpdates.update.type;
    var updateTypeElement = document.getElementById("updateType");
    updateTypeElement.setAttribute("severity", severity);
 
    var intro;
    if (severity == "major") {
      
      intro = gUpdates.strings.getFormattedString(
        "introType_major_app_and_version", 
        [gUpdates.brandName, gUpdates.update.version]);

      this._updateMoreInfoContent = 
        document.getElementById("updateMoreInfoContent");

      
      
      
      this._updateMoreInfoContent.update_name = gUpdates.brandName;
      this._updateMoreInfoContent.update_version = gUpdates.update.version;
      this._updateMoreInfoContent.url = gUpdates.update.detailsURL;
    }
    else {
      
      
      intro = gUpdates.strings.getFormattedString(
        "introType_minor_app", [gUpdates.brandName]);

      var updateMoreInfoURL = document.getElementById("updateMoreInfoURL");
      updateMoreInfoURL.href = gUpdates.update.detailsURL; 
    }

    var updateTitle = gUpdates.strings
                              .getString("updatesfound_" + severity + ".title");
    gUpdates.wiz.currentPage.setAttribute("label", updateTitle);
    
    
    gUpdates.wiz._adjustWizardHeader();

    while (updateTypeElement.hasChildNodes())
      updateTypeElement.removeChild(updateTypeElement.firstChild);
    updateTypeElement.appendChild(document.createTextNode(intro));
    
    var em = Components.classes["@mozilla.org/extensions/manager;1"]
                       .getService(Components.interfaces.nsIExtensionManager);
    var items = em.getIncompatibleItemList("", gUpdates.update.version,
                                           nsIUpdateItem.TYPE_ADDON, false, 
                                           { });
    if (items.length > 0) {
      
      
      var incompatibleWarning = document.getElementById("incompatibleWarning");
      incompatibleWarning.hidden = false;
      
      this._incompatibleItems = items;
    }
    
    
    
    this.onShowMoreDetails();

    var licenseAccepted;
    try {
      licenseAccepted = gUpdates.update.getProperty("licenseAccepted");
    }
    catch (e) {
      gUpdates.update.setProperty("licenseAccepted", "false");
      licenseAccepted = false;
    }
    
    
    if (gUpdates.update.type == "major" &&
        gUpdates.update.licenseURL && !licenseAccepted)
      gUpdates.wiz.currentPage.setAttribute("next", "license");

    gUpdates.setButtons(null, true, "downloadButton_" + severity, 
                        false, null, false,
                        null, false, true, 
                        "laterButton", false, 
                        severity == "major" ? "neverButton" : null, false);
    gUpdates.wiz.getButton("next").focus();
  },
  
  


  onShowMoreDetails: function() {
    var updateTypeElement = document.getElementById("updateType");
    var moreInfoURL = document.getElementById("moreInfoURL");
    var moreInfoContent = document.getElementById("moreInfoContent");

    if (updateTypeElement.getAttribute("severity") == "major") {
      moreInfoURL.hidden = true;
      moreInfoContent.hidden = false;
      document.getElementById("updateName").hidden = true;
      document.getElementById("updateNameSep").hidden = true;
      document.getElementById("upgradeEvangelism").hidden = true;
      document.getElementById("upgradeEvangelismSep").hidden = true;

      
      
      
      
      
      
      
      
      
      var neverPrefName = PREF_UPDATE_NEVER_BRANCH + encodeURIComponent(gUpdates.update.version);
      gPref.setBoolPref(neverPrefName, false);
    }
    else {
      moreInfoURL.hidden = false;
      moreInfoContent.hidden = true;
    }

    
    
    
    var detailsDeck = document.getElementById("detailsDeck");
    detailsDeck.selectedIndex = 1;
  },

  


  onWizardCancel: function() {
    try {
      
      if (this._updateMoreInfoContent)
        this._updateMoreInfoContent.stopDownloading();
    }
    catch (ex) {
      dump("XXX _updateMoreInfoContent.stopDownloading() failed: " + ex + "\n");
    }
  },

  


  showIncompatibleItems: function() {
    openDialog("chrome://mozapps/content/update/incompatible.xul", "", 
               "dialog,centerscreen,modal,resizable,titlebar", this._incompatibleItems);
  }
};






var gLicensePage = {
  


  _licenseContent: null,

  


  onPageShow: function() {
    this._licenseContent = document.getElementById("licenseContent");
    
    
    document.getElementById("acceptDeclineLicense").disabled = true;

    gUpdates.setButtons(null, true, null, true, null, true, null, 
                        false, false, null, false, null, false);

    this._licenseContent.addEventListener("load", this.onLicenseLoad, false);
    
    
    
    this._licenseContent.update_name = gUpdates.brandName;
    this._licenseContent.update_version = gUpdates.update.version;
    this._licenseContent.url = gUpdates.update.licenseURL;
  },
  
  


  onLicenseLoad: function() {
    
    
    
    document.getElementById("acceptDeclineLicense").disabled =
      (gLicensePage._licenseContent.getAttribute("state") == "error");
  },

  


  onAcceptDeclineRadio: function() {
    var selectedIndex = document.getElementById("acceptDeclineLicense")
                                .selectedIndex;
    
    var licenseAccepted = (selectedIndex == 0);
    gUpdates.wiz.getButton("next").disabled = !licenseAccepted;
    gUpdates.wiz.canAdvance = licenseAccepted;
  },
  
  


  onWizardNext: function() {
    try {
      gUpdates.update.setProperty("licenseAccepted", "true");
      var um = 
        Components.classes["@mozilla.org/updates/update-manager;1"].
        getService(Components.interfaces.nsIUpdateManager);
      um.saveUpdates();
    }
    catch (ex) {
      dump("XXX ex " + ex + "\n");
    }
  },
  
  


  onWizardCancel: function() {
    try {
      
      if (this._licenseContent)
        this._licenseContent.stopDownloading();
    }
    catch (ex) {
      dump("XXX _licenseContent.stopDownloading() failed: " + ex + "\n");
    }
  }
};






function DownloadStatusFormatter() {
  this._startTime = Math.floor((new Date()).getTime() / 1000);
  this._elapsed = 0;
  
  var us = gUpdates.strings;
  this._statusFormat = us.getString("statusFormat");

  this._progressFormat = us.getString("progressFormat");
  this._progressFormatKBMB = us.getString("progressFormatKBMB");
  this._progressFormatKBKB = us.getString("progressFormatKBKB");
  this._progressFormatMBMB = us.getString("progressFormatMBMB");
  this._progressFormatUnknownMB = us.getString("progressFormatUnknownMB");
  this._progressFormatUnknownKB = us.getString("progressFormatUnknownKB");

  this._rateFormat = us.getString("rateFormat");
  this._rateFormatKBSec = us.getString("rateFormatKBSec");
  this._rateFormatMBSec = us.getString("rateFormatMBSec");

  this._timeFormat = us.getString("timeFormat");
  this._longTimeFormat = us.getString("longTimeFormat");
  this._shortTimeFormat = us.getString("shortTimeFormat");

  this._remain = us.getString("remain");
  this._unknownFilesize = us.getString("unknownFilesize");
}
DownloadStatusFormatter.prototype = {
  


  _startTime: 0,

  


  _elapsed: -1,
  
  


  _rate: 0,
  
  


  _rateFormatted: "",
  
  


  _rateFormattedContainer: "",
  
  



  progress: "",

  









  formatStatus: function(currSize, finalSize) {
    var now = Math.floor((new Date()).getTime() / 1000);
    
    
    var total = parseInt(finalSize/1024 + 0.5);
    this.progress = this._formatKBytes(parseInt(currSize/1024 + 0.5), total);
    
    var progress = this._replaceInsert(this._progressFormat, 1, this.progress);
    var rateFormatted = "";
    
    
    var oldElapsed = this._elapsed;
    this._elapsed = now - this._startTime;
    if (oldElapsed != this._elapsed) {
      this._rate = this._elapsed ? Math.floor((currSize / 1024) / this._elapsed) : 0;
      var isKB = true;
      if (parseInt(this._rate / 1024) > 0) {
        this._rate = (this._rate / 1024).toFixed(1);
        isKB = false;
      }
      if (this._rate > 100)
        this._rate = Math.round(this._rate);
      
      if (this._rate) {
        var format = isKB ? this._rateFormatKBSec : this._rateFormatMBSec;
        this._rateFormatted = this._replaceInsert(format, 1, this._rate);
        this._rateFormattedContainer = this._replaceInsert(" " + this._rateFormat, 1, this._rateFormatted);
      }
    }
    progress = this._replaceInsert(progress, 2, this._rateFormattedContainer);
    

    
    var remainingTime = "";
    if (this._rate && (finalSize > 0)) {
      remainingTime = Math.floor(((finalSize - currSize) / 1024) / this._rate);
      remainingTime = this._formatSeconds(remainingTime);
      remainingTime = this._replaceInsert(this._timeFormat, 1, remainingTime)
      remainingTime = this._replaceInsert(remainingTime, 2, this._remain);
    }
    
    
    
    
    
    
    
    
    
    
    
    
    var status = this._statusFormat;
    status = this._replaceInsert(status, 1, progress);
    status = this._replaceInsert(status, 2, remainingTime);
    return status;
  },

  










  
  _replaceInsert: function(format, index, value) {
    return format.replace(new RegExp("#" + index), value);
  },

  













  _formatKBytes: function(currentKB, totalKB) {
    var progressHasMB = parseInt(currentKB / 1024) > 0;
    var totalHasMB = parseInt(totalKB / 1024) > 0;
    
    var format = "";
    if (!progressHasMB && !totalHasMB) {
      if (!totalKB) {
        format = this._progressFormatUnknownKB;
        format = this._replaceInsert(format, 1, currentKB);
      } else {
        format = this._progressFormatKBKB;
        format = this._replaceInsert(format, 1, currentKB);
        format = this._replaceInsert(format, 2, totalKB);
      }
    }
    else if (progressHasMB && totalHasMB) {
      format = this._progressFormatMBMB;
      format = this._replaceInsert(format, 1, (currentKB / 1024).toFixed(1));
      format = this._replaceInsert(format, 2, (totalKB / 1024).toFixed(1));
    }
    else if (totalHasMB && !progressHasMB) {
      format = this._progressFormatKBMB;
      format = this._replaceInsert(format, 1, currentKB);
      format = this._replaceInsert(format, 2, (totalKB / 1024).toFixed(1));
    }
    else if (progressHasMB && !totalHasMB) {
      format = this._progressFormatUnknownMB;
      format = this._replaceInsert(format, 1, (currentKB / 1024).toFixed(1));
    }
    return format;  
  },

  





  _formatSeconds: function(seconds) {
    
    var hours = (seconds - (seconds % 3600)) / 3600;
    seconds -= hours * 3600;
    var minutes = (seconds - (seconds % 60)) / 60;
    seconds -= minutes * 60;
    
    
    if (hours < 10)
      hours = "0" + hours;
    if (minutes < 10)
      minutes = "0" + minutes;
    if (seconds < 10)
      seconds = "0" + seconds;
    
    
    var result = parseInt(hours) ? this._longTimeFormat : this._shortTimeFormat;
    result = this._replaceInsert(result, 1, hours);
    result = this._replaceInsert(result, 2, minutes);
    result = this._replaceInsert(result, 3, seconds);

    return result;
  }
};





var gDownloadingPage = {
  


  _downloadName     : null,
  _downloadStatus   : null,
  _downloadProgress : null,
  _downloadThrobber : null,
  _pauseButton      : null,
  
  


  _label_downloadStatus : null,

  


  _statusFormatter  : null,
  get statusFormatter() {
    if (!this._statusFormatter) 
      this._statusFormatter = new DownloadStatusFormatter();
    return this._statusFormatter;
  },
  
  


  onPageShow: function() {
    this._downloadName = document.getElementById("downloadName");
    this._downloadStatus = document.getElementById("downloadStatus");
    this._downloadProgress = document.getElementById("downloadProgress");
    this._downloadThrobber = document.getElementById("downloadThrobber");
    this._pauseButton = document.getElementById("pauseButton");
    this._label_downloadStatus = this._downloadStatus.textContent;
  
    var updates = 
        Components.classes["@mozilla.org/updates/update-service;1"].
        getService(Components.interfaces.nsIApplicationUpdateService);

    var um = 
        Components.classes["@mozilla.org/updates/update-manager;1"].
        getService(Components.interfaces.nsIUpdateManager);
    var activeUpdate = um.activeUpdate;
    if (activeUpdate)
      gUpdates.setUpdate(activeUpdate);
    
    if (!gUpdates.update) {
      LOG("UI:DownloadingPage", "onPageShow: no valid update to download?!");
      return;
    }
    
    try {
    
    
    gUpdates.update.QueryInterface(Components.interfaces.nsIWritablePropertyBag);
    gUpdates.update.setProperty("foregroundDownload", "true");
  
    
    
    updates.pauseDownload();
    var state = updates.downloadUpdate(gUpdates.update, false);
    if (state == "failed") {
      
      
      
      
      this.showVerificationError();
    }
    else {
      
      updates.addDownloadListener(this);
    }
    
    if (activeUpdate)
      this._setUIState(!updates.isDownloading);

    var link = document.getElementById("detailsLink");
    link.href = gUpdates.update.detailsURL;
    }
    catch(ex) {
      LOG("UI:DownloadingPage", "onPageShow: " + ex);
    }

    gUpdates.setButtons(null, true, null, true, null, true, "hideButton",
                        false, false, null, false, null, false);
  },
  
  


  _setStatus: function(status) {
    
    
    if (this._downloadStatus.textContent == status)
      return;
    while (this._downloadStatus.hasChildNodes())
      this._downloadStatus.removeChild(this._downloadStatus.firstChild);
    this._downloadStatus.appendChild(document.createTextNode(status));
  },
  
  


  _paused       : false,
  
  




  _setUIState: function(paused) {
    var u = gUpdates.update;
    if (paused) {
      if (this._downloadThrobber.hasAttribute("state"))
        this._downloadThrobber.removeAttribute("state");
      if (this._downloadProgress.mode != "normal")
        this._downloadProgress.mode = "normal";
      this._downloadName.value = gUpdates.strings.getFormattedString(
        "pausedName", [u.name]);
      this._pauseButton.label = gUpdates.strings.getString("pauseButtonResume");
      var p = u.selectedPatch.QueryInterface(Components.interfaces.nsIPropertyBag);
      var status = p.getProperty("status");
      if (status)
        this._setStatus(status);
    }
    else {
      if (!(this._downloadThrobber.hasAttribute("state") &&
           (this._downloadThrobber.getAttribute("state") == "loading")))
        this._downloadThrobber.setAttribute("state", "loading");
      if (this._downloadProgress.mode != "undetermined")
        this._downloadProgress.mode = "undetermined";
      this._downloadName.value = gUpdates.strings.getFormattedString(
        "downloadingPrefix", [u.name]);
      this._pauseButton.label = gUpdates.strings.getString("pauseButtonPause");
      this._setStatus(this._label_downloadStatus);
    }
  },

  


  onPause: function() {
    var updates = 
        Components.classes["@mozilla.org/updates/update-service;1"].
        getService(Components.interfaces.nsIApplicationUpdateService);
    if (this._paused)
      updates.downloadUpdate(gUpdates.update, false);
    else {
      var patch = gUpdates.update.selectedPatch;
      patch.QueryInterface(Components.interfaces.nsIWritablePropertyBag);
      patch.setProperty("status",
        gUpdates.strings.getFormattedString("pausedStatus", 
        [this.statusFormatter.progress]));
      updates.pauseDownload();
    }
    this._paused = !this._paused;
    
    
    this._setUIState(this._paused);
  },
  
  


  onWizardCancel: function() {
    
    
    
    var updates = 
        Components.classes["@mozilla.org/updates/update-service;1"].
        getService(Components.interfaces.nsIApplicationUpdateService);
    updates.removeDownloadListener(this);
    
    var um = 
        Components.classes["@mozilla.org/updates/update-manager;1"]
                  .getService(Components.interfaces.nsIUpdateManager);
    um.activeUpdate = gUpdates.update;
    
    
    
    var downloadInBackground = true;
    if (this._paused) {
      var title = gUpdates.strings.getString("resumePausedAfterCloseTitle");
      var message = gUpdates.strings.getFormattedString(
        "resumePausedAfterCloseMessage", [gUpdates.brandName]);
      var ps = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                        .getService(Components.interfaces.nsIPromptService);
      var flags = ps.STD_YES_NO_BUTTONS;
      
      
      
      
      
      window.focus();
      var rv = ps.confirmEx(window, title, message, flags, null, null, null, null, { });
      if (rv == 1) {
        downloadInBackground = false;
      }
    }
    if (downloadInBackground) {
      
      LOG("UI:DownloadingPage", "onWizardCancel: continuing download in background at full speed");
      updates.downloadUpdate(gUpdates.update, false);
    }
  },
  
  






  onStartRequest: function(request, context) {
    request.QueryInterface(nsIIncrementalDownload);
    LOG("UI:DownloadingPage", "onStartRequest: " + request.URI.spec);
    
    
    
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
    request.QueryInterface(nsIIncrementalDownload);
    LOG("UI:DownloadingPage.onProgress", " " + request.URI.spec + ", " + progress + 
        "/" + maxProgress);

    var name = gUpdates.strings.getFormattedString("downloadingPrefix", [gUpdates.update.name]);
    var status = this.statusFormatter.formatStatus(progress, maxProgress);
    var progress = Math.round(100 * (progress/maxProgress));

    var p = gUpdates.update.selectedPatch;
    p.QueryInterface(Components.interfaces.nsIWritablePropertyBag);
    p.setProperty("progress", progress);
    p.setProperty("status", status);
    
    
    
    if (this._paused)
      return;

    if (!(this._downloadThrobber.hasAttribute("state") &&
         (this._downloadThrobber.getAttribute("state") == "loading")))
      this._downloadThrobber.setAttribute("state", "loading");
    if (this._downloadProgress.mode != "normal")
      this._downloadProgress.mode = "normal";
    this._downloadProgress.value = progress;
    this._pauseButton.disabled = false;
    this._downloadName.value = name;
    this._setStatus(status);
  },
  
  










  onStatus: function(request, context, status, statusText) {
    request.QueryInterface(nsIIncrementalDownload);
    LOG("UI:DownloadingPage", "onStatus: " + request.URI.spec + " status = " + 
        status + ", text = " + statusText);
    this._setStatus(statusText);
  },
  
  








  onStopRequest: function(request, context, status) {
    request.QueryInterface(nsIIncrementalDownload);
    LOG("UI:DownloadingPage", "onStopRequest: " + request.URI.spec + 
        ", status = " + status);
    
    if (this._downloadThrobber.hasAttribute("state"))
      this._downloadThrobber.removeAttribute("state");
    if (this._downloadProgress.mode != "normal")
      this._downloadProgress.mode = "normal";

    var u = gUpdates.update;
    const NS_BINDING_ABORTED = 0x804b0002;
    switch (status) {
    case Components.results.NS_ERROR_UNEXPECTED:
      if (u.selectedPatch.state == STATE_DOWNLOAD_FAILED && 
          u.isCompleteUpdate) {
        
        
        gUpdates.wiz.currentPage = document.getElementById("errors");
      }
      else {
        
        
        
        
        
        this._downloadProgress.mode = "undetermined";
        this._pauseButton.disabled = true;
        
        var verificationFailed = document.getElementById("verificationFailed");
        verificationFailed.hidden = false;

        this._statusFormatter = null;
        return;
      }
      break;
    case NS_BINDING_ABORTED:
      LOG("UI:DownloadingPage", "onStopRequest: Pausing Download");
      
      
      return;
    case Components.results.NS_OK:
      LOG("UI:DownloadingPage", "onStopRequest: Patch Verification Succeeded");
      gUpdates.wiz.canAdvance = true;
      gUpdates.wiz.advance();
      break;
    default:
      LOG("UI:DownloadingPage", "onStopRequest: Transfer failed");
      
      gUpdates.wiz.currentPage = document.getElementById("errors");
      break;
    }

    var updates = 
        Components.classes["@mozilla.org/updates/update-service;1"].
        getService(Components.interfaces.nsIApplicationUpdateService);
    updates.removeDownloadListener(this);
  },
  
  


  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.nsIRequestObserver) &&
        !iid.equals(Components.interfaces.nsIProgressEventSink) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};




var gErrorsPage = {
  


  onPageShow: function() {
    gUpdates.setButtons(null, true, null, true, null, false, "hideButton",
                        true, false, null, false, null, false);
    gUpdates.wiz.getButton("finish").focus();
    
    var errorReason = document.getElementById("errorReason");
    errorReason.value = gUpdates.update.statusText;
    var formatter = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                              .getService(Components.interfaces.nsIURLFormatter);
    var manualURL = formatter.formatURLPref(PREF_UPDATE_MANUAL_URL);
    var errorLinkLabel = document.getElementById("errorLinkLabel");
    errorLinkLabel.value = manualURL;
    errorLinkLabel.href = manualURL;
  },
  
  


  onPageShowPatching: function() {
    gUpdates.wiz.getButton("back").disabled = true;
    gUpdates.wiz.getButton("cancel").disabled = true;
    gUpdates.wiz.getButton("next").focus();
  },

  


  onWizardFinish: function() {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    var updates =
        Components.classes["@mozilla.org/updates/update-service;1"].
        getService(Components.interfaces.nsIApplicationUpdateService);
    updates.removeDownloadListener(gDownloadingPage);
  }
};





var gFinishedPage = {
  





  onPageShow: function(aDelayRestart) {  
    gUpdates.setButtons(null, true, null, true, "restartButton", aDelayRestart,
                        "laterButton", false, false, null, false, null, false);
    if (aDelayRestart)
      setTimeout(this._enableRestartButton, 2000);
    else
      gUpdates.wiz.getButton("finish").focus();
  },
  
  


  onPageShowBackground: function() {
    var finishedBackground = document.getElementById("finishedBackground");
    finishedBackground.setAttribute("label", gUpdates.strings.getFormattedString(
      "updateReadyToInstallHeader", [gUpdates.update.name]));
    
    gUpdates.wiz._adjustWizardHeader();
    var updateFinishedName = document.getElementById("updateFinishedName");
    updateFinishedName.value = gUpdates.update.name;
    
    var link = document.getElementById("finishedBackgroundLink");
    link.href = gUpdates.update.detailsURL;
    
    this.onPageShow(true);

    if (getPref("getBoolPref", PREF_UPDATE_TEST_LOOP, false)) {
      window.restart = function () {
        gUpdates.wiz.getButton("finish").click();
      }
      setTimeout("restart();", UPDATE_TEST_LOOP_INTERVAL);
    }
  },
  
  


  _enableRestartButton: function() {
    gUpdates.wiz.canAdvance = true;
    var finishButton = gUpdates.wiz.getButton("finish");
    finishButton.disabled = false;
    finishButton.focus();
  },

  



  onWizardFinish: function() {
    
    LOG("UI:FinishedPage" , "onWizardFinish: Restarting Application...");
    
    
    
    
    
    
    
    
    
    
    gUpdates.wiz.getButton("finish").disabled = true;
    gUpdates.wiz.getButton("cancel").disabled = true;

    
    
    
    
    
    
    

    
    var os = Components.classes["@mozilla.org/observer-service;1"]
                       .getService(Components.interfaces.nsIObserverService);
    var cancelQuit = 
        Components.classes["@mozilla.org/supports-PRBool;1"].
        createInstance(Components.interfaces.nsISupportsPRBool);
    os.notifyObservers(cancelQuit, "quit-application-requested", "restart");

    
    if (cancelQuit.data)
      return;
    
    
    os.notifyObservers(null, "quit-application-granted", null);

    var appStartup = 
        Components.classes["@mozilla.org/toolkit/app-startup;1"].
        getService(Components.interfaces.nsIAppStartup);
    appStartup.quit(appStartup.eAttemptQuit | appStartup.eRestart);
  },
  
  



  onWizardCancel: function() {
    var ps = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                       .getService(Components.interfaces.nsIPromptService);
    var message = gUpdates.strings.getFormattedString("restartLaterMsg",
      [gUpdates.brandName]);
    ps.alert(window, gUpdates.strings.getString("restartLaterTitle"), 
             message);

    var interval = getPref("getIntPref", PREF_UPDATE_NAGTIMER_RESTART, 1800);
    gUpdates.registerNagTimer("restart-nag-timer", interval, 
                              "showUpdateComplete");
  }
};




var gInstalledPage = {
  


  onPageShow: function() {
    var ai = 
        Components.classes["@mozilla.org/xre/app-info;1"].
        getService(Components.interfaces.nsIXULAppInfo);
    
    var branding = document.getElementById("brandStrings");
    try {
      var url = branding.getFormattedString("whatsNewURL", [ai.version]);
      var whatsnewLink = document.getElementById("whatsnewLink");
      whatsnewLink.href = url;
      whatsnewLink.hidden = false;
    }
    catch (e) {
    }
    
    gUpdates.setButtons(null, true, null, true, null, false, "hideButton",
                        true, false, null, false, null, false);
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
