


'use strict';

const xulApp = require("sdk/system/xul-app");
const { open: openWindow, getMostRecentBrowserWindow } = require('sdk/window/utils');
const { openTab, getTabContentWindow, getActiveTab, setTabURL, closeTab } = require('sdk/tabs/utils');
const promise = require("sdk/core/promise");
const windowHelpers = require('sdk/window/helpers');
const events = require("sdk/system/events");

exports.openWebpage = function openWebpage(url, enablePrivate) {
  if (xulApp.is("Fennec")) {
    let chromeWindow = getMostRecentBrowserWindow();
    let rawTab = openTab(chromeWindow, url, {
      isPrivate: enablePrivate
    });

    return {
      ready: promise.resolve(getTabContentWindow(rawTab)),
      close: function () {
        closeTab(rawTab);
        
        return promise.resolve();
      }
    };
  }
  else {
    let win = openWindow(null, {
      features: {
        private: enablePrivate
      }
    });
    let deferred = promise.defer();

    
    
    events.on("browser-delayed-startup-finished", function onReady({subject}) {
      if (subject == win) {
        events.off("browser-delayed-startup-finished", onReady);
        deferred.resolve(win);

        let rawTab = getActiveTab(win);
        setTabURL(rawTab, url);
        deferred.resolve(getTabContentWindow(rawTab));
      }
    }, true);

    return {
      ready: deferred.promise,
      close: function () {
        return windowHelpers.close(win);
      }
    };
  }
  return null;
}
