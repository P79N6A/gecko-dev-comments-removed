



XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesTestUtils",
  "resource://testing-common/PlacesTestUtils.jsm");


let cachedLeftPaneFolderIdGetter;
let getter = PlacesUIUtils.__lookupGetter__("leftPaneFolderId");
if (!cachedLeftPaneFolderIdGetter && typeof(getter) == "function") {
  cachedLeftPaneFolderIdGetter = getter;
}


registerCleanupFunction(function(){
  let getter = PlacesUIUtils.__lookupGetter__("leftPaneFolderId");
  if (cachedLeftPaneFolderIdGetter && typeof(getter) != "function") {
    PlacesUIUtils.__defineGetter__("leftPaneFolderId", cachedLeftPaneFolderIdGetter);
  }
});

function openLibrary(callback, aLeftPaneRoot) {
  let library = window.openDialog("chrome://browser/content/places/places.xul",
                                  "", "chrome,toolbar=yes,dialog=no,resizable",
                                  aLeftPaneRoot);
  waitForFocus(function () {
    callback(library);
  }, library);

  return library;
}








function promiseLibrary(aLeftPaneRoot) {
  return new Promise(resolve => {
    let library = Services.wm.getMostRecentWindow("Places:Organizer");
    if (library && !library.closed) {
      if (aLeftPaneRoot) {
        library.PlacesOrganizer.selectLeftPaneContainerByHierarchy(aLeftPaneRoot);
      }
      resolve(library);
    }
    else {
      openLibrary(resolve, aLeftPaneRoot);
    }
  });
}

function promiseLibraryClosed(organizer) {
  return new Promise(resolve => {
    
    organizer.addEventListener("unload", function onUnload() {
      organizer.removeEventListener("unload", onUnload);
      resolve();
    });

    
    organizer.close();
  });
}











function promiseClipboard(aPopulateClipboardFn, aFlavor) {
  return new Promise(resolve => {
    waitForClipboard(data => !!data, aPopulateClipboardFn, resolve, aFlavor);
  });
}


















function waitForAsyncUpdates(aCallback, aScope, aArguments)
{
  let scope = aScope || this;
  let args = aArguments || [];
  let db = PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                              .DBConnection;
  let begin = db.createAsyncStatement("BEGIN EXCLUSIVE");
  begin.executeAsync();
  begin.finalize();

  let commit = db.createAsyncStatement("COMMIT");
  commit.executeAsync({
    handleResult: function() {},
    handleError: function() {},
    handleCompletion: function(aReason)
    {
      aCallback.apply(scope, args);
    }
  });
  commit.finalize();
}

function synthesizeClickOnSelectedTreeCell(aTree, aOptions) {
  let tbo = aTree.treeBoxObject;
  if (tbo.view.selection.count != 1)
     throw new Error("The test node should be successfully selected");
  
  let min = {}, max = {};
  tbo.view.selection.getRangeAt(0, min, max);
  let rowID = min.value;
  tbo.ensureRowIsVisible(rowID);
  
  var rect = tbo.getCoordsForCellItem(rowID, aTree.columns[0], "text");
  var x = rect.x + rect.width / 2;
  var y = rect.y + rect.height / 2;
  
  EventUtils.synthesizeMouse(aTree.body, x, y, aOptions || {},
                             aTree.ownerDocument.defaultView);
}









function promiseIsURIVisited(aURI) {
  let deferred = Promise.defer();

  PlacesUtils.asyncHistory.isURIVisited(aURI, function(aURI, aIsVisited) {
    deferred.resolve(aIsVisited);
  });

  return deferred.promise;
}

function promiseBookmarksNotification(notification, conditionFn) {
  info(`promiseBookmarksNotification: waiting for ${notification}`);
  return new Promise((resolve, reject) => {
    let proxifiedObserver = new Proxy({}, {
      get: (target, name) => {
        if (name == "QueryInterface")
          return XPCOMUtils.generateQI([ Ci.nsINavBookmarkObserver ]);
        info(`promiseBookmarksNotification: got ${name} notification`);
        if (name == notification)
          return () => {
            if (conditionFn.apply(this, arguments)) {
              clearTimeout(timeout);
              PlacesUtils.bookmarks.removeObserver(proxifiedObserver, false);
              executeSoon(resolve);
            } else {
              info(`promiseBookmarksNotification: skip cause condition doesn't apply to ${JSON.stringify(arguments)}`);
            }
          }
        return () => {};
      }
    });
    PlacesUtils.bookmarks.addObserver(proxifiedObserver, false);
    let timeout = setTimeout(() => {
      PlacesUtils.bookmarks.removeObserver(proxifiedObserver, false);
      reject(new Error("Timed out while waiting for bookmarks notification"));
    }, 2000);
  });
}

