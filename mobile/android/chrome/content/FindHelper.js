


"use strict";

var FindHelper = {
  _finder: null,
  _targetTab: null,
  _initialViewport: null,
  _viewportChanged: false,

  observe: function(aMessage, aTopic, aData) {
    switch(aTopic) {
      case "FindInPage:Find":
        this.doFind(aData);
        break;

      case "FindInPage:Prev":
        this.findAgain(aData, true);
        break;

      case "FindInPage:Next":
        this.findAgain(aData, false);
        break;

      case "Tab:Selected":
      case "FindInPage:Closed":
        this.findClosed();
        break;
    }
  },

  doFind: function(aSearchString) {
    if (!this._finder) {
      this._targetTab = BrowserApp.selectedTab;
      this._finder = this._targetTab.browser.finder;
      this._finder.addResultListener(this);
      this._initialViewport = JSON.stringify(this._targetTab.getViewport());
      this._viewportChanged = false;
    }

    this._finder.fastFind(aSearchString, false);
  },

  findAgain: function(aString, aFindBackwards) {
    
    if (!this._finder) {
      this.doFind(aString);
      return;
    }

    this._finder.findAgain(aFindBackwards, false, false);
  },

  findClosed: function() {
    
    if (!this._finder)
      return;

    this._finder.removeSelection();
    this._finder.removeResultListener(this);
    this._finder = null;
    this._targetTab = null;
    this._initialViewport = null;
    this._viewportChanged = false;
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
      ZoomHelper.zoomToRect(aData.rect, -1, false, true);
      this._viewportChanged = true;
    }
  }
};
