Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");





function promiseInitContentBlocklistSvc(aBrowser)
{
  return ContentTask.spawn(aBrowser, {}, function* () {
    try {
      let bls = Cc["@mozilla.org/extensions/blocklist;1"]
                          .getService(Ci.nsIBlocklistService);
    } catch (ex) {
      return ex.message;
    }
    return null;
  });
}











function waitForMs(aMs) {
  let deferred = Promise.defer();
  let startTime = Date.now();
  setTimeout(done, aMs);
  function done() {
    deferred.resolve(true);
  }
  return deferred.promise;
}




function waitForEvent(subject, eventName, checkFn, useCapture, useUntrusted) {
  return new Promise((resolve, reject) => {
    subject.addEventListener(eventName, function listener(event) {
      try {
        if (checkFn && !checkFn(event)) {
          return;
        }
        subject.removeEventListener(eventName, listener, useCapture);
        resolve(event);
      } catch (ex) {
        try {
          subject.removeEventListener(eventName, listener, useCapture);
        } catch (ex2) {
          
        }
        reject(ex);
      }
    }, useCapture, useUntrusted);
  });
}
















function promiseTabLoadEvent(tab, url, eventType="load") {
  let deferred = Promise.defer();
  info("Wait tab event: " + eventType);

  function handle(event) {
    if (event.originalTarget != tab.linkedBrowser.contentDocument ||
        event.target.location.href == "about:blank" ||
        (url && event.target.location.href != url)) {
      info("Skipping spurious '" + eventType + "'' event" +
            " for " + event.target.location.href);
      return;
    }
    clearTimeout(timeout);
    tab.linkedBrowser.removeEventListener(eventType, handle, true);
    info("Tab event received: " + eventType);
    deferred.resolve(event);
  }

  let timeout = setTimeout(() => {
    tab.linkedBrowser.removeEventListener(eventType, handle, true);
    deferred.reject(new Error("Timed out while waiting for a '" + eventType + "'' event"));
  }, 30000);

  tab.linkedBrowser.addEventListener(eventType, handle, true, true);
  if (url) {
    tab.linkedBrowser.loadURI(url);
  }
  return deferred.promise;
}

function waitForCondition(condition, nextTest, errorMsg, aTries, aWait) {
  let tries = 0;
  let maxTries = aTries || 100; 
  let maxWait = aWait || 100; 
  let interval = setInterval(function() {
    if (tries >= maxTries) {
      ok(false, errorMsg);
      moveOn();
    }
    let conditionPassed;
    try {
      conditionPassed = condition();
    } catch (e) {
      ok(false, e + "\n" + e.stack);
      conditionPassed = false;
    }
    if (conditionPassed) {
      moveOn();
    }
    tries++;
  }, maxWait);
  let moveOn = function() { clearInterval(interval); nextTest(); };
}


function promiseForCondition(aConditionFn, aMessage, aTries, aWait) {
  let deferred = Promise.defer();
  waitForCondition(aConditionFn, deferred.resolve,
                   (aMessage || "Condition didn't pass."),
                   aTries, aWait);
  return deferred.promise;
}


function getTestPlugin(aName) {
  let pluginName = aName || "Test Plug-in";
  let ph = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost);
  let tags = ph.getPluginTags();

  
  for (let i = 0; i < tags.length; i++) {
    if (tags[i].name == pluginName)
      return tags[i];
  }
  ok(false, "Unable to find plugin");
  return null;
}



function setTestPluginEnabledState(newEnabledState, pluginName) {
  let name = pluginName || "Test Plug-in";
  let plugin = getTestPlugin(name);
  plugin.enabledState = newEnabledState;
}



function getTestPluginEnabledState(pluginName) {
  let name = pluginName || "Test Plug-in";
  let plugin = getTestPlugin(name);
  return plugin.enabledState;
}


function promiseForPluginInfo(aId, aBrowser) {
  let browser = aBrowser || gTestBrowser;
  return ContentTask.spawn(browser, aId, function* (aId) {
    let plugin = content.document.getElementById(aId);
    if (!(plugin instanceof Ci.nsIObjectLoadingContent))
      throw new Error("no plugin found");
    return {
      pluginFallbackType: plugin.pluginFallbackType,
      activated: plugin.activated,
      hasRunningPlugin: plugin.hasRunningPlugin,
      displayedType: plugin.displayedType,
    };
  });
}



function promisePlayObject(aId, aBrowser) {
  let browser = aBrowser || gTestBrowser;
  return ContentTask.spawn(browser, aId, function* (aId) {
    let plugin = content.document.getElementById(aId);
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    objLoadingContent.playPlugin();
  });
}

function promiseCrashObject(aId, aBrowser) {
  let browser = aBrowser || gTestBrowser;
  return ContentTask.spawn(browser, aId, function* (aId) {
    let plugin = content.document.getElementById(aId);
    let objLoadingContent = plugin.QueryInterface(Ci.nsIObjectLoadingContent);
    Components.utils.waiveXrays(plugin).crash();
  });
}


