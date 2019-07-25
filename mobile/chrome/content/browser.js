let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;
let Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm")




const kPanDeceleration = 0.999;


const kSwipeLength = 500;



const kDragThreshold = 10;


const kLockBreakThreshold = 100;



const kMinKineticSpeed = 0.015;



const kMaxKineticSpeed = 9;



const kAxisLockRatio = 5;

function dump(a) {
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).logStringMessage(a);
}

function sendMessageToJava(aMessage) {
  let bridge = Cc["@mozilla.org/android/bridge;1"].getService(Ci.nsIAndroidBridge);
  return bridge.handleGeckoMessage(JSON.stringify(aMessage));
}

function resolveGeckoURI(aURI) {
  if (aURI.indexOf("chrome://") == 0) {
    let registry = Cc['@mozilla.org/chrome/chrome-registry;1'].getService(Ci["nsIChromeRegistry"]);
    return registry.convertChromeURL(Services.io.newURI(aURI, null, null)).spec;
  } else if (aURI.indexOf("resource://") == 0) {
    let handler = Services.io.getProtocolHandler("resource").QueryInterface(Ci.nsIResProtocolHandler);
    return handler.resolveURI(Services.io.newURI(aURI, null, null));
  }
  return aURI;
}

var BrowserApp = {
  _tabs: [],
  _selectedTab: null,

  deck: null,

  startup: function startup() {
    window.QueryInterface(Ci.nsIDOMChromeWindow).browserDOMWindow = new nsBrowserAccess();
    dump("zerdatime " + Date.now() + " - browser chrome startup finished.");

    this.deck = document.getElementById("browsers");
    BrowserEventHandler.init();

    Services.obs.addObserver(this, "Tab:Add", false);
    Services.obs.addObserver(this, "Tab:Load", false);
    Services.obs.addObserver(this, "Tab:Select", false);
    Services.obs.addObserver(this, "Tab:Close", false);
    Services.obs.addObserver(this, "session-back", false);
    Services.obs.addObserver(this, "session-reload", false);

    NativeWindow.init();

    let uri = "about:support";
    if ("arguments" in window && window.arguments[0])
      uri = window.arguments[0];

    
    Services.io.offline = false;
    let newTab = this.addTab(uri);
    newTab.active = true;

    Downloads.init();

    
    let event = document.createEvent("Events");
    event.initEvent("UIReady", true, false);
    window.dispatchEvent(event);
  },

  shutdown: function shutdown() {
    NativeWindow.uninit();
  },

  get tabs() {
    return this._tabs;
  },

  get selectedTab() {
    return this._selectedTab;
  },

  set selectedTab(aTab) {
    this._selectedTab = aTab;
    if (!aTab)
      return;

    this.deck.selectedPanel = aTab.browser;
  },

  get selectedBrowser() {
    if (this._selectedTab)
      return this._selectedTab.browser;
    return null;
  },

  getTabForId: function getTabForId(aId) {
    let tabs = this._tabs;
    for (let i=0; i < tabs.length; i++) {
       if (tabs[i].id == aId)
         return tabs[i];
    }
    return null;
  },

  getTabForBrowser: function getTabForBrowser(aBrowser) {
    let tabs = this._tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser == aBrowser)
        return tabs[i];
    }
    return null;
  },

  getBrowserForWindow: function getBrowserForWindow(aWindow) {
    let tabs = this._tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser.contentWindow == aWindow)
        return tabs[i].browser;
    }
    return null;
  },

  getBrowserForDocument: function getBrowserForDocument(aDocument) {
    let tabs = this._tabs;
    for (let i = 0; i < tabs.length; i++) {
      if (tabs[i].browser.contentDocument == aDocument)
        return tabs[i].browser;
    }
    return null;
  },

  loadURI: function loadURI(aURI, aParams) {
    let browser = this.selectedBrowser;
    if (!browser)
      return;

    let flags = aParams.flags || Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    let postData = ("postData" in aParams && aParams.postData) ? aParams.postData.value : null;
    let referrerURI = "referrerURI" in aParams ? aParams.referrerURI : null;
    let charset = "charset" in aParams ? aParams.charset : null;
    browser.loadURIWithFlags(aURI, flags, referrerURI, charset, postData);
  },

  addTab: function addTab(aURI) {
    let newTab = new Tab(aURI);
    this._tabs.push(newTab);
    return newTab;
  },

  closeTab: function closeTab(aTab) {
    if (aTab == this.selectedTab)
      this.selectedTab = null;

    aTab.destroy();
    this._tabs.splice(this._tabs.indexOf(aTab), 1);
  },

  selectTab: function selectTab(aTab) {
    if (aTab != null) {
      this.selectedTab = aTab;
      aTab.active = true;
      let message = {
        gecko: {
          type: "Tab:Selected",
          tabID: aTab.id
        }
      };

      sendMessageToJava(message);
    }
  },

  observe: function(aSubject, aTopic, aData) {
    let browser = this.selectedBrowser;
    if (!browser)
      return;

    if (aTopic == "session-back")
      browser.goBack();
    else if (aTopic == "session-reload")
      browser.reload();
    else if (aTopic == "Tab:Add") {
      let newTab = this.addTab(aData);
      newTab.active = true;
    } else if (aTopic == "Tab:Load") 
      browser.loadURI(aData);
    else if (aTopic == "Tab:Select") 
      this.selectTab(this.getTabForId(parseInt(aData)));
    else if (aTopic == "Tab:Close")
      this.closeTab(this.getTabForId(parseInt(aData)));
  }
}

