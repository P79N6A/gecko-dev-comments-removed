




const kHeartbeatDuration = 1000;

const kProgressMarginStart = 30;
const kProgressMarginEnd = 70;

const WebProgress = {
  _progressActive: false,

  init: function init() {
    messageManager.addMessageListener("Content:StateChange", this);
    messageManager.addMessageListener("Content:LocationChange", this);
    messageManager.addMessageListener("Content:SecurityChange", this);
    Elements.progress.addEventListener("transitionend", this._progressTransEnd, true);
    Elements.tabList.addEventListener("TabSelect", this._onTabSelect, true);

    let urlBar = document.getElementById("urlbar-edit");
    urlBar.addEventListener("input", this._onUrlBarInput, false);

    return this;
  },

  receiveMessage: function receiveMessage(aMessage) {
    let json = aMessage.json;
    let tab = Browser.getTabForBrowser(aMessage.target);

    switch (aMessage.name) {
      case "Content:StateChange": {
        if (json.stateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) {
          if (json.stateFlags & Ci.nsIWebProgressListener.STATE_START)
            this._windowStart(json, tab);
          else if (json.stateFlags & Ci.nsIWebProgressListener.STATE_STOP)
            this._windowStop(json, tab);
        }

        if (json.stateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
          if (json.stateFlags & Ci.nsIWebProgressListener.STATE_START)
            this._networkStart(json, tab);
          else if (json.stateFlags & Ci.nsIWebProgressListener.STATE_STOP)
            this._networkStop(json, tab);
        }

        this._progressStep();
        break;
      }

      case "Content:LocationChange": {
        this._locationChange(json, tab);
        this._progressStep();
        break;
      }

      case "Content:SecurityChange": {
        this._securityChange(json, tab);
        this._progressStep();
        break;
      }
    }
  },

  _securityChange: function _securityChange(aJson, aTab) {
    let state = aJson.state;
    let nsIWebProgressListener = Ci.nsIWebProgressListener;

    if (state & nsIWebProgressListener.STATE_IDENTITY_EV_TOPLEVEL) {
      aTab._identityState = "verifiedIdentity";
    } else if (state & nsIWebProgressListener.STATE_IS_SECURE) {
      aTab._identityState = "verifiedDomain";
    } else {
      aTab._identityState = "";
    }

    if (aTab == Browser.selectedTab) {
      let identityBox = document.getElementById("identity-box-inner");
      identityBox.className = aTab._identityState;
    }
  },

  _locationChange: function _locationChange(aJson, aTab) {
    let spec = aJson.location;
    let location = spec.split("#")[0]; 

    if (aTab == Browser.selectedTab) {
      BrowserUI.updateURI();
      BrowserUI.update();
    }

    let locationHasChanged = (location != aTab.browser.lastLocation);
    if (locationHasChanged) {
      Browser.getNotificationBox(aTab.browser).removeTransientNotifications();
      aTab.resetZoomLevel();
      aTab.browser.lastLocation = location;
      aTab.browser.userTypedValue = "";
      aTab.browser.appIcon = { href: null, size:-1 };

#ifdef MOZ_CRASHREPORTER
      if (CrashReporter.enabled)
        CrashReporter.annotateCrashReport("URL", spec);
#endif
      this._waitForLoad(aTab);
    }

    let event = document.createEvent("UIEvents");
    event.initUIEvent("URLChanged", true, false, window, locationHasChanged);
    aTab.browser.dispatchEvent(event);
  },

  _waitForLoad: function _waitForLoad(aTab) {
    let browser = aTab.browser;

    aTab._firstPaint = false;

    browser.messageManager.addMessageListener("Browser:FirstPaint", function firstPaintListener(aMessage) {
      browser.messageManager.removeMessageListener(aMessage.name, arguments.callee);
      aTab._firstPaint = true;
      aTab.scrolledAreaChanged(true);
      aTab.updateThumbnailSource();
    });
  },

  _networkStart: function _networkStart(aJson, aTab) {
    aTab.startLoading();

    if (aTab == Browser.selectedTab)
      BrowserUI.update(TOOLBARSTATE_LOADING);
  },

  _networkStop: function _networkStop(aJson, aTab) {
    aTab.endLoading();

    if (aTab == Browser.selectedTab)
      BrowserUI.update(TOOLBARSTATE_LOADED);
  },

  _windowStart: function _windowStart(aJson, aTab) {
    this._progressStart(aJson, aTab);
  },

  _windowStop: function _windowStop(aJson, aTab) {
    this._progressStop(aJson, aTab);
  },

  _progressStart: function _progressStart(aJson, aTab) {
    
    
    if (this._progressActive)
      return;

    this._progressActive = true;

    
    Elements.progressContainer.removeAttribute("collapsed");

    
    this._progressCount = kProgressMarginStart;
    Elements.progress.style.width = this._progressCount + "%";
    Elements.progress.removeAttribute("fade");

    
    
    setTimeout(function() {
      WebProgress._progressStepTimer();
    }, kHeartbeatDuration, this);
  },

  _stepProgressCount: function _stepProgressCount() {
    
    let left = kProgressMarginEnd - this._progressCount;
    let step = left * .05;
    this._progressCount += Math.ceil(step);

    
    if (this._progressCount > kProgressMarginEnd) {
      this._progressCount = kProgressMarginEnd;
    }
  },

  _progressStep: function _progressStep() {
    if (!this._progressActive)
      return;
    this._stepProgressCount();
    Elements.progress.style.width = this._progressCount + "%";
  },

  _progressStepTimer: function _progressStepTimer() {
    if (!this._progressActive)
      return;
    this._progressStep();

    setTimeout(function() {
      WebProgress._progressStepTimer();
    }, kHeartbeatDuration, this);
  },

  _progressStop: function _progressStop(aJson, aTab) {
    this._progressActive = false;
    
    Elements.progress.style.width = "100%";
    Elements.progress.setAttribute("fade", true);
  },

  _progressTransEnd: function _progressTransEnd(data) {
    if (!Elements.progress.hasAttribute("fade"))
      return;
    
    if (data.propertyName == "opacity") {
      Elements.progress.style.width = "0px";
      Elements.progressContainer.setAttribute("collapsed", true);
    }
  },

  _onTabSelect: function(aEvent) {
    let identityBox = document.getElementById("identity-box-inner");
    let tab = Browser.getTabFromChrome(aEvent.originalTarget);
    identityBox.className = tab._identityState || "";
  },

  _onUrlBarInput: function(aEvent) {
    let identityBox = document.getElementById("identity-box-inner");
    Browser.selectedTab._identityState = identityBox.className = "";
  },
};
