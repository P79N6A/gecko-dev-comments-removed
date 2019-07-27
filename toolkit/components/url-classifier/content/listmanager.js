# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

const Cu = Components.utils;
Cu.import("resource://gre/modules/Services.jsm");












this.log = function log(...stuff) {
  var prefs_ = new G_Preferences();
  var debug = prefs_.getPref("browser.safebrowsing.debug");
  if (!debug) {
    return;
  }

  var d = new Date();
  let msg = "listmanager: " + d.toTimeString() + ": " + stuff.join(" ");
  Services.console.logStringMessage(msg);
  dump(msg + "\n");
}

this.QueryAdapter = function QueryAdapter(callback) {
  this.callback_ = callback;
};

QueryAdapter.prototype.handleResponse = function(value) {
  this.callback_.handleEvent(value);
}







this.PROT_ListManager = function PROT_ListManager() {
  log("Initializing list manager");
  this.prefs_ = new G_Preferences();
  this.updateInterval = this.prefs_.getPref("urlclassifier.updateinterval", 30 * 60) * 1000;

  
  
  this.tablesData = {};
  
  
  
  this.needsUpdate_ = {};

  this.observerServiceObserver_ = new G_ObserverServiceObserver(
                                          'quit-application',
                                          BindToObject(this.shutdown_, this),
                                          true );

  
  
  
  
  this.updateCheckers_ = {};
  this.requestBackoffs_ = {};
  this.dbService_ = Cc["@mozilla.org/url-classifier/dbservice;1"]
                   .getService(Ci.nsIUrlClassifierDBService);


  this.hashCompleter_ = Cc["@mozilla.org/url-classifier/hashcompleter;1"]
                        .getService(Ci.nsIUrlClassifierHashCompleter);
}





PROT_ListManager.prototype.shutdown_ = function() {
  for (var name in this.tablesData) {
    delete this.tablesData[name];
  }
}








PROT_ListManager.prototype.registerTable = function(tableName,
                                                    updateUrl,
                                                    gethashUrl) {
  log("registering " + tableName + " with " + updateUrl);
  if (!updateUrl) {
    log("Can't register table " + tableName + " without updateUrl");
    return false;
  }
  this.tablesData[tableName] = {};
  this.tablesData[tableName].updateUrl = updateUrl;
  this.tablesData[tableName].gethashUrl = gethashUrl;

  
  if (!this.needsUpdate_[updateUrl]) {
    this.needsUpdate_[updateUrl] = {};
    
    var backoffInterval = 30 * 60 * 1000;
    backoffInterval += Math.floor(Math.random() * (30 * 60 * 1000));

    log("Creating request backoff for " + updateUrl);
    this.requestBackoffs_[updateUrl] = new RequestBackoff(2 ,
                                      60*1000 ,
                                            4 ,
                                   60*60*1000 ,
                              backoffInterval ,
                                 8*60*60*1000 );

  }
  this.needsUpdate_[updateUrl][tableName] = false;

  return true;
}

PROT_ListManager.prototype.getGethashUrl = function(tableName) {
  if (this.tablesData[tableName] && this.tablesData[tableName].gethashUrl) {
    return this.tablesData[tableName].gethashUrl;
  }
  return "";
}





PROT_ListManager.prototype.enableUpdate = function(tableName) {
  var table = this.tablesData[tableName];
  if (table) {
    log("Enabling table updates for " + tableName);
    this.needsUpdate_[table.updateUrl][tableName] = true;
  }
}





PROT_ListManager.prototype.updatesNeeded_ = function(updateUrl) {
  let updatesNeeded = false;
  for (var tableName in this.needsUpdate_[updateUrl]) {
    if (this.needsUpdate_[updateUrl][tableName]) {
      updatesNeeded = true;
    }
  }
  return updatesNeeded;
}





PROT_ListManager.prototype.disableUpdate = function(tableName) {
  var table = this.tablesData[tableName];
  if (table) {
    log("Disabling table updates for " + tableName);
    this.needsUpdate_[table.updateUrl][tableName] = false;
    if (!this.updatesNeeded_(table.updateUrl) &&
        this.updateCheckers_[table.updateUrl]) {
      this.updateCheckers_[table.updateUrl].cancel();
      this.updateCheckers_[table.updateUrl] = null;
    }
  }
}




PROT_ListManager.prototype.requireTableUpdates = function() {
  for (var name in this.tablesData) {
    
    if (this.needsUpdate_[this.tablesData[name].updateUrl][name]) {
      return true;
    }
  }

  return false;
}




PROT_ListManager.prototype.kickoffUpdate_ = function (onDiskTableData)
{
  this.startingUpdate_ = false;
  var initialUpdateDelay = 3000;

  
  log("needsUpdate: " + JSON.stringify(this.needsUpdate_, undefined, 2));
  for (var updateUrl in this.needsUpdate_) {
    
    
    
    
    
    if (this.updatesNeeded_(updateUrl) && !this.updateCheckers_[updateUrl]) {
      log("Initializing update checker for " + updateUrl);
      this.updateCheckers_[updateUrl] =
        new G_Alarm(BindToObject(this.checkForUpdates, this, updateUrl),
                    initialUpdateDelay, false );
    } else {
      log("No updates needed or already initialized for " + updateUrl);
    }
  }
}

PROT_ListManager.prototype.stopUpdateCheckers = function() {
  log("Stopping updates");
  for (var updateUrl in this.updateCheckers_) {
    this.updateCheckers_[updateUrl].cancel();
    this.updateCheckers_[updateUrl] = null;
  }
}





