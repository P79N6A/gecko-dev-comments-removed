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










































const kTableVersionPrefPrefix = "urlclassifier.tableversion.";


const kUpdateInterval = 30 * 60 * 1000;







function PROT_ListManager() {
  this.debugZone = "listmanager";
  G_debugService.enableZone(this.debugZone);

  this.currentUpdateChecker_ = null;   
  this.prefs_ = new G_Preferences();

  this.updateserverURL_ = null;

  
  
  
  
  this.tablesKnown_ = {};
  this.isTesting_ = false;
  
  if (this.isTesting_) {
    
    this.tablesKnown_ = {
      
      "test1-foo-domain" : new PROT_VersionParser("test1-foo-domain", 0, -1),
      "test2-foo-domain" : new PROT_VersionParser("test2-foo-domain", 0, -1),
      "test-white-domain" : 
        new PROT_VersionParser("test-white-domain", 0, -1, true ),
      "test-mac-domain" :
        new PROT_VersionParser("test-mac-domain", 0, -1, true )
    };
    
    
    this.wrappedJSObject = this;
  }

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
      delete this.tablesKnown_[name];
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
  var table = new PROT_VersionParser(tableName, 1, -1, opt_requireMac);
  if (!table)
    return false;
  this.tablesKnown_[tableName] = table;
  this.tablesData[tableName] = newUrlClassifierTable(tableName);

  return true;
}





PROT_ListManager.prototype.enableUpdate = function(tableName) {
  var changed = false;
  var table = this.tablesKnown_[tableName];
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
  var table = this.tablesKnown_[tableName];
  if (table) {
    G_Debug(this, "Disabling table updates for " + tableName);
    table.needsUpdate = false;
    changed = true;
  }

  if (changed === true)
    this.maybeToggleUpdateChecking();
}




PROT_ListManager.prototype.requireTableUpdates = function() {
  for (var type in this.tablesKnown_) {
    
    
    if (this.tablesKnown_[type].major == 0)
      continue;
     
    
    if (this.tablesKnown_[type].needsUpdate)
      return true;
  }

  return false;
}






