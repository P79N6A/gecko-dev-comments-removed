






































































































































































































function G_TabbedBrowserWatcher(tabBrowser, name, opt_filterAboutBlank) {
  this.debugZone = "tabbedbrowserwatcher";
  this.tabBrowser_ = tabBrowser;
  this.filterAboutBlank_ = !!opt_filterAboutBlank;
  this.events = G_TabbedBrowserWatcher.events;      
  this.eventListeners_ = {};
  for (var e in this.events)
    this.eventListeners_[this.events[e]] = [];

  
  
  
  
  G_Assert(this, typeof name == "string" && !!name,
           "Need a probabilistically unique name");
  this.name_ = name;
  this.mark_ = G_TabbedBrowserWatcher.mark_ + "-" + this.name_;

  this.tabbox_ = this.getTabBrowser().mTabBox;

  
  
  this.onTabSwitchClosure_ = BindToObject(this.onTabSwitch, this);
  this.tabbox_.addEventListener("select",
                                this.onTabSwitchClosure_, true);

  
  this.lastTab_ = this.getCurrentBrowser();
}


G_TabbedBrowserWatcher.events = {
   TABSWITCH: "tabswitch",
   };


G_TabbedBrowserWatcher.mark_ = "watcher-marked";




G_TabbedBrowserWatcher.prototype.shutdown = function() {
  G_Debug(this, "Removing event listeners");
  if (this.tabbox_) {
    this.tabbox_.removeEventListener("select",
                                     this.onTabSwitchClosure_, true);
    
    this.tabbox_ = null;
  }
  
  if (this.lastTab_) {
    this.lastTab_ = null;
  }

  if (this.tabBrowser_) {
    this.tabBrowser_ = null;
  }
}








G_TabbedBrowserWatcher.prototype.isInstrumented_ = function(browser) {
  return !!browser[this.mark_];
}






G_TabbedBrowserWatcher.prototype.instrumentBrowser_ = function(browser) {
  G_Assert(this, !this.isInstrumented_(browser),
           "Browser already instrumented!");

  
  new G_BrowserWatcher(this, browser);
  browser[this.mark_] = true;
}









G_TabbedBrowserWatcher.prototype.registerListener = function(eventType,
                                                             listener) {
  if (!(eventType in this.eventListeners_))
    throw new Error("Unknown event type: " + eventType);

  this.eventListeners_[eventType].push(listener);
}







G_TabbedBrowserWatcher.prototype.removeListener = function(eventType,
                                                           listener) {
  if (!(eventType in this.eventListeners_))
    throw new Error("Unknown event type: " + eventType);

  var ix = this.eventListeners_[eventType].indexOf(listener);
  if (ix > -1)
    this.eventListeners_[eventType].splice(ix, 1);
}







G_TabbedBrowserWatcher.prototype.fire = function(eventType, e) {
  if (!(eventType in this.eventListeners_))
    throw new Error("Unknown event type: " + eventType);

  this.eventListeners_[eventType].forEach(function(listener) { listener(e); });
}














G_TabbedBrowserWatcher.prototype.fireDocEvent_ = function(eventType,
                                                          doc,
                                                          browser) {
  
  if (!this.tabBrowser_) {
    G_Debug(this, "Firing event after shutdown: " + eventType);
    return;
  }

  try {
    
    
    
    var isTop = (doc == browser.contentDocument);
  } catch(e) {
    var isTop = undefined;
  }

  var inSelected = (browser == this.getCurrentBrowser());

  var location = doc ? doc.location.href : undefined;

  
  if (!this.filterAboutBlank_ || location != "about:blank") {

    G_Debug(this, "firing " + eventType + " for " + location +
            (isTop ? " (isTop)" : "") + (inSelected ? " (inSelected)" : ""));
    this.fire(eventType, { "doc": doc,
                           "isTop": isTop,
                           "inSelected": inSelected,
                           "browser": browser});
  }
}






G_TabbedBrowserWatcher.prototype.onTabSwitch = function(e) {
  
  
  
  if (e.target == null || 
      (e.target.localName != "tabs" && e.target.localName != "tabpanels"))
    return;

  var fromBrowser = this.lastTab_;
  var toBrowser = this.getCurrentBrowser();

  if (fromBrowser != toBrowser) {
    this.lastTab_ = toBrowser;
    G_Debug(this, "firing tabswitch");
    this.fire(this.events.TABSWITCH, { "fromBrowser": fromBrowser,
                                       "toBrowser": toBrowser });
  }
}







G_TabbedBrowserWatcher.prototype.getTabBrowser = function() {
  return this.tabBrowser_;
}




G_TabbedBrowserWatcher.prototype.getCurrentBrowser = function() {
  return this.getTabBrowser().selectedBrowser;
}




G_TabbedBrowserWatcher.prototype.getCurrentWindow = function() {
  return this.getCurrentBrowser().contentWindow;
}








G_TabbedBrowserWatcher.prototype.getBrowserFromDocument = function(doc) {
  
  
  
  

  
  function docInWindow(doc, win) {
    if (win.document == doc)
      return true;

    if (win.frames)
      for (var i = 0; i < win.frames.length; i++)
        if (docInWindow(doc, win.frames[i]))
          return true;

    return false;
  }

  var browsers = this.getTabBrowser().browsers;
  for (var i = 0; i < browsers.length; i++)
    if (docInWindow(doc, browsers[i].contentWindow))
      return browsers[i];

  return null;
}












G_TabbedBrowserWatcher.prototype.getDocumentFromURL = function(url,
                                                               opt_browser) {

  
  function docWithURL(win, url) {
    if (win.document.location.href == url)
      return win.document;

    if (win.frames)
      for (var i = 0; i < win.frames.length; i++) {
        var rv = docWithURL(win.frames[i], url);
  	if (rv)
          return rv;
      }

    return null;
  }

  if (opt_browser)
    return docWithURL(opt_browser.contentWindow, url);

  var browsers = this.getTabBrowser().browsers;
  for (var i=0; i < browsers.length; i++) {
    var rv = docWithURL(browsers[i].contentWindow, url);
    if (rv)
      return rv;
  }

  return null;
}













G_TabbedBrowserWatcher.prototype.getDocumentsFromURL = function(url,
                                                                opt_browser) {

  var docs = [];

  
  function getDocsWithURL(win, url) {
    if (win.document.location.href == url)
      docs.push(win.document);

    if (win.frames)
      for (var i = 0; i < win.frames.length; i++)
        getDocsWithURL(win.frames[i], url);
  }

  if (opt_browser)
    return getDocsWithURL(opt_browser.contentWindow, url);

  var browsers = this.getTabBrowser().browsers;
  for (var i=0; i < browsers.length; i++)
    getDocsWithURL(browsers[i].contentWindow, url);

  return docs;
}









G_TabbedBrowserWatcher.prototype.getBrowserFromWindow = function(sub) {

  
  function containsSubWindow(sub, win) {
    if (win == sub)
      return true;

    if (win.frames)
      for (var i=0; i < win.frames.length; i++)
        if (containsSubWindow(sub, win.frames[i]))
          return true;

    return false;
  }

  var browsers = this.getTabBrowser().browsers;
  for (var i=0; i < browsers.length; i++)
    if (containsSubWindow(sub, browsers[i].contentWindow))
      return browsers[i];

  return null;
}









G_TabbedBrowserWatcher.getTabElementFromBrowser = function(tabBrowser,
                                                           browser) {

  for (var i=0; i < tabBrowser.browsers.length; i++)
    if (tabBrowser.browsers[i] == browser)
      return tabBrowser.mTabContainer.childNodes[i];

  return null;
}
