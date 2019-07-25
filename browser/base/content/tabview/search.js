



























































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
  
  
  
  nameOf: function TabUtils_nameOfTab(tab) {
    
    
    
    
    
    return tab.label != undefined ? tab.label : tab.nameEl.innerHTML;
  },
  
  
  
  
  faviconURLOf: function TabUtils_faviconURLOf(tab) {
    return tab.image != undefined ? tab.image : tab.favEl.src;
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
  
  
  
  
  
  _filterAndSortForMatches: function TabMatcher__filterAndSortForMatches(tabs) {
    var self = this;
    tabs = tabs.filter(function(tab){
      var name = TabUtils.nameOf(tab);
      return name.match(self.term, "i");
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
      var name = tab.nameEl.innerHTML;
      return !name.match(self.term, "i");
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
        
        
        
        tvWindow = win.TabView.getContentWindow();
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
    if ( this.term.length < 2 )
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

    otherMatches.forEach(function(tab,i){
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
  
  
  
  
  
  init: function () {
    var self = this;
    iQ("#searchbox")[0].focus(); 
    iQ("#search").hide().click(function(event) {
      if ( event.target.id != "searchbox")
        hideSearch();
    });
    
    iQ("#searchbox").keyup(function() {
      performSearch();
    });
    
    iQ("#searchbutton").mousedown(function() {
      ensureSearchShown(null);
      self.switchToInMode();      
    });
    
    this.currentHandler = null;
    this.switchToBeforeMode();
  },
  
  
  
  
  beforeSearchKeyHandler: function (event) {
    
    var key = String.fromCharCode(event.which);
    
    if (!key.match(/[A-Z0-9]/) || event.altKey || event.ctrlKey || event.metaKey)
      return;

    
    if (event.target.nodeName == "INPUT")
      return;

    this.switchToInMode();
    ensureSearchShown(event);
  },

  
  
  
  inSearchKeyHandler: function (event) {
    var term = iQ("#searchbox").val();
    
    if (event.which == event.DOM_VK_ESCAPE) 
      hideSearch(event);
    if (event.which == event.DOM_VK_BACK_SPACE && term.length <= 1) 
      hideSearch(event);

    var matcher = new TabMatcher(term);
    var matches = matcher.matched();
    var others =  matcher.matchedTabsFromOtherWindows();
    if (event.which == event.DOM_VK_RETURN && (matches.length > 0 || others.length > 0)) {
      hideSearch(event);
      if (matches.length > 0) 
        matches[0].zoomIn();
      else
        TabUtils.focus(others[0]);
    }
  },

  
  
  
  
  switchToBeforeMode: function switchToBeforeMode() {
    var self = this;
    if (this.currentHandler)
      iQ(document).unbind("keydown", this.currentHandler);
    this.currentHandler = function(event) self.beforeSearchKeyHandler(event);
    iQ(document).keydown(self.currentHandler);
  },
  
  
  
  
  
  switchToInMode: function switchToInMode() {
    var self = this;
    if (this.currentHandler)
      iQ(document).unbind("keydown", this.currentHandler);
    this.currentHandler = function(event) self.inSearchKeyHandler(event);
    iQ(document).keydown(self.currentHandler);
  }
};

var TabHandlers = {
  onMatch: function(tab, index){
    tab.addClass("onTop");
    index != 0 ? tab.addClass("notMainMatch") : tab.removeClass("notMainMatch");

    
    
    
    iQ(tab.canvasEl)
    .unbind("mousedown", TabHandlers._hideHandler)
    .unbind("mouseup", TabHandlers._showHandler);

    iQ(tab.canvasEl)
    .mousedown(TabHandlers._hideHandler)
    .mouseup(TabHandlers._showHandler);
  },
  
  onUnmatch: function(tab, index){
    iQ(tab.container).removeClass("onTop");
    tab.removeClass("notMainMatch");

    iQ(tab.canvasEl)
     .unbind("mousedown", TabHandlers._hideHandler)
     .unbind("mouseup", TabHandlers._showHandler);
  },
  
  onOther: function(tab, index){
    
    
    
    
    var item = iQ("<div/>")
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
    TabHandlers._mouseDownLocation = {x:event.clientX, y:event.clientY};
  },
  
  _showHandler: function(event){
    
    
    
    if (TabHandlers._mouseDownLocation.x == event.clientX &&
        TabHandlers._mouseDownLocation.y == event.clientY){
        hideSearch();
        return;
    }
    
    iQ("#search").show();
    iQ("#searchbox")[0].focus();
    
    setTimeout(performSearch, 0);
  },
  
  _mouseDownLocation: null
};

function hideSearch(event){
  iQ("#searchbox").val("");
  iQ("#search").hide();
  
  iQ("#searchbutton").css({ opacity:.8 });
  
  var mainWindow = gWindow.document.getElementById("main-window");    
  mainWindow.setAttribute("activetitlebarcolor", "#C4C4C4");

  performSearch();
  SearchEventHandler.switchToBeforeMode();

  if (event){
    event.preventDefault();
    event.stopPropagation();
  }

  let newEvent = document.createEvent("Events");
  newEvent.initEvent("tabviewsearchdisabled", false, false);
  dispatchEvent(newEvent);

  
  UI.blurAll();
  gTabViewFrame.contentWindow.focus();
}

function performSearch() {
  var matcher = new TabMatcher(iQ("#searchbox").val());

  
  
  iQ("#results").empty();
  iQ("#otherresults").hide();
  iQ("#otherresults>.label").text(tabviewString("search.otherWindowTabs"));

  matcher.doSearch(TabHandlers.onMatch, TabHandlers.onUnmatch, TabHandlers.onOther);
}

function ensureSearchShown(event){
  var $search = iQ("#search");
  var $searchbox = iQ("#searchbox");
  iQ("#searchbutton").css({ opacity: 1 });
  
  
  if ($search.css("display") == "none") {
    $search.show();
    var mainWindow = gWindow.document.getElementById("main-window");
    mainWindow.setAttribute("activetitlebarcolor", "#717171");       
        
    
    
    
    
    
    setTimeout(function focusSearch() {
      $searchbox[0].focus();
      $searchbox[0].val = '0';
      $searchbox.css({"z-index":"1015"});
      if (event != null){
        var keyCode = event.which + (event.shiftKey ? 0 : 32);
        $searchbox.val(String.fromCharCode(keyCode));        
      }
    }, 0);

    let newEvent = document.createEvent("Events");
    newEvent.initEvent("tabviewsearchenabled", false, false);
    dispatchEvent(newEvent);
  }
}

var SearchEventHandler = new SearchEventHandlerClass();





