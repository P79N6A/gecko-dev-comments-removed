



"use strict";

let EXPORTED_SYMBOLS = ["TabTitleAbridger"];

const Cu = Components.utils;
const ABRIDGMENT_PREF = "browser.tabs.cropTitleRedundancy";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gETLDService",
  "@mozilla.org/network/effective-tld-service;1",
  "nsIEffectiveTLDService");

function TabTitleAbridger(aBrowserWin) {
  this._tabbrowser = aBrowserWin.gBrowser;
}

TabTitleAbridger.prototype = {
  



  _eventNames: [
    "TabPinned",
    "TabUnpinned",
    "TabShow",
    "TabHide",
    "TabClose",
    "TabLabelModified"
  ],

  init: function TabTitleAbridger_Initialize() {
    this._cropTitleRedundancy = Services.prefs.getBoolPref(ABRIDGMENT_PREF);
    Services.prefs.addObserver(ABRIDGMENT_PREF, this, false);
    if (this._cropTitleRedundancy) {
      this._domainSets = new DomainSets();
      this._addListeners();
    }
  },

  destroy: function TabTitleAbridger_Destroy() {
    Services.prefs.removeObserver(ABRIDGMENT_PREF, this);
    if (this._cropTitleRedundancy) {
      this._dropListeners();
    }
  },

  


  observe: function TabTitleAbridger_PrefObserver(aSubject, aTopic, aData) {
    let val = Services.prefs.getBoolPref(aData);
    if (this._cropTitleRedundancy && !val) {
      this._dropListeners();
      this._domainSets.destroy();
      delete this._domainSets;
      this._resetTabTitles();
    } else if (!this._cropTitleRedundancy && val) {
      this._addListeners();
      
      this._domainSets = new DomainSets();
      let domains = this._domainSets.bootstrap(this._tabbrowser.visibleTabs);
      this._abridgeTabTitles(domains);
    }
    this._cropTitleRedundancy = val;
  },

  



  _addListeners: function TabTitleAbridger_addListeners() {
    let tabContainer = this._tabbrowser.tabContainer;
    for (let eventName of this._eventNames) {
      tabContainer.addEventListener(eventName, this, false);
    }
  },

  


  _dropListeners: function TabTitleAbridger_dropListeners() {
    let tabContainer = this._tabbrowser.tabContainer;
    for (let eventName of this._eventNames) {
      tabContainer.removeEventListener(eventName, this, false);
    }
  },

  handleEvent: function TabTitleAbridger_handler(aEvent) {
    let tab = aEvent.target;
    let updateSets;

    switch (aEvent.type) {
      case "TabUnpinned":
      case "TabShow":
        updateSets = this._domainSets.addTab(tab);
        break;
      case "TabPinned":
      case "TabHide":
      case "TabClose":
        updateSets = this._domainSets.removeTab(tab);
        tab.visibleLabel = tab.label;
        break;
      case "TabLabelModified":
        if (!tab.hidden && !tab.pinned) {
          aEvent.preventDefault();
          updateSets = this._domainSets.updateTab(tab);
        }
        break;
    }
    this._abridgeTabTitles(updateSets);
  },

  


  _resetTabTitles: function TabTitleAbridger_resetTabTitles() {
    
    for (let tab of this._tabbrowser.visibleTabs) {
      if (!tab.pinned && tab.visibleLabel != tab.label) {
        tab.visibleLabel = tab.label;
      }
    }
  },

  




  _applyAbridgment: function TabTitleAbridger_applyAbridgment(aTabSet,
                                                              aChopList) {
    for (let i = 0; i < aTabSet.length; i++) {
      let tab = aTabSet[i];
      let label = tab.label || "";
      if (label.length > 0) {
        let chop = aChopList[i] || 0;
        if (chop > 0) {
          label = label.substr(chop);
        }
      }
      if (label != tab.visibleLabel) {
        tab.visibleLabel = label;
      }
    }
  },

  



  _abridgeTabTitles: function TabTitleAbridger_abridgeTabtitles(aTabSets) {
    
    for (let tabSet of aTabSets) {
      
      let chopList = AbridgmentTools.getChopsForSet(tabSet);
      this._applyAbridgment(tabSet, chopList);
    }
  }
};





function DomainSets() {
  this._domainSets = {};
  this._tabsMappedToDomains = new WeakMap();
}

