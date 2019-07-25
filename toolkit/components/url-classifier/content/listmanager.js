# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:











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
  this.updateInterval = this.prefs_.getPref("urlclassifier.updateinterval", 30 * 60) * 1000;

  this.updateserverURL_ = null;
  this.gethashURL_ = null;

  this.isTesting_ = false;

  this.tablesData = {};

  this.observerServiceObserver_ = new G_ObserverServiceObserver(
                                          'quit-application',
                                          BindToObject(this.shutdown_, this),
                                          true );

  
  
  this.keyManager_ = null;

  this.rekeyObserver_ = new G_ObserverServiceObserver(
                                          'url-classifier-rekey-requested',
                                          BindToObject(this.rekey_, this),
                                          false);
  this.updateWaitingForKey_ = false;

  this.cookieObserver_ = new G_ObserverServiceObserver(
                                          'cookie-changed',
                                          BindToObject(this.cookieChanged_, this),
                                          false);

  
  var backoffInterval = 30 * 60 * 1000;
  backoffInterval += Math.floor(Math.random() * (30 * 60 * 1000));

  this.requestBackoff_ = new RequestBackoff(2 ,
                                      60*1000 ,
                                            4 ,
                                   60*60*1000 ,
                              backoffInterval ,
                                 8*60*60*1000 );

  this.dbService_ = Cc["@mozilla.org/url-classifier/dbservice;1"]
                   .getService(Ci.nsIUrlClassifierDBService);

  this.hashCompleter_ = Cc["@mozilla.org/url-classifier/hashcompleter;1"]
                        .getService(Ci.nsIUrlClassifierHashCompleter);
}





PROT_ListManager.prototype.shutdown_ = function() {
  if (this.keyManager_) {
    this.keyManager_.shutdown();
  }

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




PROT_ListManager.prototype.setGethashUrl = function(url) {
  G_Debug(this, "Set gethash url: " + url);
  if (url != this.gethashURL_) {
    this.gethashURL_ = url;
    this.hashCompleter_.gethashUrl = url;
  }
}





PROT_ListManager.prototype.setKeyUrl = function(url) {
  G_Debug(this, "Set key url: " + url);
  if (!this.keyManager_) {
    this.keyManager_ = new PROT_UrlCryptoKeyManager();
    this.keyManager_.onNewKey(BindToObject(this.newKey_, this));

    this.hashCompleter_.setKeys(this.keyManager_.getClientKey(),
                                this.keyManager_.getWrappedKey());
  }

  this.keyManager_.setKeyUrl(url);
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

  
  var repeatingUpdateDelay = this.updateInterval / 2;
  repeatingUpdateDelay += Math.floor(Math.random() * this.updateInterval);
  this.updateChecker_ = new G_Alarm(BindToObject(this.initialUpdateCheck_,
                                                 this),
                                    repeatingUpdateDelay);
}






PROT_ListManager.prototype.initialUpdateCheck_ = function() {
  this.checkForUpdates();
  this.updateChecker_ = new G_Alarm(BindToObject(this.checkForUpdates, this), 
                                    this.updateInterval, true );
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
  if (!this.keyManager_)
    return;

  if (!this.keyManager_.hasKey()) {
    
    

    
    if (this.updateWaitingForKey_)
      return;

    
    
    
    
    if (this.keyManager_.maybeReKey())
      this.updateWaitingForKey_ = true;

    return;
  }

  var tableList;
  var tableNames = {};
  for (var tableName in this.tablesData) {
    if (this.tablesData[tableName].needsUpdate)
      tableNames[tableName] = true;
    if (!tableList) {
      tableList = tableName;
    } else {
      tableList += "," + tableName;
    }
  }

  var request = "";

  
  
  var lines = tableData.split("\n");
  for (var i = 0; i < lines.length; i++) {
    var fields = lines[i].split(";");
    if (tableNames[fields[0]]) {
      request += lines[i] + ":mac\n";
      delete tableNames[fields[0]];
    }
  }

  
  
  for (var tableName in tableNames) {
    request += tableName + ";mac\n";
  }

  G_Debug(this, 'checkForUpdates: scheduling request..');
  var streamer = Cc["@mozilla.org/url-classifier/streamupdater;1"]
                 .getService(Ci.nsIUrlClassifierStreamUpdater);
  try {
    streamer.updateUrl = this.updateserverURL_ +
                         "&wrkey=" + this.keyManager_.getWrappedKey();
  } catch (e) {
    G_Debug(this, 'invalid url');
    return;
  }

  this.requestBackoff_.noteRequest();

  if (!streamer.downloadUpdates(tableList,
                                request,
                                this.keyManager_.getClientKey(),
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

  
  this.requestBackoff_.noteServerResponse(200);
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

  if (this.requestBackoff_.isErrorStatus(status)) {
    
    this.currentUpdateChecker_ =
      new G_Alarm(BindToObject(this.checkForUpdates, this),
                  this.requestBackoff_.nextRequestDelay());
  }
}





PROT_ListManager.prototype.rekey_ = function() {
  G_Debug(this, "rekey requested");

  
  this.keyManager_.dropKey();
  this.keyManager_.maybeReKey();
}




PROT_ListManager.prototype.cookieChanged_ = function(subject, topic, data) {
  if (data != "cleared")
    return;

  G_Debug(this, "cookies cleared");
  this.keyManager_.dropKey();
}




PROT_ListManager.prototype.newKey_ = function() {
  G_Debug(this, "got a new MAC key");

  this.hashCompleter_.setKeys(this.keyManager_.getClientKey(),
                              this.keyManager_.getWrappedKey());

  if (this.keyManager_.hasKey()) {
    if (this.updateWaitingForKey_) {
      this.updateWaitingForKey_ = false;
      this.checkForUpdates();
    }
  }
}

PROT_ListManager.prototype.QueryInterface = function(iid) {
  if (iid.equals(Ci.nsISupports) ||
      iid.equals(Ci.nsIUrlListManager) ||
      iid.equals(Ci.nsITimerCallback))
    return this;

  throw Components.results.NS_ERROR_NO_INTERFACE;
}