function promiseObjectValueResult(aId, aBrowser) {
  let browser = aBrowser || gTestBrowser;
  return ContentTask.spawn(browser, aId, function* (aId) {
    let plugin = content.document.getElementById(aId);
    return Components.utils.waiveXrays(plugin).getObjectValue();
  });
}


function promiseReloadPlugin(aId, aBrowser) {
  let browser = aBrowser || gTestBrowser;
  return ContentTask.spawn(browser, aId, function* (aId) {
    let plugin = content.document.getElementById(aId);
    plugin.src = plugin.src;
  });
}



function clearAllPluginPermissions() {
  let perms = Services.perms.enumerator;
  while (perms.hasMoreElements()) {
    let perm = perms.getNext();
    if (perm.type.startsWith('plugin')) {
      info("removing permission:" + perm.host + " " + perm.type + "\n");
      Services.perms.remove(perm.host, perm.type);
    }
  }
}

function updateBlocklist(aCallback) {
  let blocklistNotifier = Cc["@mozilla.org/extensions/blocklist;1"]
                          .getService(Ci.nsITimerCallback);
  let observer = function() {
    Services.obs.removeObserver(observer, "blocklist-updated");
    SimpleTest.executeSoon(aCallback);
  };
  Services.obs.addObserver(observer, "blocklist-updated", false);
  blocklistNotifier.notify(null);
}

let _originalTestBlocklistURL = null;
function setAndUpdateBlocklist(aURL, aCallback) {
  if (!_originalTestBlocklistURL) {
    _originalTestBlocklistURL = Services.prefs.getCharPref("extensions.blocklist.url");
  }
  Services.prefs.setCharPref("extensions.blocklist.url", aURL);
  updateBlocklist(aCallback);
}



function* asyncSetAndUpdateBlocklist(aURL, aBrowser) {
  info("*** loading new blocklist: " + aURL);
  let doTestRemote = aBrowser ? aBrowser.isRemoteBrowser : false;
  if (!_originalTestBlocklistURL) {
    _originalTestBlocklistURL = Services.prefs.getCharPref("extensions.blocklist.url");
  }
  Services.prefs.setCharPref("extensions.blocklist.url", aURL);
  let localPromise = TestUtils.topicObserved("blocklist-updated");
  let remotePromise;
  if (doTestRemote) {
    remotePromise = TestUtils.topicObserved("content-blocklist-updated");
  }
  let blocklistNotifier = Cc["@mozilla.org/extensions/blocklist;1"]
                            .getService(Ci.nsITimerCallback);
  blocklistNotifier.notify(null);
  info("*** waiting on local load");
  yield localPromise;
  if (doTestRemote) {
    info("*** waiting on remote load");
    yield remotePromise;
  }
  info("*** blocklist loaded.");
}


function resetBlocklist() {
  Services.prefs.setCharPref("extensions.blocklist.url", _originalTestBlocklistURL);
}



function promisePopupNotification(aName, aBrowser) {
  let deferred = Promise.defer();

  waitForCondition(() => PopupNotifications.getNotification(aName, aBrowser),
                   () => {
    ok(!!PopupNotifications.getNotification(aName, aBrowser),
       aName + " notification appeared");

    deferred.resolve();
  }, "timeout waiting for popup notification " + aName);

  return deferred.promise;
}












function promiseWaitForFocus(aWindow) {
  return new Promise((resolve) => {
    waitForFocus(resolve, aWindow);
  });
}















function waitForNotificationBar(notificationID, browser, callback) {
  return new Promise((resolve, reject) => {
    let notification;
    let notificationBox = gBrowser.getNotificationBox(browser);
    waitForCondition(
      () => (notification = notificationBox.getNotificationWithValue(notificationID)),
      () => {
        ok(notification, `Successfully got the ${notificationID} notification bar`);
        if (callback) {
          callback(notification);
        }
        resolve(notification);
      },
      `Waited too long for the ${notificationID} notification bar`
    );
  });
}

function promiseForNotificationBar(notificationID, browser) {
  let deferred = Promise.defer();
  waitForNotificationBar(notificationID, browser, deferred.resolve);
  return deferred.promise;
}








function waitForNotificationShown(notification, callback) {
  if (PopupNotifications.panel.state == "open") {
    executeSoon(callback);
    return;
  }
  PopupNotifications.panel.addEventListener("popupshown", function onShown(e) {
    PopupNotifications.panel.removeEventListener("popupshown", onShown);
    callback();
  }, false);
  notification.reshow();
}

function promiseForNotificationShown(notification) {
  let deferred = Promise.defer();
  waitForNotificationShown(notification, deferred.resolve);
  return deferred.promise;
}










function promiseUpdatePluginBindings(browser) {
  return ContentTask.spawn(browser, {}, function* () {
    let doc = content.document;
    let elems = doc.getElementsByTagName('embed');
    if (!elems || elems.length < 1) {
      elems = doc.getElementsByTagName('object');
    }
    if (elems && elems.length > 0) {
      elems[0].clientTop;
    }
  });
}