PROT_ListManager.prototype.maybeStartManagingUpdates = function() {
  if (this.isTesting_)
    return;

  
  
  this.maybeToggleUpdateChecking();
}




 
PROT_ListManager.prototype.maybeToggleUpdateChecking = function() {
  
  
  if (this.isTesting_)
    return;

  
  
  if (this.requireTableUpdates() === true) {
    G_Debug(this, "Starting managing lists");
    this.startUpdateChecker();

    
    
    if (!this.currentUpdateChecker_) {
      
      
      this.loadTableVersions_();
      var hasTables = false;
      for (var table in this.tablesKnown_) {
        if (this.tablesKnown_[table].minor != -1) {
          hasTables = true;
          break;
        }
      }

      var initialUpdateDelay = 3000;
      if (hasTables) {
        
        initialUpdateDelay += Math.floor(Math.random() * (5 * 60 * 1000));
      }
      this.currentUpdateChecker_ =
        new G_Alarm(BindToObject(this.checkForUpdates, this),
                    initialUpdateDelay);
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












PROT_ListManager.prototype.safeExists = function(table, key, callback) {
  try {
    G_Debug(this, "safeExists: " + table + ", " + key);
    var map = this.tablesData[table];
    map.exists(key, callback);
  } catch(e) {
    G_Debug(this, "safeExists masked failure for " + table + ", key " + key + ": " + e);
    callback.handleEvent(false);
  }
}





PROT_ListManager.prototype.loadTableVersions_ = function() {
  
  var prefBase = kTableVersionPrefPrefix;
  for (var table in this.tablesKnown_) {
    var version = this.prefs_.getPref(prefBase + table, "1.-1");
    G_Debug(this, "loadTableVersion " + table + ": " + version);
    var tokens = version.split(".");
    G_Assert(this, tokens.length == 2, "invalid version number");
    
    this.tablesKnown_[table].major = tokens[0];
    this.tablesKnown_[table].minor = tokens[1];
  }
}







PROT_ListManager.prototype.setTableVersion_ = function(versionString) {
  G_Debug(this, "Got version string: " + versionString);
  var versionParser = new PROT_VersionParser("");
  if (versionParser.fromString(versionString)) {
    var tableName = versionParser.type;
    var versionNumber = versionParser.versionString();
    var prefBase = kTableVersionPrefPrefix;

    this.prefs_.setPref(prefBase + tableName, versionNumber);
    
    if (!this.tablesKnown_[tableName]) {
      this.tablesKnown_[tableName] = versionParser;
    } else {
      this.tablesKnown_[tableName].ImportVersion(versionParser);
    }
    
    if (!this.tablesData[tableName])
      this.tablesData[tableName] = newUrlClassifierTable(tableName);
  }

  
  
  
  this.requestBackoff_.noteServerResponse(200 );
}









PROT_ListManager.prototype.getRequestURL_ = function(url) {
  url += "version=";
  var firstElement = true;
  var requestMac = false;

  for (var type in this.tablesKnown_) {
    
    
    if (this.tablesKnown_[type].major == 0)
      continue;

    
    if (this.tablesKnown_[type].needsUpdate == false)
      continue;

    if (!firstElement) {
      url += ","
    } else {
      firstElement = false;
    }
    url += type + ":" + this.tablesKnown_[type].toUrl();

    if (this.tablesKnown_[type].requireMac)
      requestMac = true;
  }

  
  
  if (requestMac) {
    
    if (!this.urlCrypto_)
      this.urlCrypto_ = new PROT_UrlCrypto();

    url += "&wrkey=" +
      encodeURIComponent(this.urlCrypto_.getManager().getWrappedKey());
  }

  G_Debug(this, "getRequestURL returning: " + url);
  return url;
}







PROT_ListManager.prototype.checkForUpdates = function() {
  
  this.currentUpdateChecker_ = null;

  if (!this.updateserverURL_) {
    G_Debug(this, 'checkForUpdates: no update server url');
    return false;
  }

  
  if (!this.requestBackoff_.canMakeRequest())
    return false;

  
  
  
  var tableNames = [];
  for (var tableName in this.tablesKnown_) {
    tableNames.push(tableName);
  }
  var dbService = Cc["@mozilla.org/url-classifier/dbservice;1"]
                  .getService(Ci.nsIUrlClassifierDBService);
  dbService.checkTables(tableNames.join(","),
                        BindToObject(this.makeUpdateRequest_, this));
  return true;
}







PROT_ListManager.prototype.makeUpdateRequest_ = function(tableNames) {
  
  var tables = tableNames.split(",");
  for (var i = 0; i < tables.length; ++i) {
    G_Debug(this, "Table |" + tables[i] + "| no longer exists, clearing pref.");
    this.prefs_.clearPref(kTableVersionPrefPrefix + tables[i]);
  }

  
  this.loadTableVersions_();

  G_Debug(this, 'checkForUpdates: scheduling request..');
  var url = this.getRequestURL_(this.updateserverURL_);
  var streamer = Cc["@mozilla.org/url-classifier/streamupdater;1"]
                 .getService(Ci.nsIUrlClassifierStreamUpdater);
  try {
    streamer.updateUrl = url;
  } catch (e) {
    G_Debug(this, 'invalid url');
    return;
  }

  if (!streamer.downloadUpdates(BindToObject(this.setTableVersion_, this),
                                BindToObject(this.downloadError_, this))) {
    G_Debug(this, "pending update, wait until later");
  }
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





function newUrlClassifierTable(name) {
  G_Debug("protfactory", "Creating a new nsIUrlClassifierTable: " + name);
  var tokens = name.split('-');
  var type = tokens[2];
  var table = Cc['@mozilla.org/url-classifier/table;1?type=' + type]
                .createInstance(Ci.nsIUrlClassifierTable);
  table.name = name;
  return table;
}