PROT_ListManager.prototype.maybeToggleUpdateChecking = function() {
  
  
  if (this.requireTableUpdates()) {
    log("Starting managing lists");

    
    
    if (!this.startingUpdate_) {
      this.startingUpdate_ = true;
      
      this.dbService_.getTables(BindToObject(this.kickoffUpdate_, this));
    }
  } else {
    log("Stopping managing lists (if currently active)");
    this.stopUpdateCheckers();                    
  }
}












PROT_ListManager.prototype.safeLookup = function(key, callback) {
  try {
    log("safeLookup: " + key);
    var cb = new QueryAdapter(callback);
    this.dbService_.lookup(key,
                           BindToObject(cb.handleResponse, cb),
                           true);
  } catch(e) {
    log("safeLookup masked failure for key " + key + ": " + e);
    callback.handleEvent("");
  }
}







PROT_ListManager.prototype.checkForUpdates = function(updateUrl) {
  log("checkForUpdates with " + updateUrl);
  
  if (!updateUrl) {
    return false;
  }
  if (!this.requestBackoffs_[updateUrl] ||
      !this.requestBackoffs_[updateUrl].canMakeRequest()) {
    log("Can't make update request");
    return false;
  }
  
  this.dbService_.getTables(BindToObject(this.makeUpdateRequest_, this,
                            updateUrl));
  return true;
}







PROT_ListManager.prototype.makeUpdateRequest_ = function(updateUrl, tableData) {
  log("this.tablesData: " + JSON.stringify(this.tablesData, undefined, 2));
  log("existing chunks: " + tableData + "\n");
  
  if (!updateUrl) {
    return;
  }
  
  
  
  
  
  var streamerMap = { tableList: null, tableNames: {}, request: "" };
  for (var tableName in this.tablesData) {
    
    if (this.tablesData[tableName].updateUrl != updateUrl) {
      continue;
    }
    if (this.needsUpdate_[this.tablesData[tableName].updateUrl][tableName]) {
      streamerMap.tableNames[tableName] = true;
    }
    if (!streamerMap.tableList) {
      streamerMap.tableList = tableName;
    } else {
      streamerMap.tableList += "," + tableName;
    }
  }
  
  
  var lines = tableData.split("\n");
  for (var i = 0; i < lines.length; i++) {
    var fields = lines[i].split(";");
    var name = fields[0];
    if (streamerMap.tableNames[name]) {
      streamerMap.request += lines[i] + "\n";
      delete streamerMap.tableNames[name];
    }
  }
  
  
  for (let tableName in streamerMap.tableNames) {
    streamerMap.request += tableName + ";\n";
  }

  log("update request: " + JSON.stringify(streamerMap, undefined, 2) + "\n");

  
  if (streamerMap.request.length > 0) {
    this.makeUpdateRequestForEntry_(updateUrl, streamerMap.tableList,
                                    streamerMap.request);
  } else {
    
    log("Not sending empty request");
  }
}

PROT_ListManager.prototype.makeUpdateRequestForEntry_ = function(updateUrl,
                                                                 tableList,
                                                                 request) {
  log("makeUpdateRequestForEntry_: request " + request +
      " update: " + updateUrl + " tablelist: " + tableList + "\n");
  var streamer = Cc["@mozilla.org/url-classifier/streamupdater;1"]
                 .getService(Ci.nsIUrlClassifierStreamUpdater);

  this.requestBackoffs_[updateUrl].noteRequest();

  if (!streamer.downloadUpdates(
        tableList,
        request,
        updateUrl,
        BindToObject(this.updateSuccess_, this, tableList, updateUrl),
        BindToObject(this.updateError_, this, tableList, updateUrl),
        BindToObject(this.downloadError_, this, tableList, updateUrl))) {
    
    log("pending update, queued request until later");
  }
}






PROT_ListManager.prototype.updateSuccess_ = function(tableList, updateUrl,
                                                     waitForUpdate) {
  log("update success for " + tableList + " from " + updateUrl + ": " +
      waitForUpdate + "\n");
  var delay;
  if (waitForUpdate) {
    delay = parseInt(waitForUpdate, 10);
  }
  
  
  
  if (delay >= (5 * 60)) {
    log("Waiting " + delay + " seconds");
    delay = delay * 1000;
  } else {
    log("Ignoring delay from server, waiting " + this.updateInterval / 1000);
    delay = this.updateInterval;
  }
  this.updateCheckers_[updateUrl] =
    new G_Alarm(BindToObject(this.checkForUpdates, this, updateUrl),
                delay, false);

  
  this.requestBackoffs_[updateUrl].noteServerResponse(200);
}





PROT_ListManager.prototype.updateError_ = function(table, updateUrl, result) {
  log("update error for " + table + " from " + updateUrl + ": " + result + "\n");
  
  
  this.updateCheckers_[updateUrl] =
    new G_Alarm(BindToObject(this.checkForUpdates, this, updateUrl),
                this.updateInterval, false);
}





PROT_ListManager.prototype.downloadError_ = function(table, updateUrl, status) {
  log("download error for " + table + ": " + status + "\n");
  
  
  if (!status) {
    status = 500;
  }
  status = parseInt(status, 10);
  this.requestBackoffs_[updateUrl].noteServerResponse(status);
  var delay = this.updateInterval;
  if (this.requestBackoffs_[updateUrl].isErrorStatus(status)) {
    
    delay = this.requestBackoffs_[updateUrl].nextRequestDelay();
  } else {
    log("Got non error status for error callback?!");
  }
  this.updateCheckers_[updateUrl] =
    new G_Alarm(BindToObject(this.checkForUpdates, this, updateUrl),
                delay, false);

}

PROT_ListManager.prototype.QueryInterface = function(iid) {
  if (iid.equals(Ci.nsISupports) ||
      iid.equals(Ci.nsIUrlListManager) ||
      iid.equals(Ci.nsITimerCallback))
    return this;

  throw Components.results.NS_ERROR_NO_INTERFACE;
}
