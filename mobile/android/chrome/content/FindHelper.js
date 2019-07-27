


"use strict";

var FindHelper = {
  _finder: null,
  _targetTab: null,
  _initialViewport: null,
  _viewportChanged: false,
  _result: null,
  _limit: 0,

  observe: function(aMessage, aTopic, aData) {
    switch(aTopic) {
      case "FindInPage:Opened": {
        this._findOpened();
        break;
      }

      case "Tab:Selected": {
        
        this._uninit();
        break;
      }

      case "FindInPage:Closed":
        this._uninit();
        this._findClosed();
        break;
    }
  },

  _findOpened: function() {
    try {
      this._limit = Services.prefs.getIntPref("accessibility.typeaheadfind.matchesCountLimit");
    } catch (e) {
      
      this._limit = 0;
    }

    Messaging.addListener((data) => {
      this.doFind(data.searchString, data.matchCase);
      return this._getMatchesCountResult(data.searchString);
    }, "FindInPage:Find");

    Messaging.addListener((data) => {
      this.findAgain(data.searchString, false, data.matchCase);
      return this._getMatchesCountResult(data.searchString);
    }, "FindInPage:Next");

    Messaging.addListener((data) => {
      this.findAgain(data.searchString, true, data.matchCase);
      return this._getMatchesCountResult(data.searchString);
    }, "FindInPage:Prev");
  },

  _init: function() {
    
    if (this._finder) {
      return;
    }

    this._targetTab = BrowserApp.selectedTab;
    try {
      this._finder = this._targetTab.browser.finder;
    } catch (e) {
      throw new Error("FindHelper: " + e + "\n" +
        "JS stack: \n" + (e.stack || Components.stack.formattedStack));
    }

    this._finder.addResultListener(this);
    this._initialViewport = JSON.stringify(this._targetTab.getViewport());
    this._viewportChanged = false;
  },

  _uninit: function() {
    
    if (!this._finder) {
      return;
    }

    this._finder.removeSelection();
    this._finder.removeResultListener(this);
    this._finder = null;
    this._targetTab = null;
    this._initialViewport = null;
    this._viewportChanged = false;
  },

  _findClosed: function() {
    Messaging.removeListener("FindInPage:Find");
    Messaging.removeListener("FindInPage:Next");
    Messaging.removeListener("FindInPage:Prev");
  },

  


  _getMatchesCountResult: function(findString) {
    
    if (this._limit <= 0) {
      return { total: 0, current: 0, limit: 0 };
    }

    
    this._finder.requestMatchesCount(findString, this._limit);
    return this._result;
  },

  


  onMatchesCountResult: function(result) {
    this._result = result;
    this._result.limit = this._limit;
  },

  doFind: function(searchString, matchCase) {
    if (!this._finder) {
      this._init();
    }

    this._finder.caseSensitive = matchCase;
    this._finder.fastFind(searchString, false);
  },

  findAgain: function(searchString, findBackwards, matchCase) {
    
    
    
    if (!this._finder) {
      this.doFind(searchString, matchCase);
      return;
    }

    this._finder.caseSensitive = matchCase;
    this._finder.findAgain(findBackwards, false, false);
  },

  onFindResult: function(aData) {
    if (aData.result == Ci.nsITypeAheadFind.FIND_NOTFOUND) {
      if (this._viewportChanged) {
        if (this._targetTab != BrowserApp.selectedTab) {
          
          Cu.reportError("Warning: selected tab changed during find!");
          
        }
        this._targetTab.setViewport(JSON.parse(this._initialViewport));
        this._targetTab.sendViewportUpdate();
      }
    } else {
      
      const spacingFactor = 6;

      
      
      let x = aData.rect.x + (aData.rect.width * (1 - spacingFactor)) / 2;
      let y = aData.rect.y + (aData.rect.height * (1 - spacingFactor)) / 2;

      let rect = new Rect(Math.max(x, 0),
                          Math.max(y, 0),
                          
                          aData.rect.width * spacingFactor,
                          aData.rect.height * spacingFactor);

      ZoomHelper.zoomToRect(rect);
      this._viewportChanged = true;
    }
  }
};
