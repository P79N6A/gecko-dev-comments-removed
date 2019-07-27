# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

Cu.import("resource://gre/modules/TelemetryStopwatch.jsm", this);












let gGestureSupport = {
  _currentRotation: 0,
  _lastRotateDelta: 0,
  _rotateMomentumThreshold: .75,

  





  init: function GS_init(aAddListener) {
    const gestureEvents = ["SwipeGestureStart",
      "SwipeGestureUpdate", "SwipeGestureEnd", "SwipeGesture",
      "MagnifyGestureStart", "MagnifyGestureUpdate", "MagnifyGesture",
      "RotateGestureStart", "RotateGestureUpdate", "RotateGesture",
      "TapGesture", "PressTapGesture"];

    let addRemove = aAddListener ? window.addEventListener :
      window.removeEventListener;

    gestureEvents.forEach(function (event) addRemove("Moz" + event, this, true),
                          this);
  },

  







  handleEvent: function GS_handleEvent(aEvent) {
    if (!Services.prefs.getBoolPref(
           "dom.debug.propagate_gesture_events_through_content")) {
      aEvent.stopPropagation();
    }

    
    let def = function(aThreshold, aLatched)
      ({ threshold: aThreshold, latched: !!aLatched });

    switch (aEvent.type) {
      case "MozSwipeGestureStart":
        if (this._setupSwipeGesture(aEvent)) {
          aEvent.preventDefault();
        }
        break;
      case "MozSwipeGestureUpdate":
        aEvent.preventDefault();
        this._doUpdate(aEvent);
        break;
      case "MozSwipeGestureEnd":
        aEvent.preventDefault();
        this._doEnd(aEvent);
        break;
      case "MozSwipeGesture":
        aEvent.preventDefault();
        this.onSwipe(aEvent);
        break;
      case "MozMagnifyGestureStart":
        aEvent.preventDefault();
#ifdef XP_WIN
        this._setupGesture(aEvent, "pinch", def(25, 0), "out", "in");
#else
        this._setupGesture(aEvent, "pinch", def(150, 1), "out", "in");
#endif
        break;
      case "MozRotateGestureStart":
        aEvent.preventDefault();
        this._setupGesture(aEvent, "twist", def(25, 0), "right", "left");
        break;
      case "MozMagnifyGestureUpdate":
      case "MozRotateGestureUpdate":
        aEvent.preventDefault();
        this._doUpdate(aEvent);
        break;
      case "MozTapGesture":
        aEvent.preventDefault();
        this._doAction(aEvent, ["tap"]);
        break;
      case "MozRotateGesture":
        aEvent.preventDefault();
        this._doAction(aEvent, ["twist", "end"]);
        break;
      

    }
  },

  














  _setupGesture: function GS__setupGesture(aEvent, aGesture, aPref, aInc, aDec) {
    
    for (let [pref, def] in Iterator(aPref))
      aPref[pref] = this._getPref(aGesture + "." + pref, def);

    
    let offset = 0;
    let latchDir = aEvent.delta > 0 ? 1 : -1;
    let isLatched = false;

    
    this._doUpdate = function GS__doUpdate(aEvent) {
      
      offset += aEvent.delta;

      
      if (Math.abs(offset) > aPref["threshold"]) {
        
        
        
        let sameDir = (latchDir ^ offset) >= 0;
        if (!aPref["latched"] || (isLatched ^ sameDir)) {
          this._doAction(aEvent, [aGesture, offset > 0 ? aInc : aDec]);

          
          isLatched = !isLatched;
        }

        
        offset = 0;
      }
    };

    
    this._doUpdate(aEvent);
  },

  







  _swipeNavigatesHistory: function GS__swipeNavigatesHistory(aEvent) {
    return this._getCommand(aEvent, ["swipe", "left"])
              == "Browser:BackOrBackDuplicate" &&
           this._getCommand(aEvent, ["swipe", "right"])
              == "Browser:ForwardOrForwardDuplicate";
  },

  








  _setupSwipeGesture: function GS__setupSwipeGesture(aEvent) {
    if (!this._swipeNavigatesHistory(aEvent)) {
      return false;
    }

    let isVerticalSwipe = false;
    if (aEvent.direction == aEvent.DIRECTION_UP) {
      if (gMultiProcessBrowser || content.pageYOffset > 0) {
        return false;
      }
      isVerticalSwipe = true;
    } else if (aEvent.direction == aEvent.DIRECTION_DOWN) {
      if (gMultiProcessBrowser || content.pageYOffset < content.scrollMaxY) {
        return false;
      }
      isVerticalSwipe = true;
    }
    if (isVerticalSwipe) {
      
      
      return false;
    }

    let canGoBack = gHistorySwipeAnimation.canGoBack();
    let canGoForward = gHistorySwipeAnimation.canGoForward();
    let isLTR = gHistorySwipeAnimation.isLTR;

    if (canGoBack) {
      aEvent.allowedDirections |= isLTR ? aEvent.DIRECTION_LEFT :
                                          aEvent.DIRECTION_RIGHT;
    }
    if (canGoForward) {
      aEvent.allowedDirections |= isLTR ? aEvent.DIRECTION_RIGHT :
                                          aEvent.DIRECTION_LEFT;
    }

    gHistorySwipeAnimation.startAnimation(isVerticalSwipe);

    this._doUpdate = function GS__doUpdate(aEvent) {
      gHistorySwipeAnimation.updateAnimation(aEvent.delta);
    };

    this._doEnd = function GS__doEnd(aEvent) {
      gHistorySwipeAnimation.swipeEndEventReceived();

      this._doUpdate = function (aEvent) {};
      this._doEnd = function (aEvent) {};
    }

    return true;
  },

  







  _power: function GS__power(aArray) {
    
    let num = 1 << aArray.length;
    while (--num >= 0) {
      
      yield aArray.reduce(function (aPrev, aCurr, aIndex) {
        if (num & 1 << aIndex)
          aPrev.push(aCurr);
        return aPrev;
      }, []);
    }
  },

  










  _doAction: function GS__doAction(aEvent, aGesture) {
    let command = this._getCommand(aEvent, aGesture);
    return command && this._doCommand(aEvent, command);
  },

  








  _getCommand: function GS__getCommand(aEvent, aGesture) {
    
    
    
    let keyCombos = [];
    ["shift", "alt", "ctrl", "meta"].forEach(function (key) {
      if (aEvent[key + "Key"])
        keyCombos.push(key);
    });

    
    for (let subCombo of this._power(keyCombos)) {
      
      
      
      let command;
      try {
        command = this._getPref(aGesture.concat(subCombo).join("."));
      } catch (e) {}

      if (command)
        return command;
    }
    return null;
  },

  







  _doCommand: function GS__doCommand(aEvent, aCommand) {
    let node = document.getElementById(aCommand);
    if (node) {
      if (node.getAttribute("disabled") != "true") {
        let cmdEvent = document.createEvent("xulcommandevent");
        cmdEvent.initCommandEvent("command", true, true, window, 0,
                                  aEvent.ctrlKey, aEvent.altKey,
                                  aEvent.shiftKey, aEvent.metaKey, aEvent);
        node.dispatchEvent(cmdEvent);
      }

    }
    else {
      goDoCommand(aCommand);
    }
  },

  






  _doUpdate: function(aEvent) {},

  





  _doEnd: function(aEvent) {},

  





  onSwipe: function GS_onSwipe(aEvent) {
    
    for (let dir of ["UP", "RIGHT", "DOWN", "LEFT"]) {
      if (aEvent.direction == aEvent["DIRECTION_" + dir]) {
        this._coordinateSwipeEventWithAnimation(aEvent, dir);
        break;
      }
    }
  },

  







  processSwipeEvent: function GS_processSwipeEvent(aEvent, aDir) {
    this._doAction(aEvent, ["swipe", aDir.toLowerCase()]);
  },

  










  _coordinateSwipeEventWithAnimation:
  function GS__coordinateSwipeEventWithAnimation(aEvent, aDir) {
    if ((gHistorySwipeAnimation.isAnimationRunning()) &&
        (aDir == "RIGHT" || aDir == "LEFT")) {
      gHistorySwipeAnimation.processSwipeEvent(aEvent, aDir);
    }
    else {
      this.processSwipeEvent(aEvent, aDir);
    }
  },

  







  _getPref: function GS__getPref(aPref, aDef) {
    
    const branch = "browser.gesture.";

    try {
      
      let type = typeof aDef;
      let getFunc = "get" + (type == "boolean" ? "Bool" :
                             type == "number" ? "Int" : "Char") + "Pref";
      return gPrefService[getFunc](branch + aPref);
    }
    catch (e) {
      return aDef;
    }
  },

  





  rotate: function(aEvent) {
    if (!(content.document instanceof ImageDocument))
      return;

    let contentElement = content.document.body.firstElementChild;
    if (!contentElement)
      return;
    
    if (contentElement.classList.contains("completeRotation"))
      this._clearCompleteRotation();

    this.rotation = Math.round(this.rotation + aEvent.delta);
    contentElement.style.transform = "rotate(" + this.rotation + "deg)";
    this._lastRotateDelta = aEvent.delta;
  },

  


  rotateEnd: function() {
    if (!(content.document instanceof ImageDocument))
      return;

    let contentElement = content.document.body.firstElementChild;
    if (!contentElement)
      return;

    let transitionRotation = 0;

    
    
    
    if (this.rotation <= 45)
      transitionRotation = 0;
    else if (this.rotation > 45 && this.rotation <= 135)
      transitionRotation = 90;
    else if (this.rotation > 135 && this.rotation <= 225)
      transitionRotation = 180;
    else if (this.rotation > 225 && this.rotation <= 315)
      transitionRotation = 270;
    else
      transitionRotation = 360;

    
    
    if (this._lastRotateDelta > this._rotateMomentumThreshold &&
        this.rotation > transitionRotation)
      transitionRotation += 90;
    else if (this._lastRotateDelta < -1 * this._rotateMomentumThreshold &&
             this.rotation < transitionRotation)
      transitionRotation -= 90;

    
    if (transitionRotation != this.rotation) {
      contentElement.classList.add("completeRotation");
      contentElement.addEventListener("transitionend", this._clearCompleteRotation);
    }

    contentElement.style.transform = "rotate(" + transitionRotation + "deg)";
    this.rotation = transitionRotation;
  },

  


  get rotation() {
    return this._currentRotation;
  },

  






  set rotation(aVal) {
    this._currentRotation = aVal % 360;
    if (this._currentRotation < 0)
      this._currentRotation += 360;
    return this._currentRotation;
  },

  



  restoreRotationState: function() {
    
    if (gMultiProcessBrowser)
      return;

    if (!(content.document instanceof ImageDocument))
      return;

    let contentElement = content.document.body.firstElementChild;
    let transformValue = content.window.getComputedStyle(contentElement, null)
                                       .transform;

    if (transformValue == "none") {
      this.rotation = 0;
      return;
    }

    
    
    transformValue = transformValue.split("(")[1]
                                   .split(")")[0]
                                   .split(",");
    this.rotation = Math.round(Math.atan2(transformValue[1], transformValue[0]) *
                               (180 / Math.PI));
  },

  


  _clearCompleteRotation: function() {
    let contentElement = content.document &&
                         content.document instanceof ImageDocument &&
                         content.document.body &&
                         content.document.body.firstElementChild;
    if (!contentElement)
      return;
    contentElement.classList.remove("completeRotation");
    contentElement.removeEventListener("transitionend", this._clearCompleteRotation);
  },
};


