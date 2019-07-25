



























































function scorePatternMatch(pattern, matched, offset) {
  offset = offset || 0;
  pattern = pattern.toLowerCase();
  matched = matched.toLowerCase();
 
  if (pattern.length == 0) return 0.9;
  if (pattern.length > matched.length) return 0.0;

  for (var i = pattern.length; i > 0; i--) {
    var sub_pattern = pattern.substring(0,i);
    var index = matched.indexOf(sub_pattern);

    if (index < 0) continue;
    if (index + pattern.length > matched.length + offset) continue;

    var next_string = matched.substring(index+sub_pattern.length);
    var next_pattern = null;

    if (i >= pattern.length)
      next_pattern = '';
    else
      next_pattern = pattern.substring(i);
 
    var remaining_score = 
      scorePatternMatch(next_pattern, next_string, offset + index);
 
    if (remaining_score > 0) {
      var score = matched.length-next_string.length;

      if (index != 0) {
        var j = 0;

        var c = matched.charCodeAt(index-1);
        if (c == 32 || c == 9) {
          for (var j = (index - 2); j >= 0; j--) {
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







var TabUtils = {
  
  
  
  toString: function TabUtils_toString() {
    return "[TabUtils]";
  },

  
  
  
  nameOf: function TabUtils_nameOfTab(tab) {
    
    
    
    
    
    return tab.label != undefined ? tab.label : tab.$tabTitle[0].textContent;
  },
  
  
  
  
  URLOf: function TabUtils_URLOf(tab) {
    
    if(tab.tab != undefined)
      tab = tab.tab;
    return tab.linkedBrowser.currentURI.spec;
  },

  
  
  
  faviconURLOf: function TabUtils_faviconURLOf(tab) {
    return tab.image != undefined ? tab.image : tab.$favImage[0].src;
  },
  
  
  
  
  focus: function TabUtils_focus(tab) {
    
    if (tab.tab != undefined) tab = tab.tab;
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
    var self = this;
    tabs = tabs.filter(function(tab){
      let name = TabUtils.nameOf(tab);
      let url = TabUtils.URLOf(tab);
      return name.match(self.term, "i") || url.match(self.term, "i");
    });

    tabs.sort(function sorter(x, y){
      var yScore = scorePatternMatch(self.term, TabUtils.nameOf(y));
      var xScore = scorePatternMatch(self.term, TabUtils.nameOf(x));
      return yScore - xScore; 
    });
    
    return tabs;
  },
  
  
  
  
  
  _filterForUnmatches: function TabMatcher__filterForUnmatches(tabs) {
    var self = this;
    return tabs.filter(function(tab) {
      var name = tab.$tabTitle[0].innerHTML;
      let url = TabUtils.URLOf(tab);
      return !name.match(self.term, "i") && !url.match(self.term, "i");
    });
  },
  
  
  
  
  
  
  
  
  _getTabsForOtherWindows: function TabMatcher__getTabsForOtherWindows(){
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    var enumerator = wm.getEnumerator("navigator:browser");    
    var currentWindow = wm.getMostRecentWindow("navigator:browser");
    
    var allTabs = [];    
    while (enumerator.hasMoreElements()) {
      var win = enumerator.getNext();
      
      
      if (win != currentWindow) {
        
        
        
        let tvWindow = win.TabView.getContentWindow();
        if (tvWindow)
          allTabs = allTabs.concat( tvWindow.TabItems.getItems() );
        else
          
          for (var i=0; i<win.gBrowser.tabs.length; i++) allTabs.push( win.gBrowser.tabs[i] );
      } 
    }
    return allTabs;    
  },
  
  
  
  
  
  
  
  
  
  matchedTabsFromOtherWindows: function TabMatcher_matchedTabsFromOtherWindows(){
    if (this.term.length < 2)
      return [];
    
    var tabs = this._getTabsForOtherWindows();
    tabs = this._filterAndSortForMatches(tabs);
    return tabs;
  },
  
  
  
  
  
  
  matched: function TabMatcher_matched() {
    if (this.term.length < 2)
      return [];
      
    var tabs = TabItems.getItems();
    tabs = this._filterAndSortForMatches(tabs);
    return tabs;    
  },
  
  
  
  
  unmatched: function TabMatcher_unmatched() {
    var tabs = TabItems.getItems();
    if (this.term.length < 2)
      return tabs;
      
    return this._filterForUnmatches(tabs);
  },

  
  
  
  
  
  
  
  
  
  
  doSearch: function TabMatcher_doSearch(matchFunc, unmatchFunc, otherFunc) {
    var matches = this.matched();
    var unmatched = this.unmatched();
    var otherMatches = this.matchedTabsFromOtherWindows();
    
    matches.forEach(function(tab, i) {
      matchFunc(tab, i);
    });

    otherMatches.forEach(function(tab,i) {
      otherFunc(tab, i+matches.length);      
    });
    
    unmatched.forEach(function(tab, i) {
      unmatchFunc(tab, i);
    });    
  }
};






function SearchEventHandlerClass() { 
  this.init(); 
}

SearchEventHandlerClass.prototype = {
  
  
  
  toString: function SearchEventHandlerClass_toString() {
    return "[SearchEventHandler]";
  },

  
  
  
  
  
  init: function () {
    let self = this;
    iQ("#search").hide();
    iQ("#searchshade").hide().click(function(event) {
      if ( event.target.id != "searchbox")
        hideSearch();
    });
    
    iQ("#searchbox").keyup(function() {
      performSearch();
    });
    
    iQ("#searchbutton").mousedown(function() {
      self.initiatedBy = "buttonclick";
      ensureSearchShown();
      self.switchToInMode();      
    });
    
    this.initiatedBy = "";
    this.currentHandler = null;
    this.switchToBeforeMode();
  },
  
  
  
  
  beforeSearchKeyHandler: function (event) {
    
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
    this.initiatedBy = "keydown";
    ensureSearchShown(true);
  },

  
  
  
  inSearchKeyHandler: function (event) {
    let term = iQ("#searchbox").val();
    if ((event.keyCode == event.DOM_VK_ESCAPE) || 
        (event.keyCode == event.DOM_VK_BACK_SPACE && term.length <= 1 && this.initiatedBy == "keydown")) {
      hideSearch(event);
      return;
    }

    let matcher = createSearchTabMacher();
    let matches = matcher.matched();
    let others =  matcher.matchedTabsFromOtherWindows();
    if ((event.keyCode == event.DOM_VK_RETURN || 
         event.keyCode == event.DOM_VK_ENTER) && 
         (matches.length > 0 || others.length > 0)) {
      hideSearch(event);
      if (matches.length > 0) 
        matches[0].zoomIn();
      else
        TabUtils.focus(others[0]);
    }
  },

  
  
  
  
  switchToBeforeMode: function switchToBeforeMode() {
    let self = this;
    if (this.currentHandler)
      iQ(window).unbind("keydown", this.currentHandler);
    this.currentHandler = function(event) self.beforeSearchKeyHandler(event);
    iQ(window).keydown(this.currentHandler);
  },
  
  
  
  
  
  switchToInMode: function switchToInMode() {
    let self = this;
    if (this.currentHandler)
      iQ(window).unbind("keydown", this.currentHandler);
    this.currentHandler = function(event) self.inSearchKeyHandler(event);
    iQ(window).keydown(this.currentHandler);
  }
};

var TabHandlers = {
  onMatch: function(tab, index){
    tab.addClass("onTop");
    index != 0 ? tab.addClass("notMainMatch") : tab.removeClass("notMainMatch");

    
    
    
    tab.$canvas
      .unbind("mousedown", TabHandlers._hideHandler)
      .unbind("mouseup", TabHandlers._showHandler);

    tab.$canvas
      .mousedown(TabHandlers._hideHandler)
      .mouseup(TabHandlers._showHandler);
  },
  
  onUnmatch: function(tab, index){
    tab.$container.removeClass("onTop");
    tab.removeClass("notMainMatch");

    tab.$canvas
      .unbind("mousedown", TabHandlers._hideHandler)
      .unbind("mouseup", TabHandlers._showHandler);
  },
  
  onOther: function(tab, index){
    
    
    
    
    let item = iQ("<div/>")
      .addClass("inlineMatch")
      .click(function(event){
        hideSearch(event);
        TabUtils.focus(tab);
      });
    
    iQ("<img/>")
      .attr("src", TabUtils.faviconURLOf(tab) )
      .appendTo(item);
    
    iQ("<span/>")
      .text( TabUtils.nameOf(tab) )
      .appendTo(item);
      
    index != 0 ? item.addClass("notMainMatch") : item.removeClass("notMainMatch");      
    item.appendTo("#results");
    iQ("#otherresults").show();    
  },
  
  _hideHandler: function(event){
    iQ("#search").fadeOut();
    iQ("#searchshade").fadeOut();
    TabHandlers._mouseDownLocation = {x:event.clientX, y:event.clientY};
  },
  
  _showHandler: function(event){
    
    
    
    if (TabHandlers._mouseDownLocation.x == event.clientX &&
        TabHandlers._mouseDownLocation.y == event.clientY){
      hideSearch();
      return;
    }

    iQ("#searchshade").show();    
    iQ("#search").show();
    iQ("#searchbox")[0].focus();
    
    setTimeout(performSearch, 0);
  },
  
  _mouseDownLocation: null
};

function createSearchTabMacher() {
  return new TabMatcher(iQ("#searchbox").val());
}

function hideSearch(event) {
  iQ("#searchbox").val("");
  iQ("#searchshade").hide();
  iQ("#search").hide();

  iQ("#searchbutton").css({ opacity:.8 });

#ifdef XP_MACOSX
  UI.setTitlebarColors(true);
#endif

  performSearch();
  SearchEventHandler.switchToBeforeMode();

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
}

function performSearch() {
  let matcher = new TabMatcher(iQ("#searchbox").val());

  
  
  iQ("#results").empty();
  iQ("#otherresults").hide();
  iQ("#otherresults>.label").text(tabviewString("search.otherWindowTabs"));

  matcher.doSearch(TabHandlers.onMatch, TabHandlers.onUnmatch, TabHandlers.onOther);
}






function ensureSearchShown(activatedByKeypress) {
  var $search = iQ("#search");
  var $searchShade = iQ("#searchshade");
  var $searchbox = iQ("#searchbox");
  iQ("#searchbutton").css({ opacity: 1 });

  if (!isSearchEnabled()) {
    $searchShade.show();
    $search.show();

#ifdef XP_MACOSX
    UI.setTitlebarColors({active: "#717171", inactive: "#EDEDED"});
#endif

    
    

    function dispatchTabViewSearchEnabledEvent() {
      let newEvent = document.createEvent("Events");
      newEvent.initEvent("tabviewsearchenabled", false, false);
      dispatchEvent(newEvent);
    };

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

function isSearchEnabled() {
  return iQ("#search").css("display") != "none";
}

var SearchEventHandler = new SearchEventHandlerClass();




