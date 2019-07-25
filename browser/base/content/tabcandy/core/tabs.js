(function(){


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;


var XULApp = {
  appWindowType: "navigator:browser",
  tabStripForWindow: function(aWindow) {
    return aWindow.document.getElementById("content").mStrip;
  },
  openTab: function(aUrl, aInBackground) {
    var window = this.mostRecentAppWindow;
    var tabbrowser = window.getBrowser();
    var tab = tabbrowser.addTab(aUrl);
    if (!aInBackground)
      tabbrowser.selectedTab = tab;
  },
  getBrowserFromContentWindow: function(aMainWindow, aWindow) {
    var browsers = aMainWindow.gBrowser.browsers;
    for (var i = 0; i < browsers.length; i++) {
      if (browsers[i].contentWindow == aWindow)
        return browsers[i];
    }
    return null;
  }
};


function Dictionary() {
  var keys = [];
  var values = [];

  this.set = function set(key, value) {
    var id = keys.indexOf(key);
    if (id == -1) {
      keys.push(key);
      values.push(value);
    } else
      values[id] = value;
  };

  this.get = function get(key, defaultValue) {
    if (defaultValue === undefined)
      defaultValue = null;
    var id = keys.indexOf(key);
    if (id == -1)
      return defaultValue;
    return values[id];
  };

  this.remove = function remove(key) {
    var id = keys.indexOf(key);
    if (id == -1)
      throw new Error("object not in dictionary: " + key);
    keys.splice(id, 1);
    values.splice(id, 1);
  };

  var readOnlyKeys = new ImmutableArray(keys);
  var readOnlyValues = new ImmutableArray(values);

  this.__defineGetter__("keys", function() { return readOnlyKeys; });
  this.__defineGetter__("values", function() { return readOnlyValues; });
  this.__defineGetter__("length", function() { return keys.length; });
}

function ImmutableArray(baseArray) {
  var self = this;
  var UNSUPPORTED_MUTATOR_METHODS = ["pop", "push", "reverse", "shift",
                                     "sort", "splice", "unshift"];
  UNSUPPORTED_MUTATOR_METHODS.forEach(
    function(methodName) {
      self[methodName] = function() {
        throw new Error("Mutator method '" + methodName + "()' is " +
                        "unsupported on this object.");
      };
    });

  self.toString = function() { return "[ImmutableArray]"; };

  self.__proto__ = baseArray;
}

function WindowWatcher() {
  var self = this;

  var observer = {
    observe: function(window, event) {
      if (event == "domwindowopened") {
        if (self.onWindowOpened)
          self.onWindowOpened(window);
      } else
        if (self.onWindowClosed)
          self.onWindowClosed(window);
    }
  };

  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"]
           .getService(Ci.nsIWindowWatcher);
  ww.registerNotification(observer);

  Extension.addUnloadMethod(
    this,
    function() {
      ww.unregisterNotification(observer);
    });
}







function BrowserWatcher(options) {
  var pendingHandlers = [];

  function makeSafeFunc(func) {
    function safeFunc(window) {
      try {
        func(window);
      } catch (e) {
        Utils.log(e);
      }
    };
    return safeFunc;
  }

  function addUnloader(chromeWindow, func) {
    function onUnload() {
      chromeWindow.removeEventListener("unload", onUnload, false);
      pendingHandlers.splice(pendingHandlers.indexOf(onUnload), 1);
      func(chromeWindow);
    }
    pendingHandlers.push(onUnload);
    chromeWindow.addEventListener("unload", onUnload, false);
  }

  function loadAndBind(chromeWindow) {
    if (options.onLoad)
      (makeSafeFunc(options.onLoad))(chromeWindow);
    if (options.onUnload)
      addUnloader(chromeWindow, makeSafeFunc(options.onUnload));
  }

  var wm = Cc["@mozilla.org/appshell/window-mediator;1"]
           .getService(Ci.nsIWindowMediator);

  var enumerator = wm.getEnumerator(XULApp.appWindowType);

  while (enumerator.hasMoreElements()) {
    var chromeWindow = enumerator.getNext();
    if (chromeWindow.gIsDoneLoading)
      loadAndBind(chromeWindow);
    else
      onWindowOpened(chromeWindow);
  }

  function onWindowOpened(chromeWindow) {
    function removeListener() {
      chromeWindow.removeEventListener("load", onLoad, false);
      pendingHandlers.splice(pendingHandlers.indexOf(removeListener), 1);
    }
    function onLoad() {
      removeListener();
      var type = chromeWindow.document.documentElement
                 .getAttribute("windowtype");
      if (type == XULApp.appWindowType)
        loadAndBind(chromeWindow);
    }
    chromeWindow.addEventListener("load", onLoad, false);
    pendingHandlers.push(removeListener);
  }

  var ww = new WindowWatcher();
  ww.onWindowOpened = onWindowOpened;

  Extension.addUnloadMethod(
    this,
    function() {
      ww.unload();
      var handlers = pendingHandlers.slice();
      handlers.forEach(function(handler) { handler(); });
    });
}









var Extension = {
  
  
  
  
  
  
  
  
  

  addUnloadMethod: function addUnloadMethod(obj, unloader) {
    function unloadWrapper() {
      window.removeEventListener("unload", unloadWrapper, true);
      unloader.apply(obj, arguments);
    }

    window.addEventListener("unload", unloadWrapper, true);

    obj.unload = unloadWrapper;
  }
};









function EventListenerMixIns(mixInto) {
  var mixIns = {};

  this.add = function add(options) {
    if (mixIns) {
      if (options.name in mixIns)
        Utils.log("mixIn for", options.name, "already exists.");
      options.mixInto = mixInto;
      mixIns[options.name] = new EventListenerMixIn(options);
    }
  };

  this.bubble = function bubble(name, target, event) {
    if (mixIns)
      mixIns[name].trigger(target, event);
  };

  Extension.addUnloadMethod(
    this,
    function() {
      for (name in mixIns) {
        mixIns[name].unload();
        delete mixIns[name];
      }
      mixIns = null;
    });
}

function EventListenerMixIn(options) {
  var listeners = [];

  function onEvent(event, target) {


    if (listeners) {
      if (options.filter)
        event = options.filter.call(this, event);
      if (event) {
        if (!target)
          target = options.mixInto;
        var listenersCopy = listeners.slice();

        for (var i = 0; i < listenersCopy.length; i++)
          try {

            listenersCopy[i].call(target, event);
          } catch (e) {
            Utils.log(e);
          }
        if (options.bubbleTo)
          options.bubbleTo.bubble(options.name, target, event);
      }
    }
  };

  options.mixInto[options.name] = function bind(cb) {

    if (typeof(cb) != "function")
      Utils.log("Callback must be a function.");
    if (listeners)
      listeners.push(cb);
  };

  options.mixInto[options.name].unbind = function unbind(cb) {
    if (listeners) {
      var index = listeners.indexOf(cb);
      if (index != -1)
        listeners.splice(index, 1);
    }
  };

  this.trigger = function trigger(target, event) {
    onEvent(event, target);
  };

  if (options.observe)
    options.observe.addEventListener(options.eventName,
                                     onEvent,
                                     options.useCapture);

  Extension.addUnloadMethod(
    this,
    function() {
      listeners = null;
      if (options.observe)
        options.observe.removeEventListener(options.eventName,
                                            onEvent,
                                            options.useCapture);
    });
}

function Tabs() {
  var trackedWindows = new Dictionary();
  var trackedTabs = new Dictionary();

  var windows = {
    get focused() {
      var wm = Cc["@mozilla.org/appshell/window-mediator;1"]
               .getService(Ci.nsIWindowMediator);
      var chromeWindow = wm.getMostRecentWindow("navigator:browser");
      Utils.log( trackedWindows )
      if (chromeWindow)
        return trackedWindows.get(chromeWindow);
      return null;
    }
  };

  
  
  var wm = Cc["@mozilla.org/appshell/window-mediator;1"]
           .getService(Ci.nsIWindowMediator);
  var chromeWindow = wm.getMostRecentWindow("navigator:browser");
  trackedWindows.set(chromeWindow,
                      new BrowserWindow(chromeWindow));

  windows.__proto__ = trackedWindows.values;

  var tabs = {
    get focused() {
      var browserWindow = windows.focused;
      if (browserWindow)
        return browserWindow.getFocusedTab();
      return null;
    },
    open: function open(url) {
      var browserWindow = windows.focused;
      
      
      return browserWindow.addTab(url);
    },
    tab: function tab(value) {
      
      var result = $(value).data('tab');
      if(!result)
        result = $(value).find("canvas").data("link").tab;
      
      return result;
    },
    toString: function toString() {
      return "[Tabs]";
    }
  };

  var tabsMixIns = new EventListenerMixIns(tabs);
  tabsMixIns.add({name: "onReady"});
  tabsMixIns.add({name: "onFocus"});
  tabsMixIns.add({name: "onClose"});
  tabsMixIns.add({name: "onOpen"});
  tabsMixIns.add({name: "onLoad"}); 

  tabs.__proto__ = trackedTabs.values;


  function newBrowserTab(tabbrowser, chromeTab) {
    var browserTab = new BrowserTab(tabbrowser, chromeTab);
    trackedTabs.set(chromeTab, browserTab);
    return browserTab;
  }

  function unloadBrowserTab(chromeTab) {
    var browserTab = trackedTabs.get(chromeTab);
    trackedTabs.remove(chromeTab);
    browserTab._unload();
  }

  function BrowserWindow(chromeWindow) {
    var tabbrowser = chromeWindow.getBrowser();

    for (var i = 0; i < tabbrowser.tabContainer.itemCount; i++)
      newBrowserTab(tabbrowser,
                    tabbrowser.tabContainer.getItemAtIndex(i));

    const EVENTS_TO_WATCH = ["TabOpen", "TabMove", "TabClose", "TabSelect"];

    function onEvent(event) {
      
      
      
      try {
        
        var chromeTab = event.originalTarget;

        switch (event.type) {
        case "TabSelect":
          tabsMixIns.bubble("onFocus",
                           trackedTabs.get(chromeTab),
                           true);
          break;
        case "TabOpen":
          newBrowserTab(tabbrowser, chromeTab);
          tabsMixIns.bubble("onOpen",
                            trackedTabs.get(chromeTab),
                            true);
          break;
        case "TabMove":
          break;
        case "TabClose":
          tabsMixIns.bubble("onClose",
                            trackedTabs.get(chromeTab),
                            true);
          unloadBrowserTab(chromeTab);
          break;
        }
      } catch (e) {
        Utils.log(e);
      }
    }

    EVENTS_TO_WATCH.forEach(
      function(eventType) {
        tabbrowser.addEventListener(eventType, onEvent, true);
      });

    this.addTab = function addTab(url) {
      var chromeTab = tabbrowser.addTab(url);
      
      
      return trackedTabs.get(chromeTab);
    };

    this.getFocusedTab = function getFocusedTab() {
      return trackedTabs.get(tabbrowser.selectedTab);
    };

    Extension.addUnloadMethod(
      this,
      function() {
        EVENTS_TO_WATCH.forEach(
          function(eventType) {
            tabbrowser.removeEventListener(eventType, onEvent, true);
          });
        for (var i = 0; i < tabbrowser.tabContainer.itemCount; i++)
          unloadBrowserTab(tabbrowser.tabContainer.getItemAtIndex(i));
      });
  }

  function BrowserTab(tabbrowser, chromeTab) {
    var browser = chromeTab.linkedBrowser;

    var mixIns = new EventListenerMixIns(this);
    var self = this;

    mixIns.add(
      {name: "onReady",
       observe: browser,
       eventName: "DOMContentLoaded",
       useCapture: true,
       bubbleTo: tabsMixIns,
       filter: function(event) {
         
         event.tab = self;
         return event;
       }});

    mixIns.add(
      {name: "onFocus",
       observe: chromeTab,
       eventName: "TabSelect",
       useCapture: true,
       bubbleTo: tabsMixIns,
       filter: function(event) {
         
         
         
         return true;
       }});

    this.__proto__ = {
      get isClosed() { return (browser == null); },

      get url() {
        if (browser && browser.currentURI)
          return browser.currentURI.spec;
        return null;
      },

      get favicon() {
        if (chromeTab && chromeTab.image) {
          return chromeTab.image;
        }
        return null;
      },

      get contentWindow() {
        if (browser && browser.contentWindow)
          return browser.contentWindow;
        return null;
      },

      get contentDocument() {
        if (browser && browser.contentDocument)
          return browser.contentDocument;
        return null;
      },

      get raw() { return chromeTab; },

      focus: function focus() {
        if (browser)
          tabbrowser.selectedTab = chromeTab;
      },

      close: function close() {
        if (browser)
          browser.contentWindow.close();
      },

      toString: function toString() {
        if (!browser)
          return "[Closed Browser Tab]";
        else
          return "[Browser Tab]";
      },

      _unload: function _unload() {
        mixIns.unload();
        mixIns = null;
        tabbrowser = null;
        chromeTab = null;
        browser = null;
      }
    };
  }

  var browserWatcher = new BrowserWatcher(
    {onLoad: function(chromeWindow) {
       var trackedWindow = trackedWindows.get(chromeWindow);
       if (!trackedWindow)
         trackedWindows.set(chromeWindow,
                            new BrowserWindow(chromeWindow));
     },
     onUnload: function(chromeWindow) {
       var browserWindow = trackedWindows.get(chromeWindow);
       trackedWindows.remove(chromeWindow);
       browserWindow.unload();
     }
    });

  this.__defineGetter__("tabs", function() { return tabs; });

  Extension.addUnloadMethod(
    this,
    function() {
      tabsMixIns.unload();
      browserWatcher.unload();
    });
}

window.Tabs = new Tabs().tabs;
})();