var NativeWindow = {
  init: function() {
    Services.obs.addObserver(this, "Menu:Clicked", false);
  },

  uninit: function() {
    Services.obs.removeObserver(this, "Menu:Clicked");
  },

  menu: {
    _callbacks: [],
    _menuId: 0,
    add: function(aName, aIcon, aCallback) {
      sendMessageToJava({ gecko: {type: "Menu:Add", name: aName, icon: aIcon, id: this._menuId }});
      this._callbacks[this._menuId] = aCallback;
      this._menuId++;
      return this._menuId - 1;
    },

    remove: function(aId) {
      sendMessageToJava({ gecko: {type: "Menu:Remove", id: aId }});
    }
  },

  observe: function(aSubject, aTopic, aData) {
    if (this.menu._callbacks[aData])
      this.menu._callbacks[aData]();
  }
};


function nsBrowserAccess() {
}

nsBrowserAccess.prototype = {
  openURI: function browser_openURI(aURI, aOpener, aWhere, aContext) {
    dump("nsBrowserAccess::openURI");
    let browser = BrowserApp.selectedBrowser;
    if (!browser)
      browser = BrowserApp.addTab("about:blank").browser;

    
    Services.io.offline = false;
    browser.loadURI(aURI.spec, null, null);
    return null;
  },

  openURIInFrame: function browser_openURIInFrame(aURI, aOpener, aWhere, aContext) {
    dump("nsBrowserAccess::openURIInFrame");
    return null;
  },

  isTabContentWindow: function(aWindow) {
    return BrowserApp.getBrowserForWindow(aWindow) != null;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBrowserDOMWindow])
};


let gTabIDFactory = 0;

function Tab(aURL) {
  this.filter = {
    percentage: 0,
    requestsFinished: 0,
    requestsTotal: 0
  };

  this.browser = null;
  this.id = 0;
  this.create(aURL);
}

