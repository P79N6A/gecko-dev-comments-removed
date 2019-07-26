

























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
    this._update();
  },

  


  _customizing: false,

  







  customizeStart: function DB_customizeStart()
  {
    
    
    this._customizing = true;
    this._anchorRequested = false;

    let indicator = DownloadsIndicatorView.indicator;
    if (indicator) {
      indicator.collapsed = true;
    }

    let placeholder = this._placeholder;
    if (placeholder) {
      placeholder.collapsed = false;
    }
  },

  


  customizeDone: function DB_customizeDone()
  {
    this._customizing = false;
    this._update();
  },

  









  _update: function DB_update() {
    this._updatePositionInternal();

    if (!DownloadsCommon.useToolkitUI) {
      DownloadsIndicatorView.ensureInitialized();
    } else {
      DownloadsIndicatorView.ensureTerminated();
    }
  },

  






  updatePosition: function DB_updatePosition()
  {
    if (!this._anchorRequested) {
      this._updatePositionInternal();
    }
  },

  





  _updatePositionInternal: function DB_updatePositionInternal()
  {
    let indicator = DownloadsIndicatorView.indicator;
    if (!indicator) {
      
      return null;
    }

    let placeholder = this._placeholder;
    if (!placeholder) {
      
      indicator.collapsed = true;
      
      
      indicator.parentNode.appendChild(indicator);
      return null;
    }

    
    
    
    placeholder.parentNode.insertBefore(indicator, placeholder);
    placeholder.collapsed = true;
    indicator.collapsed = false;

    indicator.open = this._anchorRequested;

    
    if (!isElementVisible(placeholder.parentNode)) {
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
      aCallback(this._updatePositionInternal());
    }

    DownloadsOverlayLoader.ensureOverlayLoaded(this.kIndicatorOverlay,
                                               DB_GA_callback.bind(this));
  },

  


  releaseAnchor: function DB_releaseAnchor()
  {
    this._anchorRequested = false;
    this._updatePositionInternal();
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
      aCallback();
      return;
    }

    function DIV_EO_callback() {
      this._operational = true;

      
      
      if (this._initialized) {
        DownloadsCommon.getIndicatorData(window).refreshView(this);
      }

      aCallback();
    }

    DownloadsOverlayLoader.ensureOverlayLoaded(
                                 DownloadsButton.kIndicatorOverlay,
                                 DIV_EO_callback.bind(this));
  },

  
  

  


  _notificationTimeout: null,

  



  showEventNotification: function DIV_showEventNotification()
  {
    if (!this._initialized) {
      return;
    }

    function DIV_SEN_callback() {
      if (this._notificationTimeout) {
        clearTimeout(this._notificationTimeout);
      }

      
      
      DownloadsButton.updatePosition();

      let indicator = this.indicator;
      indicator.setAttribute("notification", "true");
      this._notificationTimeout = setTimeout(
        function () indicator.removeAttribute("notification"), 1000);
    }

    this._ensureOperational(DIV_SEN_callback.bind(this));
  },

  
  

  



  set hasDownloads(aValue)
  {
    if (this._hasDownloads != aValue) {
      this._hasDownloads = aValue;

      
      
      if (aValue) {
        this._ensureOperational(function() DownloadsButton.updatePosition());
      } else {
        DownloadsButton.updatePosition();
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
        this.indicator.setAttribute("attention", "true")
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

  onDragExit: function () { },

  onDrop: function DIV_onDrop(aEvent)
  {
    let dt = aEvent.dataTransfer;
    
    
    if (dt.mozGetDataAt("application/x-moz-file", 0))
      return;

    let name = {};
    let url = browserDragAndDrop.drop(aEvent, name);
    if (url) {
      saveURL(url, name.value, null, true, true);
      aEvent.preventDefault();
    }
  },

  



  get indicator()
  {
    let indicator = document.getElementById("downloads-indicator");
    if (!indicator) {
      return null;
    }

    
    delete this.indicator;
    return this.indicator = indicator;
  },

  get indicatorAnchor()
  {
    delete this.indicatorAnchor;
    return this.indicatorAnchor =
      document.getElementById("downloads-indicator-anchor");
  },

  get _indicatorCounter()
  {
    delete this._indicatorCounter;
    return this._indicatorCounter =
      document.getElementById("downloads-indicator-counter");
  },

  get _indicatorProgress()
  {
    delete this._indicatorProgress;
    return this._indicatorProgress =
      document.getElementById("downloads-indicator-progress");
  }
};
