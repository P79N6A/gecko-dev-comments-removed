



























































let TabUtils = {
  
  
  
  toString: function TabUtils_toString() {
    return "[TabUtils]";
  },

  
  
  
  nameOf: function TabUtils_nameOf(tab) {
    
    
    
    
    
    return tab.label != undefined ? tab.label : tab.$tabTitle[0].textContent;
  },

  
  
  
  URLOf: function TabUtils_URLOf(tab) {
    
    if ("tab" in tab)
      tab = tab.tab;
    return tab.linkedBrowser.currentURI.spec;
  },

  
  
  
  faviconURLOf: function TabUtils_faviconURLOf(tab) {
    return tab.image != undefined ? tab.image : tab.$favImage[0].src;
  },

  
  
  
  focus: function TabUtils_focus(tab) {
    
    if ("tab" in tab)
      tab = tab.tab;
    tab.ownerDocument.defaultView.gBrowser.selectedTab = tab;
    tab.ownerDocument.defaultView.focus();
  }
};






function TabMatcher(term) {
  this.term = term;
}

TabMatcher.prototype = {
  
  
  
  toString: function TabMatcher_toString() {
    return "[TabMatcher (" + this.term + ")]";
  },

  
  
  
  
  
  _filterAndSortForMatches: function TabMatcher__filterAndSortForMatches(tabs) {
    let self = this;
    tabs = tabs.filter(function TabMatcher__filterAndSortForMatches_filter(tab) {
      let name = TabUtils.nameOf(tab);
      let url = TabUtils.URLOf(tab);
      return name.match(self.term, "i") || url.match(self.term, "i");
    });

    tabs.sort(function TabMatcher__filterAndSortForMatches_sort(x, y) {
      let yScore = self._scorePatternMatch(self.term, TabUtils.nameOf(y));
      let xScore = self._scorePatternMatch(self.term, TabUtils.nameOf(x));
      return yScore - xScore;
    });

    return tabs;
  },

  
  
  
  
  _filterForUnmatches: function TabMatcher__filterForUnmatches(tabs) {
    let self = this;
    return tabs.filter(function TabMatcher__filterForUnmatches_filter(tab) {
      let name = tab.$tabTitle[0].textContent;
      let url = TabUtils.URLOf(tab);
      return !name.match(self.term, "i") && !url.match(self.term, "i");
    });
  },

  
  
  
  
  
  
  
  _getTabsForOtherWindows: function TabMatcher__getTabsForOtherWindows() {
    let enumerator = Services.wm.getEnumerator("navigator:browser");
    let allTabs = [];

    while (enumerator.hasMoreElements()) {
      let win = enumerator.getNext();
      
      if (win != gWindow)
        allTabs.push.apply(allTabs, win.gBrowser.tabs);
    }
    return allTabs;
  },

  
  
  
  
  
  
  
  
  matchedTabsFromOtherWindows: function TabMatcher_matchedTabsFromOtherWindows() {
    if (this.term.length < 2)
      return [];

    let tabs = this._getTabsForOtherWindows();
    return this._filterAndSortForMatches(tabs);
  },

  
  
  
  
  matched: function TabMatcher_matched() {
    if (this.term.length < 2)
      return [];

    let tabs = TabItems.getItems();
    return this._filterAndSortForMatches(tabs);
  },

  
  
  
  unmatched: function TabMatcher_unmatched() {
    let tabs = TabItems.getItems();
    if (this.term.length < 2)
      return tabs;

    return this._filterForUnmatches(tabs);
  },

  
  
  
  
  
  
  
  
  
  
  doSearch: function TabMatcher_doSearch(matchFunc, unmatchFunc, otherFunc) {
    let matches = this.matched();
    let unmatched = this.unmatched();
    let otherMatches = this.matchedTabsFromOtherWindows();
    
    matches.forEach(function(tab, i) {
      matchFunc(tab, i);
    });

    otherMatches.forEach(function(tab,i) {
      otherFunc(tab, i+matches.length);
    });

    unmatched.forEach(function(tab, i) {
      unmatchFunc(tab, i);
    });
  },

  
  
  
  
  
  _scorePatternMatch: function TabMatcher__scorePatternMatch(pattern, matched, offset) {
    offset = offset || 0;
    pattern = pattern.toLowerCase();
    matched = matched.toLowerCase();

    if (pattern.length == 0)
      return 0.9;
    if (pattern.length > matched.length)
      return 0.0;

    for (let i = pattern.length; i > 0; i--) {
      let sub_pattern = pattern.substring(0,i);
      let index = matched.indexOf(sub_pattern);

      if (index < 0)
        continue;
      if (index + pattern.length > matched.length + offset)
        continue;

      let next_string = matched.substring(index+sub_pattern.length);
      let next_pattern = null;

      if (i >= pattern.length)
        next_pattern = '';
      else
        next_pattern = pattern.substring(i);

      let remaining_score = this._scorePatternMatch(next_pattern, next_string, offset + index);

      if (remaining_score > 0) {
        let score = matched.length-next_string.length;

        if (index != 0) {
          let c = matched.charCodeAt(index-1);
          if (c == 32 || c == 9) {
            for (let j = (index - 2); j >= 0; j--) {
              c = matched.charCodeAt(j);
              score -= ((c == 32 || c == 9) ? 1 : 0.15);
            }
          } else {
            score -= index;
          }
        }

        score += remaining_score * next_string.length;
        score /= matched.length;
        return score;
      }
    }
    return 0.0;
  }
};





