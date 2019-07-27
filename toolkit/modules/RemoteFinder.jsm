




this.EXPORTED_SYMBOLS = ["RemoteFinder", "RemoteFinderListener"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function RemoteFinder(browser) {
  this._browser = browser;
  this._listeners = [];
  this._searchString = null;

  this._browser.messageManager.addMessageListener("Finder:Result", this);
  this._browser.messageManager.addMessageListener("Finder:MatchesResult", this);
  this._browser.messageManager.sendAsyncMessage("Finder:Initialize");
}

RemoteFinder.prototype = {
  addResultListener: function (aListener) {
    if (this._listeners.indexOf(aListener) === -1)
      this._listeners.push(aListener);
  },

  removeResultListener: function (aListener) {
    this._listeners = this._listeners.filter(l => l != aListener);
  },

  receiveMessage: function (aMessage) {
    
    if (aMessage.name == "Finder:Result") {
      this._searchString = aMessage.data.searchString;
    }

    
    for (let l of this._listeners) {
      if (aMessage.name == "Finder:Result") {
        l.onFindResult(aMessage.data);
      } else if (aMessage.name == "Finder:MatchesResult") {
        l.onMatchesCountResult(aMessage.data);
      }
    }
  },

  get searchString() {
    return this._searchString;
  },

  set caseSensitive(aSensitive) {
    this._browser.messageManager.sendAsyncMessage("Finder:CaseSensitive",
                                                  { caseSensitive: aSensitive });
  },

  fastFind: function (aSearchString, aLinksOnly) {
    this._browser.messageManager.sendAsyncMessage("Finder:FastFind",
                                                  { searchString: aSearchString,
                                                    linksOnly: aLinksOnly });
  },

  findAgain: function (aFindBackwards, aLinksOnly) {
    this._browser.messageManager.sendAsyncMessage("Finder:FindAgain",
                                                  { findBackwards: aFindBackwards,
                                                    linksOnly: aLinksOnly });
  },

  highlight: function (aHighlight, aWord) {
    this._browser.messageManager.sendAsyncMessage("Finder:Highlight",
                                                  { highlight: aHighlight,
                                                    word: aWord });
  },

  enableSelection: function () {
    this._browser.messageManager.sendAsyncMessage("Finder:EnableSelection");
  },

  removeSelection: function () {
    this._browser.messageManager.sendAsyncMessage("Finder:RemoveSelection");
  },

  focusContent: function () {
    
    for (let l of this._listeners) {
      try {
        if ("shouldFocusContent" in l &&
            !l.shouldFocusContent())
          return;
      } catch (ex) {
        Cu.reportError(ex);
      }
    }

    this._browser.messageManager.sendAsyncMessage("Finder:FocusContent");
  },

  keyPress: function (aEvent) {
    this._browser.messageManager.sendAsyncMessage("Finder:KeyPress",
                                                  { keyCode: aEvent.keyCode,
                                                    shiftKey: aEvent.shiftKey });
  },

  requestMatchesCount: function (aSearchString, aMatchLimit, aLinksOnly) {
    this._browser.messageManager.sendAsyncMessage("Finder:MatchesCount",
                                                  { searchString: aSearchString,
                                                    matchLimit: aMatchLimit,
                                                    linksOnly: aLinksOnly });
  }
}

function RemoteFinderListener(global) {
  let {Finder} = Cu.import("resource://gre/modules/Finder.jsm", {});
  this._finder = new Finder(global.docShell);
  this._finder.addResultListener(this);
  this._global = global;

  for (let msg of this.MESSAGES) {
    global.addMessageListener(msg, this);
  }
}

RemoteFinderListener.prototype = {
  MESSAGES: [
    "Finder:CaseSensitive",
    "Finder:FastFind",
    "Finder:FindAgain",
    "Finder:Highlight",
    "Finder:EnableSelection",
    "Finder:RemoveSelection",
    "Finder:FocusContent",
    "Finder:KeyPress",
    "Finder:MatchesCount"
  ],

  onFindResult: function (aData) {
    this._global.sendAsyncMessage("Finder:Result", aData);
  },

  
  
  onMatchesCountResult: function (aData) {
    this._global.sendAsyncMessage("Finder:MatchesResult", aData);
  },

  receiveMessage: function (aMessage) {
    let data = aMessage.data;

    switch (aMessage.name) {
      case "Finder:CaseSensitive":
        this._finder.caseSensitive = data.caseSensitive;
        break;

      case "Finder:FastFind":
        this._finder.fastFind(data.searchString, data.linksOnly);
        break;

      case "Finder:FindAgain":
        this._finder.findAgain(data.findBackwards, data.linksOnly);
        break;

      case "Finder:Highlight":
        this._finder.highlight(data.highlight, data.word);
        break;

      case "Finder:RemoveSelection":
        this._finder.removeSelection();
        break;

      case "Finder:FocusContent":
        this._finder.focusContent();
        break;

      case "Finder:KeyPress":
        this._finder.keyPress(data);
        break;

      case "Finder:MatchesCount":
        this._finder.requestMatchesCount(data.searchString, data.matchLimit, data.linksOnly);
        break;
    }
  }
};
