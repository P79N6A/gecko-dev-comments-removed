
































































function PROT_Controller(win, tabWatcher, phishingWarden) {
  this.debugZone = "controller";

  this.win_ = win;
  this.phishingWarden_ = phishingWarden;

  
  this.prefs_ = new G_Preferences();

  
  this.tabWatcher_ = tabWatcher;
  this.onTabSwitchCallback_ = BindToObject(this.onTabSwitch, this);
  this.tabWatcher_.registerListener("tabswitch",
                                    this.onTabSwitchCallback_);


  
  
  
  var commandHandlers = { 
    "safebrowsing-show-warning" :
      BindToObject(this.onUserShowWarning, this),
    "safebrowsing-accept-warning" :
      BindToObject(this.onUserAcceptWarning, this),
    "safebrowsing-decline-warning" :
      BindToObject(this.onUserDeclineWarning, this),
  };

  this.commandController_ = new PROT_CommandController(commandHandlers);
  this.win_.controllers.appendController(this.commandController_);

  
  
  this.browserView_ = new PROT_BrowserView(this.tabWatcher_, 
                                           this.win_.document);

  
  
  
  
  this.phishingWarden_.addBrowserView(this.browserView_);

  this.windowWatcher_ = 
    Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
    .getService(Components.interfaces.nsIWindowWatcher);

  G_Debug(this, "Controller initialized.");
}




PROT_Controller.prototype.shutdown = function(e) {
  G_Debug(this, "Browser window closing. Shutting controller down.");
  if (this.browserView_) {
    this.phishingWarden_.removeBrowserView(this.browserView_);
  }

  if (this.commandController_) {
    this.win_.controllers.removeController(this.commandController_);
    this.commandController_ = null;
  }


  
  
  
  this.browserView_ = null;

  if (this.tabWatcher_) {
    this.tabWatcher_.removeListener("tabswitch", 
                                    this.onTabSwitchCallback_);
    this.tabWatcher_.shutdown();
  }

  this.win_.removeEventListener("unload", this.onShutdown_, false);
  this.prefs_ = null;

  this.windowWatcher_ = null;

  G_Debug(this, "Controller shut down.");
}





PROT_Controller.prototype.onUserShowWarning = function() {
  var browser = this.tabWatcher_.getCurrentBrowser();
  this.browserView_.explicitShow(browser);
}








PROT_Controller.prototype.onUserAcceptWarning = function() {
  G_Debug(this, "User accepted warning.");
  var browser = this.tabWatcher_.getCurrentBrowser();
  G_Assert(this, !!browser, "Couldn't get current browser?!?");
  G_Assert(this, this.browserView_.hasProblem(browser),
           "User accept fired, but browser doesn't have warning showing?!?");

  this.browserView_.acceptAction(browser);
  this.browserView_.problemResolved(browser);
}








PROT_Controller.prototype.onUserDeclineWarning = function() {
  G_Debug(this, "User declined warning.");
  var browser = this.tabWatcher_.getCurrentBrowser();
  G_Assert(this, this.browserView_.hasProblem(browser),
           "User decline fired, but browser doesn't have warning showing?!?");
  this.browserView_.declineAction(browser);
  
  
  
  
}







PROT_Controller.prototype.onTabSwitch = function(e) {
  if (this.browserView_.hasProblem(e.fromBrowser)) 
    this.browserView_.problemBrowserUnselected(e.fromBrowser);

  if (this.browserView_.hasProblem(e.toBrowser))
    this.browserView_.problemBrowserSelected(e.toBrowser);
}