let TabHandlers = {
  _mouseDownLocation: null,

  
  
  
  onMatch: function TabHandlers_onMatch(tab, index) {
    tab.addClass("onTop");
    index != 0 ? tab.addClass("notMainMatch") : tab.removeClass("notMainMatch");

    
    
    
    tab.$canvas
      .unbind("mousedown", TabHandlers._hideHandler)
      .unbind("mouseup", TabHandlers._showHandler);

    tab.$canvas
      .mousedown(TabHandlers._hideHandler)
      .mouseup(TabHandlers._showHandler);
  },

  
  
  
  onUnmatch: function TabHandlers_onUnmatch(tab, index) {
    tab.$container.removeClass("onTop");
    tab.removeClass("notMainMatch");

    tab.$canvas
      .unbind("mousedown", TabHandlers._hideHandler)
      .unbind("mouseup", TabHandlers._showHandler);
  },

  
  
  
  onOther: function TabHandlers_onOther(tab, index) {
    
    
    
    
    let item = iQ("<div/>")
      .addClass("inlineMatch")
      .click(function TabHandlers_onOther_click(event) {
        Search.hide(event);
        TabUtils.focus(tab);
      });

    iQ("<img/>")
      .attr("src", TabUtils.faviconURLOf(tab))
      .appendTo(item);

    iQ("<span/>")
      .text(TabUtils.nameOf(tab))
      .appendTo(item);

    index != 0 ? item.addClass("notMainMatch") : item.removeClass("notMainMatch");
    item.appendTo("#results");
    iQ("#otherresults").show();
  },

  
  
  
  _hideHandler: function TabHandlers_hideHandler(event) {
    iQ("#search").fadeOut();
    iQ("#searchshade").fadeOut();
    TabHandlers._mouseDownLocation = {x:event.clientX, y:event.clientY};
  },

  
  
  
  _showHandler: function TabHandlers_showHandler(event) {
    
    
    
    if (TabHandlers._mouseDownLocation.x == event.clientX &&
        TabHandlers._mouseDownLocation.y == event.clientY) {
      Search.hide();
      return;
    }

    iQ("#searchshade").show();
    iQ("#search").show();
    iQ("#searchbox")[0].focus();
    
    setTimeout(Search.perform, 0);
  }
};





