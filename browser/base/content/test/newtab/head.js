


const PREF_NEWTAB_ENABLED = "browser.newtabpage.enabled";
const PREF_NEWTAB_DIRECTORYSOURCE = "browser.newtabpage.directory.source";

Services.prefs.setBoolPref(PREF_NEWTAB_ENABLED, true);

let tmp = {};
Cu.import("resource://gre/modules/Promise.jsm", tmp);
Cu.import("resource://gre/modules/NewTabUtils.jsm", tmp);
Cu.import("resource://gre/modules/DirectoryLinksProvider.jsm", tmp);
Cc["@mozilla.org/moz/jssubscript-loader;1"]
  .getService(Ci.mozIJSSubScriptLoader)
  .loadSubScript("chrome://browser/content/sanitize.js", tmp);
Cu.import("resource://gre/modules/Timer.jsm", tmp);
let {Promise, NewTabUtils, Sanitizer, clearTimeout, DirectoryLinksProvider} = tmp;

let uri = Services.io.newURI("about:newtab", null, null);
let principal = Services.scriptSecurityManager.getNoAppCodebasePrincipal(uri);

let isMac = ("nsILocalFileMac" in Ci);
let isLinux = ("@mozilla.org/gnome-gconf-service;1" in Cc);
let isWindows = ("@mozilla.org/windows-registry-key;1" in Cc);
let gWindow = window;


let gDirectorySource = 'data:application/json,{"test":1}';



let requiredSize = {};
requiredSize.innerHeight =
  40 + 32 + 
  44 + 32 + 
  (3 * (180 + 32)) + 
  100; 
requiredSize.innerWidth =
  (3 * (290 + 20)) + 
  100; 

let oldSize = {};
Object.keys(requiredSize).forEach(prop => {
  info([prop, gBrowser.contentWindow[prop], requiredSize[prop]]);
  if (gBrowser.contentWindow[prop] < requiredSize[prop]) {
    oldSize[prop] = gBrowser.contentWindow[prop];
    info("Changing browser " + prop + " from " + oldSize[prop] + " to " +
         requiredSize[prop]);
    gBrowser.contentWindow[prop] = requiredSize[prop];
  }
});
let (screenHeight = {}, screenWidth = {}) {
  Cc["@mozilla.org/gfx/screenmanager;1"].
    getService(Ci.nsIScreenManager).
    primaryScreen.
    GetAvailRectDisplayPix({}, {}, screenWidth, screenHeight);
  screenHeight = screenHeight.value;
  screenWidth = screenWidth.value;
  if (screenHeight < gBrowser.contentWindow.outerHeight) {
    info("Warning: Browser outer height is now " +
         gBrowser.contentWindow.outerHeight + ", which is larger than the " +
         "available screen height, " + screenHeight +
         ". That may cause problems.");
  }
  if (screenWidth < gBrowser.contentWindow.outerWidth) {
    info("Warning: Browser outer width is now " +
         gBrowser.contentWindow.outerWidth + ", which is larger than the " +
         "available screen width, " + screenWidth +
         ". That may cause problems.");
  }
}

registerCleanupFunction(function () {
  while (gWindow.gBrowser.tabs.length > 1)
    gWindow.gBrowser.removeTab(gWindow.gBrowser.tabs[1]);

  Object.keys(oldSize).forEach(prop => {
    if (oldSize[prop]) {
      gBrowser.contentWindow[prop] = oldSize[prop];
    }
  });

  
  let timer = NewTabUtils.allPages._scheduleUpdateTimeout;
  if (timer) {
    clearTimeout(timer);
    delete NewTabUtils.allPages._scheduleUpdateTimeout;
  }

  Services.prefs.clearUserPref(PREF_NEWTAB_ENABLED);
  Services.prefs.clearUserPref(PREF_NEWTAB_DIRECTORYSOURCE);

  return watchLinksChangeOnce();
});




function watchLinksChangeOnce() {
  let deferred = Promise.defer();
  let observer = {
    onManyLinksChanged: () => {
      DirectoryLinksProvider.removeObserver(observer);
      deferred.resolve();
    }
  };
  observer.onDownloadFail = observer.onManyLinksChanged;
  DirectoryLinksProvider.addObserver(observer);
  return deferred.promise;
};




function test() {
  waitForExplicitFinish();
  
  watchLinksChangeOnce().then(() => {
    
    whenPagesUpdated(() => TestRunner.run(), true);
  });
  Services.prefs.setCharPref(PREF_NEWTAB_DIRECTORYSOURCE, gDirectorySource);
}




let TestRunner = {
  


  run: function () {
    this._iter = runTests();
    this.next();
  },

  


  next: function () {
    try {
      TestRunner._iter.next();
    } catch (e if e instanceof StopIteration) {
      TestRunner.finish();
    }
  },

  


  finish: function () {
    function cleanupAndFinish() {
      clearHistory(function () {
        whenPagesUpdated(finish);
        NewTabUtils.restore();
      });
    }

    let callbacks = NewTabUtils.links._populateCallbacks;
    let numCallbacks = callbacks.length;

    if (numCallbacks)
      callbacks.splice(0, numCallbacks, cleanupAndFinish);
    else
      cleanupAndFinish();
  }
};





