# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Google Safe Browsing.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Fritz Schneider <fritz@google.com> (original author)
#   Simon BÃ¼nzli <zeniko@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****





























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