DomainSets.prototype = {
  _noHostSchemes: {
    chrome: true,
    file: true,
    resource: true,
    data: true,
    about: true
  },

  destroy: function DomainSets_destroy() {
    delete this._domainSets;
    delete this._tabsMappedToDomains;
  },

  






  bootstrap: function DomainSets_bootstrap(aVisibleTabs) {
    let needAbridgment = [];
    for (let tab of aVisibleTabs) {
      let domainSet = this.addTab(aTab)[0] || null;
      if (domainSet && needAbridgment.indexOf(domainSet) == -1) {
        needAbridgment.push(domainSet);
      }
    }
    return needAbridgment;
  },

  





  addTab: function DomainSets_addTab(aTab, aTabDomain) {
    let tabDomain = aTabDomain || this._getDomainForTab(aTab);
    if (!this._domainSets.hasOwnProperty(tabDomain)) {
      this._domainSets[tabDomain] = [];
    }
    this._domainSets[tabDomain].push(aTab);
    this._tabsMappedToDomains.set(aTab, tabDomain);
    return [this._domainSets[tabDomain]];
  },

  






  removeTab: function DomainSets_removeTab(aTab, aTabDomain) {
    let oldTabDomain = aTabDomain || this._tabsMappedToDomains.get(aTab);
    if (!this._domainSets.hasOwnProperty(oldTabDomain)) {
      return [];
    }
    let index = this._domainSets[oldTabDomain].indexOf(aTab);
    if (index == -1) {
      return [];
    }
    this._domainSets[oldTabDomain].splice(index, 1);
    this._tabsMappedToDomains.delete(aTab);
    if (!this._domainSets[oldTabDomain].length) {
      
      delete this._domainSets[oldTabDomain];
      return [];
    }
    return [this._domainSets[oldTabDomain]];
  },

  





  updateTab: function DomainSets_updateTab(aTab) {
    let tabDomain = this._getDomainForTab(aTab);
    let oldTabDomain = this._tabsMappedToDomains.get(aTab);
    if (oldTabDomain != tabDomain) {
      let needAbridgment = [];
      
      
      if (oldTabDomain) {
        needAbridgment = needAbridgment.concat(
          this.removeTab(aTab, oldTabDomain));
      }
      return needAbridgment.concat(this.addTab(aTab, tabDomain));
    }
    
    return [this._domainSets[tabDomain]];
  },

  




  _getDomainForTab: function DomainSets_getDomainForTab(aTab) {
    let browserURI = aTab.linkedBrowser.currentURI;
    if (browserURI.scheme in this._noHostSchemes) {
      return browserURI.scheme;
    }

    
    try {
      return gETLDService.getBaseDomain(browserURI);
    }
    catch (e) {}

    
    
    try {
      return browserURI.host;
    }
    catch (e) {}

    
    return browserURI.spec;
  }
};

