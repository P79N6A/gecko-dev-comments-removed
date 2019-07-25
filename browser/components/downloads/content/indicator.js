

























"use strict";











const DownloadsButton = {
  



  get _placeholder()
  {
    return document.getElementById("downloads-button");
  },

  







  initializePlaceholder: function DB_initializePlaceholder()
  {
    
    
    let placeholder = this._placeholder;
    if (placeholder) {
      placeholder.collapsed = true;
    }
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
    DownloadsIndicatorView.indicator.collapsed = true;

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

    let placeholder = this._placeholder;
    if (!DownloadsCommon.useToolkitUI) {
      DownloadsIndicatorView.ensureInitialized();
      if (placeholder) {
        placeholder.collapsed = true;
      }
    } else {
      DownloadsIndicatorView.ensureTerminated();
    }
  },

  





  updatePosition: function DB_updatePosition()
  {
    
    
    
    if (!this._anchorRequested) {
      return this._updatePositionInternal();
    }
  },

  _updatePositionInternal: function DB_updatePositionInternal()
  {
    let indicator = DownloadsIndicatorView.indicator;
    let placeholder = this._placeholder;

    
    if (!placeholder && !this._anchorRequested &&
        !DownloadsIndicatorView.hasDownloads) {
      indicator.collapsed = true;
      return null;
    }
    indicator.collapsed = false;

    indicator.open = this._anchorRequested;

    
    if (placeholder) {
      placeholder.parentNode.insertBefore(indicator, placeholder);
      
      if (isElementVisible(placeholder.parentNode)) {
        return DownloadsIndicatorView.indicatorAnchor;
      }
    }

    
    
    if (!this._anchorRequested) {
      this._navBar.appendChild(indicator);
      return null;
    }

    
    if (isElementVisible(this._navBar)) {
      this._navBar.appendChild(indicator);
      return DownloadsIndicatorView.indicatorAnchor;
    }

    
    if (!this._tabsToolbar.collapsed) {
      this._tabsToolbar.appendChild(indicator);
      return DownloadsIndicatorView.indicatorAnchor;
    }

    
    return null;
  },

  



  _anchorRequested: false,

  






  getAnchor: function DB_getAnchor()
  {
    
    if (this._customizing) {
      return null;
    }

    this._anchorRequested = true;
    return this._updatePositionInternal();
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

  


  ensureInitialized: function DIV_ensureInitialized()
  {
    if (this._initialized) {
      return;
    }
    this._initialized = true;

    window.addEventListener("unload", this.onWindowUnload, false);
    DownloadsCommon.indicatorData.addView(this);
  },

  


  ensureTerminated: function DIV_ensureTerminated()
  {
    if (!this._initialized) {
      return;
    }
    this._initialized = false;

    window.removeEventListener("unload", this.onWindowUnload, false);
    DownloadsCommon.indicatorData.removeView(this);

    
    
    this.counter = "";
    this.percentComplete = 0;
    this.paused = false;
    this.attention = false;
  },

  
  

  


  _notificationTimeout: null,

  



  showEventNotification: function DIV_showEventNotification()
  {
    if (!this._initialized) {
      return;
    }

    if (this._notificationTimeout) {
      clearTimeout(this._notificationTimeout);
    }

    let indicator = this.indicator;
    indicator.setAttribute("notification", "true");
    this._notificationTimeout = setTimeout(
      function () indicator.removeAttribute("notification"), 1000);
  },

  
  

  



  set hasDownloads(aValue)
  {
    if (this._hasDownloads != aValue) {
      this._hasDownloads = aValue;
      DownloadsButton.updatePosition();
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
      
      DownloadsCommon.indicatorData.attention = false;
    }

    BrowserDownloadsUI();

    aEvent.stopPropagation();
  },

  onDragOver: function DIV_onDragOver(aEvent)
  {
    browserDragAndDrop.dragOver(aEvent);
  },

  onDragExit: function () { },

  onDrop: function DIV_onDrop(aEvent)
  {
    let name = {};
    let url = browserDragAndDrop.drop(aEvent, name);
    if (url) {
      saveURL(url, name.value, null, true, true);
      aEvent.preventDefault();
    }
  },

  get indicator()
  {
    delete this.indicator;
    return this.indicator = document.getElementById("downloads-indicator");
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
