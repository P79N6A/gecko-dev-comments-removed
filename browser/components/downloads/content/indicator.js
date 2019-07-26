

























"use strict";











const DownloadsButton = {
  


  get kIndicatorOverlay()
      "chrome:

  



  get _placeholder()
  {
    return document.getElementById("downloads-button");
  },

  






  initializeIndicator: function DB_initializeIndicator()
  {
    if (!DownloadsCommon.useToolkitUI) {
      DownloadsIndicatorView.ensureInitialized();
    } else {
      DownloadsIndicatorView.ensureTerminated();
    }
  },

  


  _customizing: false,

  







  customizeStart: function DB_customizeStart()
  {
    
    
    this._customizing = true;
    this._anchorRequested = false;
  },

  


  customizeDone: function DB_customizeDone()
  {
    this._customizing = false;
    if (!DownloadsCommon.useToolkitUI) {
      DownloadsIndicatorView.afterCustomize();
    } else {
      DownloadsIndicatorView.ensureTerminated();
    }
  },

  





  _getAnchorInternal: function DB_getAnchorInternal()
  {
    let indicator = DownloadsIndicatorView.indicator;
    if (!indicator) {
      
      
      return null;
    }

    indicator.open = this._anchorRequested;

    
    if (!isElementVisible(indicator.parentNode)) {
      return null;
    }

    return DownloadsIndicatorView.indicatorAnchor;
  },

  







  checkIsVisible: function DB_checkIsVisible(aCallback)
  {
    function DB_CEV_callback() {
      if (!this._placeholder) {
        aCallback(false);
      } else {
        let element = DownloadsIndicatorView.indicator || this._placeholder;
        aCallback(isElementVisible(element.parentNode));
      }
    }
    DownloadsOverlayLoader.ensureOverlayLoaded(this.kIndicatorOverlay,
                                               DB_CEV_callback.bind(this));
  },

  



  _anchorRequested: false,

  







  getAnchor: function DB_getAnchor(aCallback)
  {
    
    if (this._customizing) {
      aCallback(null);
      return;
    }

    function DB_GA_callback() {
      this._anchorRequested = true;
      aCallback(this._getAnchorInternal());
    }

    DownloadsOverlayLoader.ensureOverlayLoaded(this.kIndicatorOverlay,
                                               DB_GA_callback.bind(this));
  },

  


  releaseAnchor: function DB_releaseAnchor()
  {
    this._anchorRequested = false;
    this._getAnchorInternal();
  },

  get _tabsToolbar()
  {
    delete this._tabsToolbar;
    return this._tabsToolbar = document.getElementById("TabsToolbar");
  },

  get _navBar()
  {
    delete this._navBar;
    return this._navBar = document.getElementById("nav-bar");
  }
};










