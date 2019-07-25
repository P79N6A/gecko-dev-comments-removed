









































(function(){

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;



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

}




window.TabsManager = iQ.extend(new Subscribable(), {
  
  
  
  init: function TabsManager_init() {
    var self = this;
    if (!gWindow || !gWindow.getBrowser || !gWindow.getBrowser()) {
      iQ.timeout(function TabsManager_init_delayedInit() {
        self.init();
      }, 100);
      return;
    }

    var trackedTabs = [];

    gWindow.tabcandyBrowserWindow = new BrowserWindow(gWindow);

    var windows = {
      get focused() {
        var wm = Cc["@mozilla.org/appshell/window-mediator;1"]
                 .getService(Ci.nsIWindowMediator);
        var chromeWindow = wm.getMostRecentWindow("navigator:browser");
        if (chromeWindow)
          return chromeWindow.tabcandyBrowserWindow;
        return null;
      }
    };

    window.Tabs = {
      
      get focused() {
        var browserWindow = windows.focused;
        if (browserWindow)
          return browserWindow.getFocusedTab();
        return null;
      },

      
      toString: function toString() {
        return "[Tabs]";
      }
    };

    window.Tabs.__proto__ = trackedTabs;

    var tabsMixIns = new EventListenerMixIns(window.Tabs);
    tabsMixIns.add({name: "onReady"});
    tabsMixIns.add({name: "onLoad"});
    tabsMixIns.add({name: "onFocus"});
    tabsMixIns.add({name: "onClose"});
    tabsMixIns.add({name: "onOpen"});
    tabsMixIns.add({name: "onMove"});

  

    function newBrowserTab(tabbrowser, chromeTab) {
      var browserTab = new BrowserTab(tabbrowser, chromeTab);
      chromeTab.tabcandyBrowserTab = browserTab;
      trackedTabs.push(browserTab);
      return browserTab;
    }

    function unloadBrowserTab(chromeTab) {
      var browserTab = chromeTab.tabcandyBrowserTab;
      var index = trackedTabs.indexOf(browserTab);
      if (index > -1) {
        trackedTabs.splice(index,1);
      } else {
        Utils.assert("unloadBrowserTab: browserTab not found in trackedTabs",false);
      }
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
                               chromeTab.tabcandyBrowserTab,
                               true);
              break;

            case "TabOpen":
              newBrowserTab(tabbrowser, chromeTab);
              tabsMixIns.bubble("onOpen",
                                chromeTab.tabcandyBrowserTab,
                                true);
              break;

            case "TabMove":
              tabsMixIns.bubble("onMove",
                               chromeTab.tabcandyBrowserTab,
                               true);
              break;

            case "TabClose":
              tabsMixIns.bubble("onClose",
                                chromeTab.tabcandyBrowserTab,
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
          tabbrowser.tabContainer.addEventListener(eventType, onEvent, true);
        });

      this.getFocusedTab = function getFocusedTab() {
        return tabbrowser.selectedTab.tabcandyBrowserTab;
      };
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
        {name: "onLoad",
         observe: browser,
         eventName: "load",
         useCapture: true,
         bubbleTo: tabsMixIns,
         filter: function(event) {
           
           event.tab = self;
           return event;
         }});

      













      this.__proto__ = {

        get closed() !browser,
        get url() browser && browser.currentURI ? browser.currentURI.spec : null,
        get favicon() chromeTab && chromeTab.image ? chromeTab.image : null,

        get contentWindow() browser && browser.contentWindow ? browser.contentWindow : null,
        get contentDocument() browser && browser.contentDocument ? browser.contentDocument : null,

        get raw() chromeTab,
        get tabbrowser() tabbrowser,

        isFocused: function() browser && tabbrowser.selectedTab == chromeTab,

        focus: function focus() {
          if (browser)
            tabbrowser.selectedTab = chromeTab;
        },

        close: function close() {
          if (browser)
            tabbrowser.removeTab(chromeTab);
        },

        toString: function toString() !browser ? "[Closed Browser Tab]" : "[Browser Tab]",

        _unload: function _unload() {
          mixIns = null;
          tabbrowser = null;
          chromeTab = null;
          browser = null;
        }
      };
    }

    this._sendToSubscribers('load');
  }
});


window.TabsManager.init();

})();
