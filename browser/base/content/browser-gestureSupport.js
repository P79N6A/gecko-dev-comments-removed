# -*- Mode: javascript; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:


let gHistorySwipeAnimation = {

  active: false,
  isLTR: false,

  



  init: function HSA_init() {
    if (!this._isSupported())
      return;
  
    gBrowser.addEventListener("pagehide", this, false);
    gBrowser.addEventListener("pageshow", this, false);
    gBrowser.addEventListener("popstate", this, false);
    gBrowser.tabContainer.addEventListener("TabClose", this, false);

    this.active = true;
    this.isLTR = document.documentElement.mozMatchesSelector(
                                            ":-moz-locale-dir(ltr)");
    this._trackedSnapshots = [];
    this._historyIndex = -1;
    this._boxWidth = -1;
    this._maxSnapshots = this._getMaxSnapshots();
    this._lastSwipeDir = "";
  },

  


  uninit: function HSA_uninit() {
    gBrowser.removeEventListener("pagehide", this, false);
    gBrowser.removeEventListener("pageshow", this, false);
    gBrowser.removeEventListener("popstate", this, false);
    gBrowser.tabContainer.removeEventListener("TabClose", this, false);

    this.active = false;
    this.isLTR = false;
  },

  



  startAnimation: function HSA_startAnimation() {
    if (this.isAnimationRunning()) {
      gBrowser.stop();
      this._lastSwipeDir = "RELOAD"; 
      this._canGoBack = this.canGoBack();
      this._canGoForward = this.canGoForward();
      this._handleFastSwiping();
    }
    else {
      this._historyIndex = gBrowser.webNavigation.sessionHistory.index;
      this._canGoBack = this.canGoBack();
      this._canGoForward = this.canGoForward();
      this._takeSnapshot();
      this._installPrevAndNextSnapshots();
      this._addBoxes();
      this._lastSwipeDir = "";
    }
    this.updateAnimation(0);
  },

  


  stopAnimation: function HSA_stopAnimation() {
    gHistorySwipeAnimation._removeBoxes();
  },

  






  updateAnimation: function HSA_updateAnimation(aVal) {
    if (!this.isAnimationRunning())
      return;

    if ((aVal >= 0 && this.isLTR) ||
        (aVal <= 0 && !this.isLTR)) {
      if (aVal > 1)
        aVal = 1; 

      if (this._canGoBack)
        this._prevBox.collapsed = false;
      else
        this._prevBox.collapsed = true;

      
      
      
      this._positionBox(this._curBox, aVal);

      
      this._positionBox(this._nextBox, 1);
    }
    else {
      if (aVal < -1)
        aVal = -1; 

      
      
      
      
      
      
      if (this._canGoForward) {
        let offset = this.isLTR ? 1 : -1;
        this._positionBox(this._curBox, 0);
        this._positionBox(this._nextBox, offset + aVal); 
      }
      else {
        this._prevBox.collapsed = true;
        this._positionBox(this._curBox, aVal);
      }
    }
  },

  





  handleEvent: function HSA_handleEvent(aEvent) {
    switch (aEvent.type) {
      case "TabClose":
        let browser = gBrowser.getBrowserForTab(aEvent.target);
        this._removeTrackedSnapshot(-1, browser);
        break;
      case "pageshow":
      case "popstate":
        if (this.isAnimationRunning()) {
          if (aEvent.target != gBrowser.selectedBrowser.contentDocument)
            break;
          this.stopAnimation();
        }
        this._historyIndex = gBrowser.webNavigation.sessionHistory.index;
        break;
      case "pagehide":
        if (aEvent.target == gBrowser.selectedBrowser.contentDocument) {
          
          
          this._takeSnapshot();
        }
        break;
    }
  },

  




  isAnimationRunning: function HSA_isAnimationRunning() {
    return !!this._container;
  },

  







  processSwipeEvent: function HSA_processSwipeEvent(aEvent, aDir) {
    if (aDir == "RIGHT")
      this._historyIndex += this.isLTR ? 1 : -1;
    else if (aDir == "LEFT")
      this._historyIndex += this.isLTR ? -1 : 1;
    else
      return;
    this._lastSwipeDir = aDir;
  },

  




  canGoBack: function HSA_canGoBack() {
    if (this.isAnimationRunning())
      return this._doesIndexExistInHistory(this._historyIndex - 1);
    return gBrowser.webNavigation.canGoBack;
  },

  




  canGoForward: function HSA_canGoForward() {
    if (this.isAnimationRunning())
      return this._doesIndexExistInHistory(this._historyIndex + 1);
    return gBrowser.webNavigation.canGoForward;
  },

  




  swipeEndEventReceived: function HSA_swipeEndEventReceived() {
    if (this._lastSwipeDir != "")
      this._navigateToHistoryIndex();
    else
      this.stopAnimation();
  },

  






  _doesIndexExistInHistory: function HSA__doesIndexExistInHistory(aIndex) {
    try {
      gBrowser.webNavigation.sessionHistory.getEntryAtIndex(aIndex, false);
    }
    catch(ex) {
      return false;
    }
    return true;
  },

  



  _navigateToHistoryIndex: function HSA__navigateToHistoryIndex() {
    if (this._doesIndexExistInHistory(this._historyIndex)) {
      gBrowser.webNavigation.gotoIndex(this._historyIndex);
    }
  },

  





  _isSupported: function HSA__isSupported() {
    
    
    return window.matchMedia("(-moz-mac-lion-theme)").matches;
  },

  




  _handleFastSwiping: function HSA__handleFastSwiping() {
    this._installCurrentPageSnapshot(null);
    this._installPrevAndNextSnapshots();
  },

  


  _addBoxes: function HSA__addBoxes() {
    let browserStack =
      document.getAnonymousElementByAttribute(gBrowser.getNotificationBox(),
                                              "class", "browserStack");
    this._container = this._createElement("historySwipeAnimationContainer",
                                          "stack");
    browserStack.appendChild(this._container);

    this._prevBox = this._createElement("historySwipeAnimationPreviousPage",
                                        "box");
    this._container.appendChild(this._prevBox);

    this._curBox = this._createElement("historySwipeAnimationCurrentPage",
                                       "box");
    this._container.appendChild(this._curBox);

    this._nextBox = this._createElement("historySwipeAnimationNextPage",
                                        "box");
    this._container.appendChild(this._nextBox);

    this._boxWidth = this._curBox.getBoundingClientRect().width; 
  },

  


  _removeBoxes: function HSA__removeBoxes() {
    this._curBox = null;
    this._prevBox = null;
    this._nextBox = null;
    if (this._container)
      this._container.parentNode.removeChild(this._container);
    this._container = null;
    this._boxWidth = -1;
  },

  








  _createElement: function HSA__createElement(aID, aTagName) {
    let XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    let element = document.createElementNS(XULNS, aTagName);
    element.id = aID;
    return element;
  },

  







  _positionBox: function HSA__positionBox(aBox, aPosition) {
    aBox.style.transform = "translateX(" + this._boxWidth * aPosition + "px)";
  },

  


  _takeSnapshot: function HSA__takeSnapshot() {
    if ((this._maxSnapshots < 1) ||
        (gBrowser.webNavigation.sessionHistory.index < 0))
      return;

    let browser = gBrowser.selectedBrowser;
    let r = browser.getBoundingClientRect();
    let canvas = document.createElementNS("http://www.w3.org/1999/xhtml",
                                          "canvas");
    canvas.mozOpaque = true;
    canvas.width = r.width;
    canvas.height = r.height;
    let ctx = canvas.getContext("2d");
    let zoom = browser.markupDocumentViewer.fullZoom;
    ctx.scale(zoom, zoom);
    ctx.drawWindow(browser.contentWindow, 0, 0, r.width, r.height, "white",
                   ctx.DRAWWINDOW_DO_NOT_FLUSH | ctx.DRAWWINDOW_DRAW_VIEW |
                   ctx.DRAWWINDOW_ASYNC_DECODE_IMAGES |
                   ctx.DRAWWINDOW_USE_WIDGET_LAYERS);

    this._installCurrentPageSnapshot(canvas);
    this._assignSnapshotToCurrentBrowser(canvas);
  },

  



  _getMaxSnapshots: function HSA__getMaxSnapshots() {
    return gPrefService.getIntPref("browser.snapshots.limit");
  },

  







  _assignSnapshotToCurrentBrowser:
  function HSA__assignSnapshotToCurrentBrowser(aCanvas) {
    let browser = gBrowser.selectedBrowser;
    let currIndex = browser.webNavigation.sessionHistory.index;

    this._removeTrackedSnapshot(currIndex, browser);
    this._addSnapshotRefToArray(currIndex, browser);

    if (!("snapshots" in browser))
      browser.snapshots = [];
    let snapshots = browser.snapshots;
    
    
    
    snapshots[currIndex] = aCanvas;

    
    aCanvas.toBlob(function(aBlob) {
        snapshots[currIndex] = aBlob;
      }, "image/png"
    );
  },

  











  _removeTrackedSnapshot: function HSA__removeTrackedSnapshot(aIndex, aBrowser) {
    let arr = this._trackedSnapshots;
    let requiresExactIndexMatch = aIndex >= 0;
    for (let i = 0; i < arr.length; i++) {
      if ((arr[i].browser == aBrowser) &&
          (aIndex < 0 || aIndex == arr[i].index)) {
        delete aBrowser.snapshots[arr[i].index];
        arr.splice(i, 1);
        if (requiresExactIndexMatch)
          return; 
        i--; 
             
      }
    }
  },

  








  _addSnapshotRefToArray:
  function HSA__addSnapshotRefToArray(aIndex, aBrowser) {
    let id = { index: aIndex,
               browser: aBrowser };
    let arr = this._trackedSnapshots;
    arr.unshift(id);

    while (arr.length > this._maxSnapshots) {
      let lastElem = arr[arr.length - 1];
      delete lastElem.browser.snapshots[lastElem.index];
      arr.splice(-1, 1);
    }
  },

  









  _convertToImg: function HSA__convertToImg(aBlob) {
    if (!aBlob)
      return null;

    
    if (aBlob instanceof HTMLCanvasElement)
      return aBlob;

    let img = new Image();
    let url = URL.createObjectURL(aBlob);
    img.onload = function() {
      URL.revokeObjectURL(url);
    };
    img.src = url;
    return img;
  },

  








  _installCurrentPageSnapshot:
  function HSA__installCurrentPageSnapshot(aCanvas) {
    let currSnapshot = aCanvas;
    if (!currSnapshot) {
      let snapshots = gBrowser.selectedBrowser.snapshots || {};
      let currIndex = this._historyIndex;
      if (currIndex in snapshots)
        currSnapshot = this._convertToImg(snapshots[currIndex]);
    }
    document.mozSetImageElement("historySwipeAnimationCurrentPageSnapshot",
                                  currSnapshot);
  },

  



  _installPrevAndNextSnapshots:
  function HSA__installPrevAndNextSnapshots() {
    let snapshots = gBrowser.selectedBrowser.snapshots || [];
    let currIndex = this._historyIndex;
    let prevIndex = currIndex - 1;
    let prevSnapshot = null;
    if (prevIndex in snapshots)
      prevSnapshot = this._convertToImg(snapshots[prevIndex]);
    document.mozSetImageElement("historySwipeAnimationPreviousPageSnapshot",
                                prevSnapshot);

    let nextIndex = currIndex + 1;
    let nextSnapshot = null;
    if (nextIndex in snapshots)
      nextSnapshot = this._convertToImg(snapshots[nextIndex]);
    document.mozSetImageElement("historySwipeAnimationNextPageSnapshot",
                                nextSnapshot);
  },
};