Tab.prototype = {
  create: function(aURL) {
    if (this.browser)
      return;

    this.browser = document.createElement("browser");
    this.browser.setAttribute("type", "content");
    BrowserApp.deck.appendChild(this.browser);
    this.browser.stop();

    let flags = Ci.nsIWebProgress.NOTIFY_STATE_ALL |
                Ci.nsIWebProgress.NOTIFY_LOCATION |
                Ci.nsIWebProgress.NOTIFY_PROGRESS;
    this.browser.addProgressListener(this, flags);
    this.browser.loadURI(aURL);

    this.id = ++gTabIDFactory;
    let message = {
      gecko: {
        type: "Tab:Added",
        tabID: this.id,
        uri: aURL
      }
    };

    sendMessageToJava(message);
  },

  destroy: function() {
    if (!this.browser)
      return;

    this.browser.removeProgressListener(this);
    BrowserApp.deck.removeChild(this.browser);
    this.browser = null;
    let message = {
      gecko: {
        type: "Tab:Closed",
        tabID: this.id
      }
    };

    sendMessageToJava(message);
  },

  set active(aActive) {
    if (!this.browser)
      return;

    if (aActive) {
      this.browser.setAttribute("type", "content-primary");
      this.browser.focus();
      BrowserApp.selectedTab = this;
    } else {
      this.browser.setAttribute("type", "content");
    }
  },

  get active() {
    if (!this.browser)
      return false;
    return this.browser.getAttribute("type") == "content-primary";
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_START) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK) {
        
        this.filter = {
          percentage: 0,
          requestsFinished: 0,
          requestsTotal: 0
        };
      }
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_REQUEST)
        
        
        ++this.filter.requestsTotal;
    }
    else if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_REQUEST) {
        
        
        ++this.filter.requestsFinished;
        this.onProgressChange(aWebProgress, null, 0, 0, this.filter.requestsFinished, this.filter.requestsTotal);
      }
    }

    if (aStateFlags & Ci.nsIWebProgressListener.STATE_IS_DOCUMENT) {
      
      let browser = BrowserApp.getBrowserForWindow(aWebProgress.DOMWindow);
      let uri = "";
      if (browser)
        uri = browser.currentURI.spec;
  
      let message = {
        gecko: {
          type: "onStateChange",
          tabID: this.id,
          uri: uri,
          state: aStateFlags
        }
      };
  
      sendMessageToJava(message);
    }
  },

  onLocationChange: function(aWebProgress, aRequest, aLocationURI) {
    let browser = BrowserApp.getBrowserForWindow(aWebProgress.DOMWindow);
    let uri = browser.currentURI.spec;

    let message = {
      gecko: {
        type: "onLocationChange",
        tabID: this.id,
        uri: uri
      }
    };

    sendMessageToJava(message);
  },

  onSecurityChange: function(aBrowser, aWebProgress, aRequest, aState) {
  },

  onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress) {
    
    if (aCurTotalProgress > aMaxTotalProgress || aMaxTotalProgress <= 0)
      return;

    
    if (this.filter.requestsTotal > 1 && aRequest)
      return;

    
    let percentage = (aCurTotalProgress * 100) / aMaxTotalProgress;
    if (percentage > this.filter.percentage + 3) {
      this.filter.percentage = percentage;

      let message = {
        gecko: {
          type: "onProgressChange",
          tabID: this.id,
          current: aCurTotalProgress,
          total: aMaxTotalProgress
        }
      };
  
      sendMessageToJava(message);
    }
  },

  onStatusChange: function(aBrowser, aWebProgress, aRequest, aStatus, aMessage) {
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener, Ci.nsISupportsWeakReference])
};