let gHistorySwipeAnimation = {

  active: false,
  isLTR: false,

  



  init: function HSA_init() {
    if (!this._isSupported())
      return;

    this.active = false;
    this.isLTR = document.documentElement.matches(":-moz-locale-dir(ltr)");
    this._trackedSnapshots = [];
    this._startingIndex = -1;
    this._historyIndex = -1;
    this._boxWidth = -1;
    this._boxHeight = -1;
    this._maxSnapshots = this._getMaxSnapshots();
    this._lastSwipeDir = "";
    this._direction = "horizontal";

    
    
    if (this._maxSnapshots > 0) {
      this.active = true;
      gBrowser.addEventListener("pagehide", this, false);
      gBrowser.addEventListener("pageshow", this, false);
      gBrowser.addEventListener("popstate", this, false);
      gBrowser.addEventListener("DOMModalDialogClosed", this, false);
      gBrowser.tabContainer.addEventListener("TabClose", this, false);
    }
  },

  


  uninit: function HSA_uninit() {
    gBrowser.removeEventListener("pagehide", this, false);
    gBrowser.removeEventListener("pageshow", this, false);
    gBrowser.removeEventListener("popstate", this, false);
    gBrowser.removeEventListener("DOMModalDialogClosed", this, false);
    gBrowser.tabContainer.removeEventListener("TabClose", this, false);

    this.active = false;
    this.isLTR = false;
  },

  






  startAnimation: function HSA_startAnimation(aIsVerticalSwipe) {
    this._direction = aIsVerticalSwipe ? "vertical" : "horizontal";

    if (this.isAnimationRunning()) {
      
      
      
      
      
      
      
      if (this._direction == "horizontal" || this._lastSwipeDir != "") {
        gBrowser.stop();
        this._lastSwipeDir = "RELOAD"; 
        this._canGoBack = this.canGoBack();
        this._canGoForward = this.canGoForward();
        this._handleFastSwiping();
      }
    }
    else {
      this._startingIndex = gBrowser.webNavigation.sessionHistory.index;
      this._historyIndex = this._startingIndex;
      this._canGoBack = this.canGoBack();
      this._canGoForward = this.canGoForward();
      if (this.active) {
        this._addBoxes();
        this._takeSnapshot();
        this._installPrevAndNextSnapshots();
        this._lastSwipeDir = "";
      }
    }
    this.updateAnimation(0);
  },

  


  stopAnimation: function HSA_stopAnimation() {
    gHistorySwipeAnimation._removeBoxes();
    this._historyIndex = gBrowser.webNavigation.sessionHistory.index;
  },

  






  updateAnimation: function HSA_updateAnimation(aVal) {
    if (!this.isAnimationRunning()) {
      return;
    }

    
    
    
    let dampValue = 4;
    if (this._direction == "vertical") {
      this._prevBox.collapsed = true;
      this._nextBox.collapsed = true;
      this._positionBox(this._curBox, -1 * aVal / dampValue);
    } else if ((aVal >= 0 && this.isLTR) ||
               (aVal <= 0 && !this.isLTR)) {
      let tempDampValue = 1;
      if (this._canGoBack) {
        this._prevBox.collapsed = false;
      } else {
        tempDampValue = dampValue;
        this._prevBox.collapsed = true;
      }

      
      
      
      this._positionBox(this._curBox, aVal / tempDampValue);

      
      this._positionBox(this._nextBox, 1);
    } else {
      
      
      
      
      
      
      if (this._canGoForward) {
        this._nextBox.collapsed = false;
        let offset = this.isLTR ? 1 : -1;
        this._positionBox(this._curBox, 0);
        this._positionBox(this._nextBox, offset + aVal);
      } else {
        this._prevBox.collapsed = true;
        this._positionBox(this._curBox, aVal / dampValue);
      }
    }
  },

  





  handleEvent: function HSA_handleEvent(aEvent) {
    let browser = gBrowser.selectedBrowser;
    switch (aEvent.type) {
      case "TabClose":
        let browserForTab = gBrowser.getBrowserForTab(aEvent.target);
        this._removeTrackedSnapshot(-1, browserForTab);
        break;
      case "DOMModalDialogClosed":
        this.stopAnimation();
        break;
      case "pageshow":
        if (aEvent.target == browser.contentDocument) {
          this.stopAnimation();
        }
        break;
      case "popstate":
        if (aEvent.target == browser.contentDocument.defaultView) {
          this.stopAnimation();
        }
        break;
      case "pagehide":
        if (aEvent.target == browser.contentDocument) {
          
          
          
          if (!this.isAnimationRunning()) {
            this._takeSnapshot();
          }
          this._compressSnapshotAtCurrentIndex();
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
    if (this._lastSwipeDir != "" && this._historyIndex != this._startingIndex)
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
    if (this._doesIndexExistInHistory(this._historyIndex))
      gBrowser.webNavigation.gotoIndex(this._historyIndex);
    else
      this.stopAnimation();
  },

  





  _isSupported: function HSA__isSupported() {
    return window.matchMedia("(-moz-swipe-animation-enabled)").matches;
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
    this._boxHeight = this._curBox.getBoundingClientRect().height;
  },

  


  _removeBoxes: function HSA__removeBoxes() {
    this._curBox = null;
    this._prevBox = null;
    this._nextBox = null;
    if (this._container)
      this._container.parentNode.removeChild(this._container);
    this._container = null;
    this._boxWidth = -1;
    this._boxHeight = -1;
  },

  








  _createElement: function HSA__createElement(aID, aTagName) {
    let XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    let element = document.createElementNS(XULNS, aTagName);
    element.id = aID;
    return element;
  },

  







  _positionBox: function HSA__positionBox(aBox, aPosition) {
    let transform = "";

    if (this._direction == "vertical")
      transform = "translateY(" + this._boxHeight * aPosition + "px)";
    else
      transform = "translateX(" + this._boxWidth * aPosition + "px)";

    aBox.style.transform = transform;
  },

  





  _readyToTakeSnapshots: function HSA__readyToTakeSnapshots() {
    if ((this._maxSnapshots < 1) ||
        (gBrowser.webNavigation.sessionHistory.index < 0)) {
      return false;
    }
    return true;
  },

  


  _takeSnapshot: function HSA__takeSnapshot() {
    if (!this._readyToTakeSnapshots()) {
      return;
    }

    let canvas = null;

    TelemetryStopwatch.start("FX_GESTURE_TAKE_SNAPSHOT_OF_PAGE");
    try {
      let browser = gBrowser.selectedBrowser;
      let r = browser.getBoundingClientRect();
      canvas = document.createElementNS("http://www.w3.org/1999/xhtml",
                                        "canvas");
      canvas.mozOpaque = true;
      let scale = window.devicePixelRatio;
      canvas.width = r.width * scale;
      canvas.height = r.height * scale;
      let ctx = canvas.getContext("2d");
      let zoom = browser.markupDocumentViewer.fullZoom * scale;
      ctx.scale(zoom, zoom);
      ctx.drawWindow(browser.contentWindow,
                     0, 0, canvas.width / zoom, canvas.height / zoom, "white",
                     ctx.DRAWWINDOW_DO_NOT_FLUSH | ctx.DRAWWINDOW_DRAW_VIEW |
                     ctx.DRAWWINDOW_ASYNC_DECODE_IMAGES |
                     ctx.DRAWWINDOW_USE_WIDGET_LAYERS);
    } finally {
      TelemetryStopwatch.finish("FX_GESTURE_TAKE_SNAPSHOT_OF_PAGE");
    }

    TelemetryStopwatch.start("FX_GESTURE_INSTALL_SNAPSHOT_OF_PAGE");
    try {
      this._installCurrentPageSnapshot(canvas);
      this._assignSnapshotToCurrentBrowser(canvas);
    } finally {
      TelemetryStopwatch.finish("FX_GESTURE_INSTALL_SNAPSHOT_OF_PAGE");
    }
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
    
    
    
    snapshots[currIndex] = {
      image: aCanvas,
      scale: window.devicePixelRatio
    };
  },

  



  _compressSnapshotAtCurrentIndex:
  function HSA__compressSnapshotAtCurrentIndex() {
    if (!this._readyToTakeSnapshots()) {
      
      
      return;
    }

    TelemetryStopwatch.start("FX_GESTURE_COMPRESS_SNAPSHOT_OF_PAGE");
    try {
      let browser = gBrowser.selectedBrowser;
      let snapshots = browser.snapshots;
      let currIndex = browser.webNavigation.sessionHistory.index;

      
      let canvas = snapshots[currIndex].image;
      canvas.toBlob(function(aBlob) {
          if (snapshots[currIndex]) {
            snapshots[currIndex].image = aBlob;
          }
        }, "image/png"
      );
    } finally {
      TelemetryStopwatch.finish("FX_GESTURE_COMPRESS_SNAPSHOT_OF_PAGE");
    }
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
      delete lastElem.browser.snapshots[lastElem.index].image;
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
    let url = "";
    try {
      url = URL.createObjectURL(aBlob);
      img.onload = function() {
        URL.revokeObjectURL(url);
      };
    }
    finally {
      img.src = url;
      return img;
    }
  },

  









  _scaleSnapshot: function HSA__scaleSnapshot(aSnapshot, aScale, aBox) {
    if (aSnapshot && aScale != 1 && aBox) {
      if (aSnapshot instanceof HTMLCanvasElement) {
        aBox.style.backgroundSize =
          aSnapshot.width / aScale + "px " + aSnapshot.height / aScale + "px";
      } else {
        
        aSnapshot.addEventListener("load", function() {
          aBox.style.backgroundSize =
            aSnapshot.width / aScale + "px " + aSnapshot.height / aScale + "px";
        });
      }
    }
  },

  








  _installCurrentPageSnapshot:
  function HSA__installCurrentPageSnapshot(aCanvas) {
    let currSnapshot = aCanvas;
    let scale = window.devicePixelRatio;
    if (!currSnapshot) {
      let snapshots = gBrowser.selectedBrowser.snapshots || {};
      let currIndex = this._historyIndex;
      if (currIndex in snapshots) {
        currSnapshot = this._convertToImg(snapshots[currIndex].image);
        scale = snapshots[currIndex].scale;
      }
    }
    this._scaleSnapshot(currSnapshot, scale, this._curBox ? this._curBox :
                                                            null);
    document.mozSetImageElement("historySwipeAnimationCurrentPageSnapshot",
                                currSnapshot);
  },

  



  _installPrevAndNextSnapshots:
  function HSA__installPrevAndNextSnapshots() {
    let snapshots = gBrowser.selectedBrowser.snapshots || [];
    let currIndex = this._historyIndex;
    let prevIndex = currIndex - 1;
    let prevSnapshot = null;
    if (prevIndex in snapshots) {
      prevSnapshot = this._convertToImg(snapshots[prevIndex].image);
      this._scaleSnapshot(prevSnapshot, snapshots[prevIndex].scale,
                          this._prevBox);
    }
    document.mozSetImageElement("historySwipeAnimationPreviousPageSnapshot",
                                prevSnapshot);

    let nextIndex = currIndex + 1;
    let nextSnapshot = null;
    if (nextIndex in snapshots) {
      nextSnapshot = this._convertToImg(snapshots[nextIndex].image);
      this._scaleSnapshot(nextSnapshot, snapshots[nextIndex].scale,
                          this._nextBox);
    }
    document.mozSetImageElement("historySwipeAnimationNextPageSnapshot",
                                nextSnapshot);
  },
};
