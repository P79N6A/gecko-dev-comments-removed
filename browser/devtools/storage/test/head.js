



"use strict";

let tempScope = {};
Cu.import("resource://gre/modules/devtools/Loader.jsm", tempScope);
Cu.import("resource://gre/modules/devtools/Console.jsm", tempScope);
const console = tempScope.console;
const devtools = tempScope.devtools;
tempScope = null;
const require = devtools.require;
const TargetFactory = devtools.TargetFactory;

const SPLIT_CONSOLE_PREF = "devtools.toolbox.splitconsoleEnabled";
const STORAGE_PREF = "devtools.storage.enabled";
const PATH = "browser/browser/devtools/storage/test/";
const MAIN_DOMAIN = "http://test1.example.org/" + PATH;
const ALT_DOMAIN = "http://sectest1.example.org/" + PATH;
const ALT_DOMAIN_SECURED = "https://sectest1.example.org:443/" + PATH;

let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});

waitForExplicitFinish();

let gToolbox, gPanelWindow, gWindow, gUI;

Services.prefs.setBoolPref(STORAGE_PREF, true);
gDevTools.testing = true;
registerCleanupFunction(() => {
  gToolbox = gPanelWindow = gWindow = gUI = null;
  Services.prefs.clearUserPref(STORAGE_PREF);
  Services.prefs.clearUserPref(SPLIT_CONSOLE_PREF);
  gDevTools.testing = false;
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }
});








function addTab(url) {
  info("Adding a new tab with URL: '" + url + "'");
  let def = promise.defer();

  
  
  
  window.focus();

  let tab = window.gBrowser.selectedTab = window.gBrowser.addTab(url);
  let linkedBrowser = tab.linkedBrowser;

  linkedBrowser.addEventListener("load", function onload(event) {
    if (event.originalTarget.location.href != url) {
      return;
    }
    linkedBrowser.removeEventListener("load", onload, true);
    info("URL '" + url + "' loading complete");
    def.resolve(tab.linkedBrowser.contentWindow);
  }, true);

  return def.promise;
}










let openTabAndSetupStorage = Task.async(function*(url) {
  



  let setupIDBInFrames = (w, i, c) => {
    if (w[i] && w[i].idbGenerator) {
      w[i].setupIDB = w[i].idbGenerator(() => setupIDBInFrames(w, i + 1, c));
      w[i].setupIDB.next();
    }
    else if (w[i] && w[i + 1]) {
      setupIDBInFrames(w, i + 1, c);
    }
    else {
      c();
    }
  };

  let content = yield addTab(url);

  let def = promise.defer();
  
  gWindow = content.wrappedJSObject;
  if (gWindow.idbGenerator) {
    gWindow.setupIDB = gWindow.idbGenerator(() => {
      setupIDBInFrames(gWindow, 0, () => {
        def.resolve();
      });
    });
    gWindow.setupIDB.next();
    yield def.promise;
  }

  
  return yield openStoragePanel();
});









let openStoragePanel = Task.async(function*(cb) {
  info("Opening the storage inspector");
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  let storage, toolbox;

  
  
  
  toolbox = gDevTools.getToolbox(target);
  if (toolbox) {
    storage = toolbox.getPanel("storage");
    if (storage) {
      gPanelWindow = storage.panelWindow;
      gUI = storage.UI;
      gToolbox = toolbox;
      info("Toolbox and storage already open");
      if (cb) {
        return cb(storage, toolbox);
      } else {
        return {
          toolbox: toolbox,
          storage: storage
        };
      }
    }
  }

  info("Opening the toolbox");
  toolbox = yield gDevTools.showToolbox(target, "storage");
  storage = toolbox.getPanel("storage");
  gPanelWindow = storage.panelWindow;
  gUI = storage.UI;
  gToolbox = toolbox;

  info("Waiting for the stores to update");
  yield gUI.once("store-objects-updated");

  yield waitForToolboxFrameFocus(toolbox);

  if (cb) {
    return cb(storage, toolbox);
  } else {
    return {
      toolbox: toolbox,
      storage: storage
    };
  }
});








function waitForToolboxFrameFocus(toolbox) {
  info("Making sure that the toolbox's frame is focused");
  let def = promise.defer();
  let win = toolbox.frame.contentWindow;
  waitForFocus(def.resolve, win);
  return def.promise;
}





function forceCollections() {
  Cu.forceGC();
  Cu.forceCC();
  Cu.forceShrinkingGC();
}




function finishTests() {
  

  



  let clearIDB = (w, i, c) => {
    if (w[i] && w[i].clear) {
      w[i].clearIterator = w[i].clear(() => clearIDB(w, i + 1, c));
      w[i].clearIterator.next();
    }
    else if (w[i] && w[i + 1]) {
      clearIDB(w, i + 1, c);
    }
    else {
      c();
    }
  };

  gWindow.clearIterator = gWindow.clear(() => {
    clearIDB(gWindow, 0, () => {
      
      forceCollections();
      finish();
    });
  });
  gWindow.clearIterator.next();
}


