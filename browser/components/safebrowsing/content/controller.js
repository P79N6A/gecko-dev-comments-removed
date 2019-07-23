
































































function PROT_Controller(win, tabBrowser, phishingWarden) {
  this.debugZone = "controller";

  this.win_ = win;
  this.phishingWarden_ = phishingWarden;

  
  this.prefs_ = new G_Preferences();

  
  this.tabBrowser_ = tabBrowser;
  this.onTabSwitchClosure_ = BindToObject(this.onTabSwitch, this);
  this.tabBrowser_.mTabBox.addEventListener("select", this.onTabSwitchClosure_, true);

  
  this.lastTab_ = tabBrowser.selectedBrowser;

  
  
  
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

  
  
  this.browserView_ = new PROT_BrowserView(this.tabBrowser_);

  
  
  
  
  this.phishingWarden_.addBrowserView(this.browserView_);

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

  if (this.tabBrowser_)
    this.tabBrowser_.mTabBox.removeEventListener("select", this.onTabSwitchClosure_, true);
  
  this.tabBrowser_ = this.lastTab_ = null;

  this.win_.removeEventListener("unload", this.onShutdown_, false);
  this.prefs_ = null;

  G_Debug(this, "Controller shut down.");
}





PROT_Controller.prototype.onUserShowWarning = function() {
  var browser = this.tabBrowser_.selectedBrowser;
  this.browserView_.explicitShow(browser);
}








PROT_Controller.prototype.onUserAcceptWarning = function() {
  G_Debug(this, "User accepted warning.");
  var browser = this.tabBrowser_.selectedBrowser;
  G_Assert(this, !!browser, "Couldn't get current browser?!?");
  G_Assert(this, this.browserView_.hasProblem(browser),
           "User accept fired, but browser doesn't have warning showing?!?");

  this.browserView_.acceptAction(browser);
  this.browserView_.problemResolved(browser);
}








PROT_Controller.prototype.onUserDeclineWarning = function() {
  G_Debug(this, "User declined warning.");
  var browser = this.tabBrowser_.selectedBrowser;
  G_Assert(this, this.browserView_.hasProblem(browser),
           "User decline fired, but browser doesn't have warning showing?!?");
  this.browserView_.declineAction(browser);
  
  
  
  
}







PROT_Controller.prototype.onTabSwitch = function(e) {
  
  
  
  if (!e.target || (e.target.localName != "tabs" && e.target.localName != "tabpanels"))
    return;

  var fromBrowser = this.lastTab_;
  var toBrowser = this.tabBrowser_.selectedBrowser;

  if (fromBrowser != toBrowser) {
    this.lastTab_ = toBrowser;

    if (this.browserView_.hasProblem(fromBrowser)) 
      this.browserView_.problemBrowserUnselected(fromBrowser);

    if (this.browserView_.hasProblem(toBrowser))
      this.browserView_.problemBrowserSelected(toBrowser);
  }
}
