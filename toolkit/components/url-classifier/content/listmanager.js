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
#   Niels Provos <niels@google.com> (original author)
#   Fritz Schneider <fritz@google.com>
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












const kUpdateInterval = 30 * 60 * 1000;

function QueryAdapter(callback) {
  this.callback_ = callback;
};

QueryAdapter.prototype.handleResponse = function(value) {
  this.callback_.handleEvent(value);
}







function PROT_ListManager() {
  this.debugZone = "listmanager";
  G_debugService.enableZone(this.debugZone);

  this.currentUpdateChecker_ = null;   
  this.prefs_ = new G_Preferences();

  this.updateserverURL_ = null;

  this.isTesting_ = false;

  this.tablesData = {};

  this.observerServiceObserver_ = new G_ObserverServiceObserver(
                                          'xpcom-shutdown',
                                          BindToObject(this.shutdown_, this),
                                          true );

  
  this.urlCrypto_ = null;
  
  this.requestBackoff_ = new RequestBackoff(3 ,
                                   10*60*1000 ,
                                   60*60*1000 ,
                                   6*60*60*1000 );

  this.dbService_ = Cc["@mozilla.org/url-classifier/dbservice;1"]
                   .getService(Ci.nsIUrlClassifierDBService);
}





PROT_ListManager.prototype.shutdown_ = function() {
  for (var name in this.tablesData) {
    delete this.tablesData[name];
  }
}










PROT_ListManager.prototype.setUpdateUrl = function(url) {
  G_Debug(this, "Set update url: " + url);
  if (url != this.updateserverURL_) {
    this.updateserverURL_ = url;
    this.requestBackoff_.reset();
    
    
    for (var name in this.tablesData) {
      delete this.tablesData[name];
    }
  }
}





PROT_ListManager.prototype.setKeyUrl = function(url) {
  G_Debug(this, "Set key url: " + url);
  if (!this.urlCrypto_)
    this.urlCrypto_ = new PROT_UrlCrypto();
  
  this.urlCrypto_.manager_.setKeyUrl(url);
}







PROT_ListManager.prototype.registerTable = function(tableName, 
                                                    opt_requireMac) {
  this.tablesData[tableName] = {};
  this.tablesData[tableName].needsUpdate = false;

  return true;
}





PROT_ListManager.prototype.enableUpdate = function(tableName) {
  var changed = false;
  var table = this.tablesData[tableName];
  if (table) {
    G_Debug(this, "Enabling table updates for " + tableName);
    table.needsUpdate = true;
    changed = true;
  }

  if (changed === true)
    this.maybeToggleUpdateChecking();
}





PROT_ListManager.prototype.disableUpdate = function(tableName) {
  var changed = false;
  var table = this.tablesData[tableName];
  if (table) {
    G_Debug(this, "Disabling table updates for " + tableName);
    table.needsUpdate = false;
    changed = true;
  }

  if (changed === true)
    this.maybeToggleUpdateChecking();
}




PROT_ListManager.prototype.requireTableUpdates = function() {
  for (var type in this.tablesData) {
    
    if (this.tablesData[type].needsUpdate)
      return true;
  }

  return false;
}






PROT_ListManager.prototype.maybeStartManagingUpdates = function() {
  if (this.isTesting_)
    return;

  
  
  this.maybeToggleUpdateChecking();
}

PROT_ListManager.prototype.kickoffUpdate_ = function (tableData)
{
  this.startingUpdate_ = false;
  
  
  var initialUpdateDelay = 3000;
  if (tableData != "") {
    
    initialUpdateDelay += Math.floor(Math.random() * (5 * 60 * 1000));
  }

  this.currentUpdateChecker_ =
    new G_Alarm(BindToObject(this.checkForUpdates, this),
                initialUpdateDelay);
}




 
PROT_ListManager.prototype.maybeToggleUpdateChecking = function() {
  
  
  if (this.isTesting_)
    return;

  
  
  if (this.requireTableUpdates() === true) {
    G_Debug(this, "Starting managing lists");
    this.startUpdateChecker();

    
    
    if (!this.currentUpdateChecker && !this.startingUpdate_) {
      this.startingUpdate_ = true;
      
      this.dbService_.getTables(BindToObject(this.kickoffUpdate_, this));
    }
  } else {
    G_Debug(this, "Stopping managing lists (if currently active)");
    this.stopUpdateChecker();                    
  }
}