function click(node) {
  node.scrollIntoView()
  executeSoon(() => EventUtils.synthesizeMouseAtCenter(node, {}, gPanelWindow));
}


















function variablesViewExpandTo(aOptions) {
  let root = aOptions.rootVariable;
  let expandTo = aOptions.expandTo.split(".");
  let lastDeferred = promise.defer();

  function getNext(aProp) {
    let name = expandTo.shift();
    let newProp = aProp.get(name);

    if (expandTo.length > 0) {
      ok(newProp, "found property " + name);
      if (newProp && newProp.expand) {
        newProp.expand();
        getNext(newProp);
      }
      else {
        lastDeferred.reject(aProp);
      }
    }
    else {
      if (newProp) {
        lastDeferred.resolve(newProp);
      }
      else {
        lastDeferred.reject(aProp);
      }
    }
  }

  function fetchError(aProp) {
    lastDeferred.reject(aProp);
  }

  if (root && root.expand) {
    root.expand();
    getNext(root);
  }
  else {
    lastDeferred.resolve(root)
  }

  return lastDeferred.promise;
}




















function findVariableViewProperties(aRules, aParsed) {
  
  function init() {
    
    
    
    if (aParsed) {
      aRules = aRules.map(({name, value, dontMatch}) => {
        return {name: "." + name, value, dontMatch}
      });
    }
    
    
    let expandRules = [];
    let rules = aRules.filter((aRule) => {
      if (typeof aRule.name == "string" && aRule.name.indexOf(".") > -1) {
        expandRules.push(aRule);
        return false;
      }
      return true;
    });

    
    
    
    let outstanding = [];

    finder(rules, gUI.view, outstanding);

    
    let lastStep = processExpandRules.bind(null, expandRules);

    
    let returnResults = onAllRulesMatched.bind(null, aRules);

    return promise.all(outstanding).then(lastStep).then(returnResults);
  }

  function onMatch(aProp, aRule, aMatched) {
    if (aMatched && !aRule.matchedProp) {
      aRule.matchedProp = aProp;
    }
  }

  function finder(aRules, aView, aPromises) {
    for (let scope of aView) {
      for (let [id, prop] of scope) {
        for (let rule of aRules) {
          let matcher = matchVariablesViewProperty(prop, rule);
          aPromises.push(matcher.then(onMatch.bind(null, prop, rule)));
        }
      }
    }
  }

  function processExpandRules(aRules) {
    let rule = aRules.shift();
    if (!rule) {
      return promise.resolve(null);
    }

    let deferred = promise.defer();
    let expandOptions = {
      rootVariable: gUI.view.getScopeAtIndex(aParsed ? 1: 0),
      expandTo: rule.name
    };

    variablesViewExpandTo(expandOptions).then(function onSuccess(aProp) {
      let name = rule.name;
      let lastName = name.split(".").pop();
      rule.name = lastName;

      let matched = matchVariablesViewProperty(aProp, rule);
      return matched.then(onMatch.bind(null, aProp, rule)).then(function() {
        rule.name = name;
      });
    }, function onFailure() {
      return promise.resolve(null);
    }).then(processExpandRules.bind(null, aRules)).then(function() {
      deferred.resolve(null);
    });

    return deferred.promise;
  }

  function onAllRulesMatched(aRules) {
    for (let rule of aRules) {
      let matched = rule.matchedProp;
      if (matched && !rule.dontMatch) {
        ok(true, "rule " + rule.name + " matched for property " + matched.name);
      }
      else if (matched && rule.dontMatch) {
        ok(false, "rule " + rule.name + " should not match property " +
           matched.name);
      }
      else {
        ok(rule.dontMatch, "rule " + rule.name + " did not match any property");
      }
    }
    return aRules;
  }

  return init();
}















function matchVariablesViewProperty(aProp, aRule) {
  function resolve(aResult) {
    return promise.resolve(aResult);
  }

  if (!aProp) {
    return resolve(false);
  }

  if (aRule.name) {
    let match = aRule.name instanceof RegExp ?
                aRule.name.test(aProp.name) :
                aProp.name == aRule.name;
    if (!match) {
      return resolve(false);
    }
  }

  if ("value" in aRule) {
    let displayValue = aProp.displayValue;
    if (aProp.displayValueClassName == "token-string") {
      displayValue = displayValue.substring(1, displayValue.length - 1);
    }

    let match = aRule.value instanceof RegExp ?
                aRule.value.test(displayValue) :
                displayValue == aRule.value;
    if (!match) {
      info("rule " + aRule.name + " did not match value, expected '" +
           aRule.value + "', found '" + displayValue  + "'");
      return resolve(false);
    }
  }

  return resolve(true);
}







function selectTreeItem(ids) {
  
  
  gUI.tree.expandAll();
  click(gPanelWindow.document.querySelector("[data-id='" + JSON.stringify(ids) +
        "'] > .tree-widget-item"));
}







function selectTableItem(id) {
  click(gPanelWindow.document.querySelector(".table-widget-cell[data-id='" +
        id + "']"));
}