const DownloadsIndicatorView = {
  


  _initialized: false,

  



  _operational: false,

  


  ensureInitialized: function DIV_ensureInitialized()
  {
    if (this._initialized) {
      return;
    }
    this._initialized = true;

    window.addEventListener("unload", this.onWindowUnload, false);
    DownloadsCommon.getIndicatorData(window).addView(this);
  },

  


  ensureTerminated: function DIV_ensureTerminated()
  {
    if (!this._initialized) {
      return;
    }
    this._initialized = false;

    window.removeEventListener("unload", this.onWindowUnload, false);
    DownloadsCommon.getIndicatorData(window).removeView(this);

    
    
    this.counter = "";
    this.percentComplete = 0;
    this.paused = false;
    this.attention = false;
  },

  



  _ensureOperational: function DIV_ensureOperational(aCallback)
  {
    if (this._operational) {
      if (aCallback) {
        aCallback();
      }
      return;
    }

    
    
    if (!DownloadsButton._placeholder) {
      return;
    }

    function DIV_EO_callback() {
      this._operational = true;

      
      
      if (this._initialized) {
        DownloadsCommon.getIndicatorData(window).refreshView(this);
      }

      if (aCallback) {
        aCallback();
      }
    }

    DownloadsOverlayLoader.ensureOverlayLoaded(
                                 DownloadsButton.kIndicatorOverlay,
                                 DIV_EO_callback.bind(this));
  },

  
  

  


  _notificationTimeout: null,

  






  showEventNotification: function DIV_showEventNotification(aType)
  {
    if (!this._initialized) {
      return;
    }

    if (!DownloadsCommon.animateNotifications) {
      return;
    }

    
    if (DownloadsPanel.isPanelShowing) {
      return;
    }

    
    
    let anchor = DownloadsButton._placeholder;
    if (!anchor || !isElementVisible(anchor.parentNode)) {
      return;
    }

    if (this._notificationTimeout) {
      clearTimeout(this._notificationTimeout);
    }

    
    
    
    
    
    let notifier = this.notifier;
    if (notifier.style.transform == '') {
      let anchorRect = anchor.getBoundingClientRect();
      let notifierRect = notifier.getBoundingClientRect();
      let topDiff = anchorRect.top - notifierRect.top;
      let leftDiff = anchorRect.left - notifierRect.left;
      let heightDiff = anchorRect.height - notifierRect.height;
      let widthDiff = anchorRect.width - notifierRect.width;
      let translateX = (leftDiff + .5 * widthDiff) + "px";
      let translateY = (topDiff + .5 * heightDiff) + "px";
      notifier.style.transform = "translate(" +  translateX + ", " + translateY + ")";
    }
    notifier.setAttribute("notification", aType);
    this._notificationTimeout = setTimeout(function () {
      notifier.removeAttribute("notification");
      notifier.style.transform = '';
    }, 1000);
  },

  
  

  



  set hasDownloads(aValue)
  {
    if (this._hasDownloads != aValue) {
      this._hasDownloads = aValue;

      
      if (aValue) {
        this._ensureOperational();
      }
    }
    return aValue;
  },
  get hasDownloads()
  {
    return this._hasDownloads;
  },
  _hasDownloads: false,

  



  set counter(aValue)
  {
    if (!this._operational) {
      return this._counter;
    }

    if (this._counter !== aValue) {
      this._counter = aValue;
      if (this._counter)
        this.indicator.setAttribute("counter", "true");
      else
        this.indicator.removeAttribute("counter");
      
      
      this._indicatorCounter.setAttribute("value", aValue);
    }
    return aValue;
  },
  _counter: null,

  




  set percentComplete(aValue)
  {
    if (!this._operational) {
      return this._percentComplete;
    }

    if (this._percentComplete !== aValue) {
      this._percentComplete = aValue;
      if (this._percentComplete >= 0)
        this.indicator.setAttribute("progress", "true");
      else
        this.indicator.removeAttribute("progress");
      
      
      this._indicatorProgress.setAttribute("value", Math.max(aValue, 0));
    }
    return aValue;
  },
  _percentComplete: null,

  




  set paused(aValue)
  {
    if (!this._operational) {
      return this._paused;
    }

    if (this._paused != aValue) {
      this._paused = aValue;
      if (this._paused) {
        this.indicator.setAttribute("paused", "true")
      } else {
        this.indicator.removeAttribute("paused");
      }
    }
    return aValue;
  },
  _paused: false,

  


  set attention(aValue)
  {
    if (!this._operational) {
      return this._attention;
    }

    if (this._attention != aValue) {
      this._attention = aValue;
      if (aValue) {
        this.indicator.setAttribute("attention", "true");
      } else {
        this.indicator.removeAttribute("attention");
      }
    }
    return aValue;
  },
  _attention: false,

  
  

  onWindowUnload: function DIV_onWindowUnload()
  {
    
    DownloadsIndicatorView.ensureTerminated();
  },

  onCommand: function DIV_onCommand(aEvent)
  {
    if (DownloadsCommon.useToolkitUI) {
      
      DownloadsCommon.getIndicatorData(window).attention = false;
      BrowserDownloadsUI();
    } else {
      DownloadsPanel.showPanel();
    }

    aEvent.stopPropagation();
  },

  onDragOver: function DIV_onDragOver(aEvent)
  {
    browserDragAndDrop.dragOver(aEvent);
  },

  onDrop: function DIV_onDrop(aEvent)
  {
    let dt = aEvent.dataTransfer;
    
    
    if (dt.mozGetDataAt("application/x-moz-file", 0))
      return;

    let name = {};
    let url = browserDragAndDrop.drop(aEvent, name);
    if (url) {
      if (url.startsWith("about:")) {
        return;
      }

      let sourceDoc = dt.mozSourceNode ? dt.mozSourceNode.ownerDocument : document;
      saveURL(url, name.value, null, true, true, null, sourceDoc);
      aEvent.preventDefault();
    }
  },

  _indicator: null,
  _indicatorAnchor: null,
  __indicatorCounter: null,
  __indicatorProgress: null,

  



  get indicator()
  {
    if (this._indicator) {
      return this._indicator;
    }

    let indicator = document.getElementById("downloads-button");
    if (!indicator || indicator.getAttribute("indicator") != "true") {
      return null;
    }

    return this._indicator = indicator;
  },

  get indicatorAnchor()
  {
    return this._indicatorAnchor ||
      (this._indicatorAnchor = document.getElementById("downloads-indicator-anchor"));
  },

  get _indicatorCounter()
  {
    return this.__indicatorCounter ||
      (this.__indicatorCounter = document.getElementById("downloads-indicator-counter"));
  },

  get _indicatorProgress()
  {
    return this.__indicatorProgress ||
      (this.__indicatorProgress = document.getElementById("downloads-indicator-progress"));
  },

  get notifier()
  {
    return this._notifier ||
      (this._notifier = document.getElementById("downloads-notification-anchor"));
  },

  _onCustomizedAway: function() {
    this._indicator = null;
    this._indicatorAnchor = null;
    this.__indicatorCounter = null;
    this.__indicatorProgress = null;
  },

  afterCustomize: function() {
    
    
    if (this._indicator != document.getElementById("downloads-button")) {
      this._onCustomizedAway();
      this._operational = false;
      this.ensureTerminated();
      this.ensureInitialized();
    }
  }
};