let AbridgmentTools = {
  




  MIN_CHOP: 3,

  











  _titleIsURI: function AbridgmentTools_titleIsURI(aStr) {
    return /^\s?[^\s\/]*([^\s\/]+:\/)?\/\/?[^\s\/]*([^\s\/]+\/?)*$/.test(aStr);
  },

  




  getChopsForSet: function AbridgmentTools_getChopsForSet(aTabSet) {
    let chopList = [];
    let pathMode = false;

    aTabSet.sort(function(aTab, bTab) {
      let aLabel = aTab.label;
      let bLabel = bTab.label;
      return (aLabel < bLabel) ? -1 : (aLabel > bLabel) ? 1 : 0;
    });

    
    for (let i = 0, next = 1; next < aTabSet.length; i = next++) {
      next = this._abridgePair(aTabSet, i, next, chopList);
    }
    return chopList;
  },

  









  _abridgePair: function TabTitleAbridger_abridgePair(aTabSet, aIndex, aNext,
                                                      aChopList) {
    let tabStr = aTabSet[aIndex].label;
    let pathMode = this._titleIsURI(tabStr);
    let chop = RedundancyFinder.indexOfSep(pathMode, tabStr);

    
    if (!aChopList[aIndex]) {
      aChopList[aIndex] = 0;
    }

    
    let nextStr;
    aNext = this._nextUnproxied(aTabSet, tabStr, aNext);
    if (aNext < aTabSet.length) {
      nextStr = aTabSet[aNext].label;
    }

    
    if (chop == -1 || aNext == aTabSet.length ||
        !nextStr.startsWith(tabStr.substr(0, chop + 1))) {
      chop = aChopList[aIndex];
      if (aNext != aTabSet.length) {
        aChopList[aNext] = 0;
      }
    } else {
      [pathMode, chop] = this._getCommonChopPoint(pathMode, tabStr, nextStr,
                                                  chop);
      [chop, aChopList[aNext]] = this._adjustChops(pathMode, tabStr, nextStr,
                                                  chop);
      aChopList[aIndex] = chop;
    }

    
    for (let j = aIndex; j < aNext; j++) {
      let oldChop = aChopList[j];
      if (!oldChop || oldChop < chop) {
        aChopList[j] = chop;
      }
    }
    return aNext;
  },

  






  _nextUnproxied: function AbridgmentTools_nextUnproxied(aTabSet, aTabStr,
                                                              aStart) {
    let nextStr = aTabSet[aStart].label;
    while (aStart < aTabSet.length && aTabStr == nextStr) {
      aStart += 1;
      if (aStart < aTabSet.length) {
        nextStr = aTabSet[aStart].label;
      }
    }
    return aStart;
  },

  









  _getCommonChopPoint: function AbridgmentTools_getCommonChopPoint(aPathMode,
                                                                   aTabStr,
                                                                   aNextStr,
                                                                   aChop) {
    aChop = RedundancyFinder.findCommonPrefix(aPathMode, aTabStr, aNextStr,
                                              aChop);
    
    if (!aPathMode) {
      aPathMode = this._titleIsURI(aTabStr.substr(aChop));
      if (aPathMode) {
        aChop = RedundancyFinder.findCommonPrefix(aPathMode, aTabStr, aNextStr,
                                                  aChop);
      }
    }

    return [aPathMode, aChop + 1];
  },

  







  _adjustChops: function AbridgmentTools_adjustChops(aPathMode, aTabStr,
                                                     aNextStr, aChop) {
    let suffix = RedundancyFinder.findCommonSuffix(aPathMode, aTabStr,
                                                   aNextStr);
    let sufPos = aTabStr.length - suffix;
    let nextSufPos = aNextStr.length - suffix;
    let nextChop = aChop;

    
    if (sufPos < aChop) {
      
      aChop = RedundancyFinder.lastIndexOfSep(aPathMode, aTabStr,
                                              sufPos - 1)[1] + 1;
    } else if (nextSufPos < aChop) {
      
      nextChop = RedundancyFinder.lastIndexOfSep(aPathMode, aNextStr,
                                                 nextSufPos - 1)[1] + 1;
    }

    if (aTabStr.length - aChop < this.MIN_CHOP) {
      aChop = RedundancyFinder.lastIndexOfSep(aPathMode, aTabStr,
                                              aChop - 2)[1] + 1;
    }
    if (aNextStr.length - nextChop < this.MIN_CHOP) {
      nextChop = RedundancyFinder.lastIndexOfSep(aPathMode, aNextStr,
                                                 nextChop - 2)[1] + 1;
    }
    return [aChop, nextChop];
  }
};

let RedundancyFinder = {
  













  indexOfSep: function RedundancyFinder_indexOfSep(aPathMode, aStr, aStart) {
    if (aPathMode) {
      return aStr.indexOf('/', aStart);
    }

    let match = aStr.slice(aStart).match(/^.+?\s+[-:>\|]+\s+/);
    if (match) {
      return (aStart || 0) + match[0].length - 1;
    }

    return -1;
  },

  







  findCommonPrefix: function RedundancyFinder_findCommonPrefix(aPathMode, aStr,
                                                               aNextStr,
                                                               aChop) {
    
    do {
      aChop = this.indexOfSep(aPathMode, aStr, aChop + 1);
    } while (aChop != -1 && aNextStr.startsWith(aStr.substr(0, aChop + 1)));

    if (aChop < 0) {
      aChop = aStr.length;
    }

    
    return this.lastIndexOfSep(aPathMode, aStr, aChop - 1)[1];
  },

  





















  lastIndexOfSep: function RedundancyFinder_lastIndexOfSep(aPathMode, aStr,
                                                           aEnd) {
    if (aPathMode) {
      let path = aStr.lastIndexOf('/', aEnd);
      return [path, path];
    }

    let string = aStr.slice(0, aEnd);
    let match = string.match(/.+((\s+[-:>\|]+\s+).*?)$/);
    if (match) {
      let index = string.length - match[1].length;
      return [index, index + match[2].length - 1];
    }

    return [-1, -1];
  },

  






  findCommonSuffix: function RedundancyFinder_findCommonSuffix(aPathMode, aStr,
                                                               aNextStr) {
    let last = this.lastIndexOfSep(aPathMode, aStr)[0];

    
    if (!aNextStr.endsWith(aStr.slice(last))) {
      return 0;
    }

    
    let oldLast;
    do {
      oldLast = last;
      last = this.lastIndexOfSep(aPathMode, aStr, last - 1)[0];
    } while (last != -1 && aNextStr.endsWith(aStr.slice(last)));

    return aStr.length - oldLast;
  }
};