PROT_ListManager.prototype.startUpdateChecker = function() {
  this.stopUpdateChecker();
  
  
  var repeatingUpdateDelay = kUpdateInterval / 2;
  repeatingUpdateDelay += Math.floor(Math.random() * kUpdateInterval);
  this.updateChecker_ = new G_Alarm(BindToObject(this.initialUpdateCheck_,
                                                 this),
                                    repeatingUpdateDelay);
}






PROT_ListManager.prototype.initialUpdateCheck_ = function() {
  this.checkForUpdates();
  this.updateChecker_ = new G_Alarm(BindToObject(this.checkForUpdates, this), 
                                    kUpdateInterval, true );
}




PROT_ListManager.prototype.stopUpdateChecker = function() {
  if (this.updateChecker_) {
    this.updateChecker_.cancel();
    this.updateChecker_ = null;
  }
  
  if (this.currentUpdateChecker_) {
    this.currentUpdateChecker_.cancel();
    this.currentUpdateChecker_ = null;
  }
}












PROT_ListManager.prototype.safeLookup = function(key, callback) {
  try {
    G_Debug(this, "safeLookup: " + key);
    var cb = new QueryAdapter(callback);
    this.dbService_.lookup(key,
                           BindToObject(cb.handleResponse, cb),
                           true);
  } catch(e) {
    G_Debug(this, "safeLookup masked failure for key " + key + ": " + e);
    callback.handleEvent("");
  }
}







PROT_ListManager.prototype.checkForUpdates = function() {
  
  this.currentUpdateChecker_ = null;

  if (!this.updateserverURL_) {
    G_Debug(this, 'checkForUpdates: no update server url');
    return false;
  }

  
  if (!this.requestBackoff_.canMakeRequest())
    return false;

  
  this.dbService_.getTables(BindToObject(this.makeUpdateRequest_, this));
  return true;
}







PROT_ListManager.prototype.makeUpdateRequest_ = function(tableData) {
  var tableNames = {};
  for (var tableName in this.tablesData) {
    if (this.tablesData[tableName].needsUpdate)
      tableNames[tableName] = true;
  }

  var request = "";

  
  
  var lines = tableData.split("\n");
  for (var i = 0; i < lines.length; i++) {
    var fields = lines[i].split(";");
    if (tableNames[fields[0]]) {
      request += lines[i] + "\n";
      delete tableNames[fields[0]];
    }
  }

  
  
  for (var tableName in tableNames) {
    request += tableName + ";\n";
  }

  G_Debug(this, 'checkForUpdates: scheduling request..');
  var streamer = Cc["@mozilla.org/url-classifier/streamupdater;1"]
                 .getService(Ci.nsIUrlClassifierStreamUpdater);
  try {
    streamer.updateUrl = this.updateserverURL_;
  } catch (e) {
    G_Debug(this, 'invalid url');
    return;
  }

  if (!streamer.downloadUpdates(request,
                                BindToObject(this.updateSuccess_, this),
                                BindToObject(this.updateError_, this),
                                BindToObject(this.downloadError_, this))) {
    G_Debug(this, "pending update, wait until later");
  }
}






PROT_ListManager.prototype.updateSuccess_ = function(waitForUpdate) {
  G_Debug(this, "update success: " + waitForUpdate);
  if (waitForUpdate) {
    var delay = parseInt(waitForUpdate, 10);
    
    
    if (delay >= (5 * 60) && this.updateChecker_)
      this.updateChecker_.setDelay(delay * 1000);
  }
}





PROT_ListManager.prototype.updateError_ = function(result) {
  G_Debug(this, "update error: " + result);
  
}





PROT_ListManager.prototype.downloadError_ = function(status) {
  G_Debug(this, "download error: " + status);
  
  
  if (!status) {
    status = 500;
  }
  status = parseInt(status, 10);
  this.requestBackoff_.noteServerResponse(status);

  
  this.currentUpdateChecker_ =
    new G_Alarm(BindToObject(this.checkForUpdates, this), 60000);
}

PROT_ListManager.prototype.QueryInterface = function(iid) {
  if (iid.equals(Ci.nsISupports) ||
      iid.equals(Ci.nsIUrlListManager) ||
      iid.equals(Ci.nsITimerCallback))
    return this;

  Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
  return null;
}