function getContentWindow() {
  return gWindow.gBrowser.selectedBrowser.contentWindow;
}





function getContentDocument() {
  return gWindow.gBrowser.selectedBrowser.contentDocument;
}





function getGrid() {
  return getContentWindow().gGrid;
}






function getCell(aIndex) {
  return getGrid().cells[aIndex];
}












function setLinks(aLinks) {
  let links = aLinks;

  if (typeof links == "string") {
    links = aLinks.split(/\s*,\s*/).map(function (id) {
      return {url: "http://example" + (id != "-1" ? id : "") + ".com/",
              title: "site#" + id};
    });
  }

  
  
  
  
  NewTabUtils.links.populateCache(function () {
    clearHistory(function () {
      fillHistory(links, function () {
        NewTabUtils.links.populateCache(function () {
          NewTabUtils.allPages.update();
          TestRunner.next();
        }, true);
      });
    });
  });
}

function clearHistory(aCallback) {
  Services.obs.addObserver(function observe(aSubject, aTopic, aData) {
    Services.obs.removeObserver(observe, aTopic);
    executeSoon(aCallback);
  }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);

  PlacesUtils.history.removeAllPages();
}

function fillHistory(aLinks, aCallback) {
  let numLinks = aLinks.length;
  if (!numLinks) {
    if (aCallback)
      executeSoon(aCallback);
    return;
  }

  let transitionLink = Ci.nsINavHistoryService.TRANSITION_LINK;

  
  
  let now = Date.now() * 1000;

  for (let i = 0; i < aLinks.length; i++) {
    let link = aLinks[i];
    let place = {
      uri: makeURI(link.url),
      title: link.title,
      
      
      
      visits: [{visitDate: now - i, transitionType: transitionLink}]
    };

    PlacesUtils.asyncHistory.updatePlaces(place, {
      handleError: function () ok(false, "couldn't add visit to history"),
      handleResult: function () {},
      handleCompletion: function () {
        if (--numLinks == 0 && aCallback)
          aCallback();
      }
    });
  }
}










function setPinnedLinks(aLinks) {
  let links = aLinks;

  if (typeof links == "string") {
    links = aLinks.split(/\s*,\s*/).map(function (id) {
      if (id)
        return {url: "http://example" + (id != "-1" ? id : "") + ".com/",
                title: "site#" + id};
    });
  }

  let string = Cc["@mozilla.org/supports-string;1"]
                 .createInstance(Ci.nsISupportsString);
  string.data = JSON.stringify(links);
  Services.prefs.setComplexValue("browser.newtabpage.pinned",
                                 Ci.nsISupportsString, string);

  NewTabUtils.pinnedLinks.resetCache();
  NewTabUtils.allPages.update();
}




function restore() {
  whenPagesUpdated();
  NewTabUtils.restore();
}




function addNewTabPageTab() {
  let tab = gWindow.gBrowser.selectedTab = gWindow.gBrowser.addTab("about:newtab");
  let browser = tab.linkedBrowser;

  function whenNewTabLoaded() {
    if (NewTabUtils.allPages.enabled) {
      
      NewTabUtils.links.populateCache(function () {
        whenSearchInitDone();
      });
    } else {
      
      
      
      executeSoon(TestRunner.next);
    }
  }

  
  if (browser.contentDocument.readyState == "complete") {
    whenNewTabLoaded();
    return;
  }

  
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    whenNewTabLoaded();
  }, true);
}











function checkGrid(aSitesPattern, aSites) {
  let length = aSitesPattern.split(",").length;
  let sites = (aSites || getGrid().sites).slice(0, length);
  let current = sites.map(function (aSite) {
    if (!aSite)
      return "";

    let pinned = aSite.isPinned();
    let hasPinnedAttr = aSite.node.hasAttribute("pinned");

    if (pinned != hasPinnedAttr)
      ok(false, "invalid state (site.isPinned() != site[pinned])");

    return aSite.url.replace(/^http:\/\/example(\d+)\.com\/$/, "$1") + (pinned ? "p" : "");
  });

  is(current, aSitesPattern, "grid status = " + aSitesPattern);
}





function blockCell(aIndex) {
  whenPagesUpdated();
  getCell(aIndex).site.block();
}






function pinCell(aIndex, aPinIndex) {
  getCell(aIndex).site.pin(aPinIndex);
}





function unpinCell(aIndex) {
  whenPagesUpdated();
  getCell(aIndex).site.unpin();
}






function simulateDrop(aSourceIndex, aDestIndex) {
  let src = getCell(aSourceIndex).site.node;
  let dest = getCell(aDestIndex).node;

  
  
  startAndCompleteDragOperation(src, dest, whenPagesUpdated);
}