var BrowserEventHandler = {
  init: function init() {
    window.addEventListener("click", this, true);
    window.addEventListener("mousedown", this, true);
    window.addEventListener("mouseup", this, true);
    window.addEventListener("mousemove", this, true);

    BrowserApp.deck.addEventListener("MozMagnifyGestureStart", this, true);
    BrowserApp.deck.addEventListener("MozMagnifyGestureUpdate", this, true);
    BrowserApp.deck.addEventListener("DOMContentLoaded", this, true);
    BrowserApp.deck.addEventListener("DOMLinkAdded", this, true);
    BrowserApp.deck.addEventListener("DOMTitleChanged", this, true);
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "DOMContentLoaded": {
        let browser = BrowserApp.getBrowserForDocument(aEvent.target);
        browser.focus();

        let tabID = BrowserApp.getTabForBrowser(browser).id;

        sendMessageToJava({
          gecko: {
            type: "DOMContentLoaded",
            tabID: tabID,
            windowID: 0,
            uri: browser.currentURI.spec,
            title: browser.contentTitle
          }
        });
        break;
      }

      case "DOMLinkAdded": {
        let target = aEvent.originalTarget;
        if (!target.href || target.disabled)
          return;

        let browser = BrowserApp.getBrowserForDocument(target.ownerDocument); 
        let tabID = BrowserApp.getTabForBrowser(browser).id;

        let json = {
          type: "DOMLinkAdded",
          tabID: tabID,
          windowId: target.ownerDocument.defaultView.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID,
          href: resolveGeckoURI(target.href),
          charset: target.ownerDocument.characterSet,
          title: target.title,
          rel: target.rel
        };
        
        
        if (target.hasAttribute("sizes"))
          json.sizes = target.getAttribute("sizes");

        sendMessageToJava({ gecko: json });
        break;
      }

      case "DOMTitleChanged": {
        if (!aEvent.isTrusted)
          return;

        let contentWin = aEvent.target.defaultView;
        if (contentWin != contentWin.top)
          return;

        let browser = BrowserApp.getBrowserForDocument(aEvent.target);
        if (!browser)
          return;

        let tabID = BrowserApp.getTabForBrowser(browser).id;
        sendMessageToJava({
          gecko: {
            type: "DOMTitleChanged",
            tabID: tabID,
            title: aEvent.target.title
          }
        });
        break;
      }

      case "click":
        if (this.blockClick) {
          aEvent.stopPropagation();
          aEvent.preventDefault();
        }
        break;

      case "mousedown":
        this.startX = aEvent.clientX;
        this.startY = aEvent.clientY;
        this.blockClick = false;

        this.firstMovement = true;
        this.edx = 0;
        this.edy = 0;
        this.lockXaxis = false;
        this.lockYaxis = false;

        this.motionBuffer = [];
        this._updateLastPosition(aEvent.clientX, aEvent.clientY, 0, 0);
        this.panElement = this._findScrollableElement(aEvent.originalTarget,
                                                      true);

        if (this.panElement)
          this.panning = true;
        break;

      case "mousemove":
        aEvent.stopPropagation();
        aEvent.preventDefault();

        if (!this.panning)
          break;

        this.edx += aEvent.clientX - this.lastX;
        this.edy += aEvent.clientY - this.lastY;

        
        
        
        
        
        if (this.firstMovement &&
            (Math.abs(this.edx) > kDragThreshold ||
             Math.abs(this.edy) > kDragThreshold)) {
          this.firstMovement = false;
          let originalElement = this.panElement;

          
          if (Math.abs(this.edx) > Math.abs(this.edy) * kAxisLockRatio)
            this.lockYaxis = true;
          else if (Math.abs(this.edy) > Math.abs(this.edx) * kAxisLockRatio)
            this.lockXaxis = true;

          
          
          while (this.panElement &&
                 !this._elementCanScroll(this.panElement,
                                         -this.edx,
                                         -this.edy)) {
            this.panElement =
              this._findScrollableElement(this.panElement, false);
          }

          
          
          if (!this.panElement)
            this.panElement = originalElement;
        }

        
        if (!this.firstMovement) {
          this._scrollElementBy(this.panElement,
                                this.lockXaxis ? 0 : -this.edx,
                                this.lockYaxis ? 0 : -this.edy);

          
          
          
          this._updateLastPosition(aEvent.clientX, aEvent.clientY,
                                   this.lockXaxis ? 0 : this.edx,
                                   this.lockYaxis ? 0 : this.edy);

          
          if (this.lockXaxis) {
            if (Math.abs(this.edx) > kLockBreakThreshold)
              this.lockXaxis = false;
          } else {
            this.edx = 0;
          }

          if (this.lockYaxis) {
            if (Math.abs(this.edy) > kLockBreakThreshold)
              this.lockYaxis = false;
          } else {
            this.edy = 0;
          }
        }
        break;

      case "mouseup":
        if (!this.panning)
          break;

        this.panning = false;

        if (Math.abs(aEvent.clientX - this.startX) > kDragThreshold ||
            Math.abs(aEvent.clientY - this.startY) > kDragThreshold) {
          this.blockClick = true;
          aEvent.stopPropagation();
          aEvent.preventDefault();

          
          
          
          this.panLastTime = Date.now();

          
          
          
          
          
          
          let xSums = { p: 0, t: 0, tp: 0, tt: 0, n: 0 };
          let ySums = { p: 0, t: 0, tp: 0, tt: 0, n: 0 };
          let lastDx = 0;
          let lastDy = 0;

          
          let edx = 0; 
          let edy = 0; 

          
          let mb = this.motionBuffer;

          
          for (let i = 0; i < mb.length; i++) {

            
            let dx = edx + mb[i].dx;
            let dy = edy + mb[i].dy;
            edx += mb[i].dx;
            edy += mb[i].dy;

            
            if ((xSums.n > 0) &&
                ((mb[i].dx < 0 && lastDx > 0) ||
                 (mb[i].dx > 0 && lastDx < 0))) {
              xSums = { p: 0, t: 0, tp: 0, tt: 0, n: 0 };
            }
            if ((ySums.n > 0) &&
                ((mb[i].dy < 0 && lastDy > 0) ||
                 (mb[i].dy > 0 && lastDy < 0))) {
              ySums = { p: 0, t: 0, tp: 0, tt: 0, n: 0 };
            }

            if (mb[i].dx != 0)
              lastDx = mb[i].dx;
            if (mb[i].dy != 0)
              lastDy = mb[i].dy;

            
            let timeDelta = this.panLastTime - mb[i].time;
            if (timeDelta > kSwipeLength)
              continue;

            xSums.p += dx;
            xSums.t += timeDelta;
            xSums.tp += timeDelta * dx;
            xSums.tt += timeDelta * timeDelta;
            xSums.n ++;

            ySums.p += dy;
            ySums.t += timeDelta;
            ySums.tp += timeDelta * dy;
            ySums.tt += timeDelta * timeDelta;
            ySums.n ++;
          }

          
          if (xSums.n < 2 && ySums.n < 2)
            break;

          
          
          let sx = 0;
          if (xSums.n > 1)
            sx = ((xSums.n * xSums.tp) - (xSums.t * xSums.p)) /
                 ((xSums.n * xSums.tt) - (xSums.t * xSums.t));
          

          let sy = 0;
          if (ySums.n > 1)
            sy = ((ySums.n * ySums.tp) - (ySums.t * ySums.p)) /
                 ((ySums.n * ySums.tt) - (ySums.t * ySums.t));
          

          
          this.panX = -sx;
          this.panY = -sy;

          if (Math.abs(this.panX) > kMaxKineticSpeed)
            this.panX = (this.panX > 0) ? kMaxKineticSpeed : -kMaxKineticSpeed;
          if (Math.abs(this.panY) > kMaxKineticSpeed)
            this.panY = (this.panY > 0) ? kMaxKineticSpeed : -kMaxKineticSpeed;

          
          if (Math.abs(this.panX) < kMinKineticSpeed &&
              Math.abs(this.panY) < kMinKineticSpeed)
            break;

          
          if (!this.panElement || !this._elementCanScroll(
                 this.panElement, -this.panX, -this.panY))
            break;

          
          this._scrollElementBy(this.panElement, -this.panX, -this.panY);

          
          this.panAccumulatedDeltaX = 0;
          this.panAccumulatedDeltaY = 0;

          let self = this;
          let panElement = this.panElement;
          let callback = {
            onBeforePaint: function kineticPanCallback(timeStamp) {
              if (self.panning || self.panElement != panElement)
                return;

              let timeDelta = timeStamp - self.panLastTime;
              self.panLastTime = timeStamp;

              
              self.panX *= Math.pow(kPanDeceleration, timeDelta);
              self.panY *= Math.pow(kPanDeceleration, timeDelta);

              
              let dx = self.panX * timeDelta;
              let dy = self.panY * timeDelta;

              
              self.panAccumulatedDeltaX += dx - Math.floor(dx);
              self.panAccumulatedDeltaY += dy - Math.floor(dy);

              dx = Math.floor(dx);
              dy = Math.floor(dy);

              if (Math.abs(self.panAccumulatedDeltaX) >= 1.0) {
                  let adx = Math.floor(self.panAccumulatedDeltaX);
                  dx += adx;
                  self.panAccumulatedDeltaX -= adx;
              }

              if (Math.abs(self.panAccumulatedDeltaY) >= 1.0) {
                  let ady = Math.floor(self.panAccumulatedDeltaY);
                  dy += ady;
                  self.panAccumulatedDeltaY -= ady;
              }

              self._scrollElementBy(panElement, -dx, -dy);

              if (Math.abs(self.panX) >= kMinKineticSpeed ||
                  Math.abs(self.panY) >= kMinKineticSpeed)
                window.mozRequestAnimationFrame(this);
              }
          };

          
          if (Math.abs(this.panX) < Math.abs(this.panY) / kAxisLockRatio)
            this.panX = 0;
          else if (Math.abs(this.panY) < Math.abs(this.panX) / kAxisLockRatio)
            this.panY = 0;

          
          window.mozRequestAnimationFrame(callback);
        }
        break;

      case "MozMagnifyGestureStart":
        this._pinchDelta = 0;
        this.zoomCallbackFired = true;
        break;

      case "MozMagnifyGestureUpdate":
        if (!aEvent.delta)
          break;
  
        this._pinchDelta += aEvent.delta;

        if ((Math.abs(this._pinchDelta) >= 1) && this.zoomCallbackFired) {
          
          
          
          
          
          let currentZoom = BrowserApp.selectedBrowser.markupDocumentViewer.fullZoom;
          let currentSize = Math.sqrt(Math.pow(window.innerWidth, 2) + Math.pow(window.innerHeight, 2));
          let newZoom = ((currentSize * currentZoom) + this._pinchDelta) / currentSize;

          let self = this;
          let callback = {
            onBeforePaint: function zoomCallback(timeStamp) {
              BrowserApp.selectedBrowser.markupDocumentViewer.fullZoom = newZoom;
              self.zoomCallbackFired = true;
            }
          };

          this._pinchDelta = 0;

          
          this.zoomCallbackFired = false;
          window.mozRequestAnimationFrame(callback);
        }
        break;
    }
  },

  _updateLastPosition: function(x, y, dx, dy) {
    this.lastX = x;
    this.lastY = y;
    this.lastTime = Date.now();

    this.motionBuffer.push({ dx: dx, dy: dy, time: this.lastTime });
  },

  _findScrollableElement: function(elem, checkElem) {
    
    let scrollable = false;
    while (elem) {
      




      if (checkElem) {
        if (((elem.scrollHeight > elem.clientHeight) ||
             (elem.scrollWidth > elem.clientWidth)) &&
            (elem.style.overflow == 'auto' ||
             elem.style.overflow == 'scroll' ||
             elem.localName == 'textarea' ||
             elem.localName == 'html' ||
             elem.localName == 'body')) {
          scrollable = true;
          break;
        }
      } else {
        checkElem = true;
      }

      
      if (!elem.parentNode && elem.documentElement &&
          elem.documentElement.ownerDocument)
        elem = elem.documentElement.ownerDocument.defaultView.frameElement;
      else
        elem = elem.parentNode;
    }

    if (!scrollable)
      return null;

    return elem;
  },

  _scrollElementBy: function(elem, x, y) {
    elem.scrollTop = elem.scrollTop + y;
    elem.scrollLeft = elem.scrollLeft + x;
  },

  _elementCanScroll: function(elem, x, y) {
    let scrollX = true;
    let scrollY = true;

    if (x < 0) {
      if (elem.scrollLeft <= 0) {
        scrollX = false;
      }
    } else if (elem.scrollLeft >= (elem.scrollWidth - elem.clientWidth)) {
      scrollX = false;
    }

    if (y < 0) {
      if (elem.scrollTop <= 0) {
        scrollY = false;
      }
    } else if (elem.scrollTop >= (elem.scrollHeight - elem.clientHeight)) {
      scrollY = false;
    }

    return scrollX || scrollY;
  }
};