let Search = {
  _initiatedBy: "",
  _blockClick: false,
  _currentHandler: null,

  
  
  
  toString: function Search_toString() {
    return "[Search]";
  },

  
  
  
  
  init: function Search_init() {
    let self = this;

    iQ("#search").hide();
    iQ("#searchshade").hide().mousedown(function Search_init_shade_mousedown(event) {
      if (event.target.id != "searchbox" && !self._blockClick)
        self.hide();
    });

    iQ("#searchbox").keyup(function Search_init_box_keyup() {
      self.perform();
    });

    iQ("#searchbutton").mousedown(function Search_init_button_mousedown() {
      self._initiatedBy = "buttonclick";
      self.ensureShown();
      self.switchToInMode();
    });

    window.addEventListener("focus", function Search_init_window_focus() {
      if (self.isEnabled()) {
        self._blockClick = true;
        setTimeout(function() {
          self._blockClick = false;
        }, 0);
      }
    }, false);

    this.switchToBeforeMode();
  },

  
  
  
  _beforeSearchKeyHandler: function Search__beforeSearchKeyHandler(event) {
    
    if (event.altKey || event.ctrlKey || event.metaKey)
      return;

    if ((event.keyCode > 0 && event.keyCode <= event.DOM_VK_DELETE) ||
        event.keyCode == event.DOM_VK_CONTEXT_MENU ||
        event.keyCode == event.DOM_VK_SLEEP ||
        (event.keyCode >= event.DOM_VK_F1 &&
         event.keyCode <= event.DOM_VK_SCROLL_LOCK) ||
        event.keyCode == event.DOM_VK_META ||
        event.keyCode == 91 || 
        event.keyCode == 92 || 
        (!event.keyCode && !event.charCode)) {
      return;
    }

    
    if (event.target.nodeName == "INPUT")
      return;

    
    
    if (event.keyCode == KeyEvent.DOM_VK_SLASH) {
      event.stopPropagation();
      event.preventDefault();
    }

    this.switchToInMode();
    this._initiatedBy = "keydown";
    this.ensureShown(true);
  },

  
  
  
  _inSearchKeyHandler: function Search__inSearchKeyHandler(event) {
    let term = iQ("#searchbox").val();
    if ((event.keyCode == event.DOM_VK_ESCAPE) ||
        (event.keyCode == event.DOM_VK_BACK_SPACE && term.length <= 1 &&
         this._initiatedBy == "keydown")) {
      this.hide(event);
      return;
    }

    let matcher = this.createSearchTabMatcher();
    let matches = matcher.matched();
    let others =  matcher.matchedTabsFromOtherWindows();
    if ((event.keyCode == event.DOM_VK_RETURN ||
         event.keyCode == event.DOM_VK_ENTER) &&
         (matches.length > 0 || others.length > 0)) {
      this.hide(event);
      if (matches.length > 0) 
        matches[0].zoomIn();
      else
        TabUtils.focus(others[0]);
    }
  },

  
  
  
  switchToBeforeMode: function Search_switchToBeforeMode() {
    let self = this;
    if (this._currentHandler)
      iQ(window).unbind("keydown", this._currentHandler);
    this._currentHandler = function Search_switchToBeforeMode_handler(event) {
      self._beforeSearchKeyHandler(event);
    }
    iQ(window).keydown(this._currentHandler);
  },

  
  
  
  switchToInMode: function Search_switchToInMode() {
    let self = this;
    if (this._currentHandler)
      iQ(window).unbind("keydown", this._currentHandler);
    this._currentHandler = function Search_switchToInMode_handler(event) {
      self._inSearchKeyHandler(event);
    }
    iQ(window).keydown(this._currentHandler);
  },

  createSearchTabMatcher: function Search_createSearchTabMatcher() {
    return new TabMatcher(iQ("#searchbox").val());
  },

  
  
  
  isEnabled: function Search_isEnabled() {
    return iQ("#search").css("display") != "none";
  },

  
  
  
  hide: function Search_hide(event) {
    if (!this.isEnabled())
      return;

    iQ("#searchbox").val("");
    iQ("#searchshade").hide();
    iQ("#search").hide();

    iQ("#searchbutton").css({ opacity:.8 });

#ifdef XP_MACOSX
    UI.setTitlebarColors(true);
#endif

    this.perform();
    this.switchToBeforeMode();

    if (event) {
      
      
      
      if (event.type == "keydown")
        UI.ignoreKeypressForSearch = true;
      event.preventDefault();
      event.stopPropagation();
    }

    
    UI.blurAll();
    gTabViewFrame.contentWindow.focus();

    let newEvent = document.createEvent("Events");
    newEvent.initEvent("tabviewsearchdisabled", false, false);
    dispatchEvent(newEvent);
  },

  
  
  
  perform: function Search_perform() {
    let matcher =  this.createSearchTabMatcher();

    
    
    iQ("#results").empty();
    iQ("#otherresults").hide();
    iQ("#otherresults>.label").text(tabviewString("search.otherWindowTabs"));

    matcher.doSearch(TabHandlers.onMatch, TabHandlers.onUnmatch, TabHandlers.onOther);
  },

  
  
  
  
  
  ensureShown: function Search_ensureShown(activatedByKeypress) {
    let $search = iQ("#search");
    let $searchShade = iQ("#searchshade");
    let $searchbox = iQ("#searchbox");
    iQ("#searchbutton").css({ opacity: 1 });

    
    
    function dispatchTabViewSearchEnabledEvent() {
      let newEvent = document.createEvent("Events");
      newEvent.initEvent("tabviewsearchenabled", false, false);
      dispatchEvent(newEvent);
    };

    if (!this.isEnabled()) {
      $searchShade.show();
      $search.show();

#ifdef XP_MACOSX
      UI.setTitlebarColors({active: "#717171", inactive: "#EDEDED"});
#endif

      if (activatedByKeypress) {
        
        $searchbox[0].focus();
        dispatchTabViewSearchEnabledEvent();
      } else {
        
        
        setTimeout(function setFocusAndDispatchSearchEnabledEvent() {
          $searchbox[0].focus();
          dispatchTabViewSearchEnabledEvent();
        }, 0);
      }
    }
  }
};