function promiseHistoryNotification(notification, conditionFn) {
  info(`Waiting for ${notification}`);
  return new Promise((resolve, reject) => {
    let proxifiedObserver = new Proxy({}, {
      get: (target, name) => {
        if (name == "QueryInterface")
          return XPCOMUtils.generateQI([ Ci.nsINavHistoryObserver ]);
        if (name == notification)
          return () => {
            if (conditionFn.apply(this, arguments)) {
              clearTimeout(timeout);
              PlacesUtils.history.removeObserver(proxifiedObserver, false);
              executeSoon(resolve);
            }
          }
        return () => {};
      }
    });
    PlacesUtils.history.addObserver(proxifiedObserver, false);
    let timeout = setTimeout(() => {
      PlacesUtils.history.removeObserver(proxifiedObserver, false);
      reject(new Error("Timed out while waiting for history notification"));
    }, 2000);
  });
}





















function promiseSetToolbarVisibility(aToolbar, aVisible, aCallback) {
  return new Promise((resolve, reject) => {
    function listener(event) {
      if (event.propertyName == "max-height") {
        aToolbar.removeEventListener("transitionend", listener);
        resolve();
      }
    }

    let transitionProperties =
      window.getComputedStyle(aToolbar).transitionProperty.split(", ");
    if (isToolbarVisible(aToolbar) != aVisible &&
        transitionProperties.some(
          prop => prop == "max-height" || prop == "all"
        )) {
      
      
      aToolbar.addEventListener("transitionend", listener);
      setToolbarVisibility(aToolbar, aVisible);
      return;
    }

    
    setToolbarVisibility(aToolbar, aVisible);
    resolve();
  });
}










function isToolbarVisible(aToolbar) {
  let hidingAttribute = aToolbar.getAttribute("type") == "menubar"
                        ? "autohide"
                        : "collapsed";
  let hidingValue = aToolbar.getAttribute(hidingAttribute).toLowerCase();
  
  return hidingValue !== "true" && hidingValue !== hidingAttribute;
}









let withBookmarksDialog = Task.async(function* (openFn, taskFn) {
  let dialogPromise = new Promise(resolve => {
    Services.ww.registerNotification(function winObserver(subject, topic, data) {
      if (topic != "domwindowopened")
        return;
      let win = subject.QueryInterface(Ci.nsIDOMWindow);
      win.addEventListener("load", function load() {
        win.removeEventListener("load", load);
        ok(win.location.href.startsWith("chrome://browser/content/places/bookmarkProperties"),
           "The bookmark properties dialog is ready");
        Services.ww.unregisterNotification(winObserver);
        
        waitForFocus(() => {
          resolve(win);
        }, win);
      });
    });
  });

  info("withBookmarksDialog: opening the dialog");
  
  executeSoon(openFn);

  info("withBookmarksDialog: waiting for the dialog");
  let dialogWin = yield dialogPromise;

  
  ok(dialogWin.gEditItemOverlay.initialized, "EditItemOverlay is initialized");

  info("withBookmarksDialog: executing the task");
  try {
    yield taskFn(dialogWin);
  } finally {
    info("withBookmarksDialog: canceling the dialog");
    dialogWin.document.documentElement.cancelDialog();
  }
});








let openContextMenuForContentSelector = Task.async(function* (browser, selector) {
  info("wait for the context menu");
  let contextPromise = BrowserTestUtils.waitForEvent(document.getElementById("contentAreaContextMenu"),
                                                     "popupshown");
  yield ContentTask.spawn(browser, { selector }, function* (args) {
    let doc = content.document;
    let elt = doc.querySelector(args.selector)
    dump(`openContextMenuForContentSelector: found ${elt}\n`);

    
    const domWindowUtils =
      content.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
             .getInterface(Components.interfaces.nsIDOMWindowUtils);
    let rect = elt.getBoundingClientRect();
    let left = rect.left + rect.width / 2;
    let top = rect.top + rect.height / 2;
    domWindowUtils.sendMouseEvent("contextmenu", left, top, 2,
                                  1, 0, false, 0, 0, true);
  });
  yield contextPromise;

  return gContextMenuContentData.popupNode;
});











let waitForCondition = Task.async(function* (conditionFn, errorMsg) {
  for (let tries = 0; tries < 100; ++tries) {
    if ((yield conditionFn()))
      return;
    yield new Promise(resolve => {
      if (!waitForCondition._timers) {
        waitForCondition._timers = new Set();
        registerCleanupFunction(() => {
          is(waitForCondition._timers.size, 0, "All the wait timers have been removed");
          delete waitForCondition._timers;
        });
      }
      let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      waitForCondition._timers.add(timer);
      timer.init(() => {
        waitForCondition._timers.delete(timer);
        resolve();
      }, 100, Ci.nsITimer.TYPE_ONE_SHOT);
    });
  }
  ok(false, errorMsg);
});











function fillBookmarkTextField(id, text, win) {
  let elt = win.document.getElementById(id);
  elt.focus();
  elt.select();
  for (let c of text.split("")) {
    EventUtils.synthesizeKey(c, {}, win);
  }
  elt.blur();
}
