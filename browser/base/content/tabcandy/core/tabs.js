









































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




window.TabsManager = {
  
  
  
  init: function TabsManager_init() {
    var trackedTabs = [];
    new BrowserWindow();

    window.Tabs = {
      
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

    function newBrowserTab(chromeTab) {
      new BrowserTab(chromeTab);
      trackedTabs.push(chromeTab);
    }

    function unloadBrowserTab(chromeTab) {
      let index = trackedTabs.indexOf(chromeTab);
      if (index > -1) {
        trackedTabs.splice(index,1);
      } else {
        Utils.assert("unloadBrowserTab: browserTab not found in trackedTabs",false);
      }
    }

    function BrowserWindow() {
      Array.forEach(gBrowser.tabs, function(tab) newBrowserTab(tab));

      const EVENTS_TO_WATCH = ["TabOpen", "TabMove", "TabClose", "TabSelect"];

      function onEvent(event) {
        
        
        
        try {
          
          var chromeTab = event.originalTarget;

          switch (event.type) {
            case "TabSelect":
              tabsMixIns.bubble("onFocus",
                               chromeTab,
                               true);
              break;

            case "TabOpen":
              newBrowserTab(chromeTab);
              tabsMixIns.bubble("onOpen",
                                chromeTab,
                                true);
              break;

            case "TabMove":
              tabsMixIns.bubble("onMove",
                               chromeTab,
                               true);
              break;

            case "TabClose":
              tabsMixIns.bubble("onClose",
                                chromeTab,
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
          gBrowser.tabContainer.addEventListener(eventType, onEvent, true);
        });
    }

    function BrowserTab(chromeTab) {
      var browser = chromeTab.linkedBrowser;
      var mixIns = new EventListenerMixIns(this);
      mixIns.add(
        {name: "onReady",
         observe: browser,
         eventName: "DOMContentLoaded",
         useCapture: true,
         bubbleTo: tabsMixIns,
         filter: function(event) {
           
           event.tab = chromeTab;
           return event;
         }});

      mixIns.add(
        {name: "onLoad",
         observe: browser,
         eventName: "load",
         useCapture: true,
         bubbleTo: tabsMixIns,
         filter: function(event) {
           
           event.tab = chromeTab;
           return event;
         }});
    }
  }
};

})();