function simulateExternalDrop(aDestIndex) {
  let dest = getCell(aDestIndex).node;

  
  createExternalDropIframe().then(iframe => {
    let link = iframe.contentDocument.getElementById("link");

    
    startAndCompleteDragOperation(link, dest, () => {
      
      
      whenPagesUpdated(() => {
        
        iframe.remove();
        
        TestRunner.next();
      });
    });
  });
}







function startAndCompleteDragOperation(aSource, aDest, aCallback) {
  
  synthesizeNativeMouseLDown(aSource);

  
  let offset = 0;
  let interval = setInterval(() => {
    synthesizeNativeMouseDrag(aSource, offset += 5);
  }, 10);

  
  
  aSource.addEventListener("dragstart", function onDragStart() {
    aSource.removeEventListener("dragstart", onDragStart);
    clearInterval(interval);

    
    synthesizeNativeMouseMove(aDest);
  });

  
  aDest.addEventListener("dragenter", function onDragEnter() {
    aDest.removeEventListener("dragenter", onDragEnter);

    
    synthesizeNativeMouseLUp(aDest);
    aCallback();
  });
}






function createExternalDropIframe() {
  const url = "data:text/html;charset=utf-8," +
              "<a id='link' href='http://example99.com/'>link</a>";

  let deferred = Promise.defer();
  let doc = getContentDocument();
  let iframe = doc.createElement("iframe");
  iframe.setAttribute("src", url);
  iframe.style.width = "50px";
  iframe.style.height = "50px";
  iframe.style.position = "absolute";
  iframe.style.zIndex = 50;

  let margin = doc.getElementById("newtab-margin-top");
  margin.appendChild(iframe);

  iframe.addEventListener("load", function onLoad() {
    iframe.removeEventListener("load", onLoad);
    executeSoon(() => deferred.resolve(iframe));
  });

  return deferred.promise;
}





function synthesizeNativeMouseLDown(aElement) {
  if (isLinux) {
    let win = aElement.ownerDocument.defaultView;
    EventUtils.synthesizeMouseAtCenter(aElement, {type: "mousedown"}, win);
  } else {
    let msg = isWindows ? 2 : 1;
    synthesizeNativeMouseEvent(aElement, msg);
  }
}





function synthesizeNativeMouseLUp(aElement) {
  let msg = isWindows ? 4 : (isMac ? 2 : 7);
  synthesizeNativeMouseEvent(aElement, msg);
}






function synthesizeNativeMouseDrag(aElement, aOffsetX) {
  let msg = isMac ? 6 : 1;
  synthesizeNativeMouseEvent(aElement, msg, aOffsetX);
}





function synthesizeNativeMouseMove(aElement) {
  let msg = isMac ? 5 : 1;
  synthesizeNativeMouseEvent(aElement, msg);
}







function synthesizeNativeMouseEvent(aElement, aMsg, aOffsetX = 0, aOffsetY = 0) {
  let rect = aElement.getBoundingClientRect();
  let win = aElement.ownerDocument.defaultView;
  let x = aOffsetX + win.mozInnerScreenX + rect.left + rect.width / 2;
  let y = aOffsetY + win.mozInnerScreenY + rect.top + rect.height / 2;

  let utils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsIDOMWindowUtils);

  let scale = utils.screenPixelsPerCSSPixel;
  utils.sendNativeMouseEvent(x * scale, y * scale, aMsg, 0, null);
}







function sendDragEvent(aEventType, aTarget, aData) {
  let event = createDragEvent(aEventType, aData);
  let ifaceReq = getContentWindow().QueryInterface(Ci.nsIInterfaceRequestor);
  let windowUtils = ifaceReq.getInterface(Ci.nsIDOMWindowUtils);
  windowUtils.dispatchDOMEventViaPresShell(aTarget, event, true);
}







function createDragEvent(aEventType, aData) {
  let dataTransfer = new (getContentWindow()).DataTransfer("dragstart", false);
  dataTransfer.mozSetDataAt("text/x-moz-url", aData, 0);
  let event = getContentDocument().createEvent("DragEvents");
  event.initDragEvent(aEventType, true, true, getContentWindow(), 0, 0, 0, 0, 0,
                      false, false, false, false, 0, null, dataTransfer);

  return event;
}








function whenPagesUpdated(aCallback, aOnlyIfHidden=false) {
  let page = {
    observe: _ => _,
    update: function (onlyIfHidden=false) {
      if (onlyIfHidden == aOnlyIfHidden) {
        NewTabUtils.allPages.unregister(this);
        executeSoon(aCallback || TestRunner.next);
      }
    }
  };

  NewTabUtils.allPages.register(page);
  registerCleanupFunction(function () {
    NewTabUtils.allPages.unregister(page);
  });
}




function whenSearchInitDone() {
  if (getContentWindow().gSearch._initialStateReceived) {
    executeSoon(TestRunner.next);
    return;
  }
  let eventName = "ContentSearchService";
  getContentWindow().addEventListener(eventName, function onEvent(event) {
    if (event.detail.type == "State") {
      getContentWindow().removeEventListener(eventName, onEvent);
      TestRunner.next();
    }
  });
}
