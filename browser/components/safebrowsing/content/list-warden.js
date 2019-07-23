






















































function PROT_ListWarden() {
  this.debugZone = "listwarden";
  var listManager = Cc["@mozilla.org/url-classifier/listmanager;1"]
                      .getService(Ci.nsIUrlListManager);
  this.listManager_ = listManager;

  
  this.blackTables_ = [];
  this.whiteTables_ = [];
}

PROT_ListWarden.IN_BLACKLIST = 0
PROT_ListWarden.IN_WHITELIST = 1
PROT_ListWarden.NOT_FOUND = 2





PROT_ListWarden.prototype.enableBlacklistTableUpdates = function() {
  for (var i = 0; i < this.blackTables_.length; ++i) {
    this.listManager_.enableUpdate(this.blackTables_[i]);
  }
}





PROT_ListWarden.prototype.disableBlacklistTableUpdates = function() {
  for (var i = 0; i < this.blackTables_.length; ++i) {
    this.listManager_.disableUpdate(this.blackTables_[i]);
  }
}





PROT_ListWarden.prototype.enableWhitelistTableUpdates = function() {
  for (var i = 0; i < this.whiteTables_.length; ++i) {
    this.listManager_.enableUpdate(this.whiteTables_[i]);
  }
}




PROT_ListWarden.prototype.disableWhitelistTableUpdates = function() {
  for (var i = 0; i < this.whiteTables_.length; ++i) {
    this.listManager_.disableUpdate(this.whiteTables_[i]);
  }
}







PROT_ListWarden.prototype.registerBlackTable = function(tableName) {
  var result = this.listManager_.registerTable(tableName, false);
  if (result) {
    this.blackTables_.push(tableName);
  }
  return result;
}







PROT_ListWarden.prototype.registerWhiteTable = function(tableName) {
  var result = this.listManager_.registerTable(tableName, false);
  if (result) {
    this.whiteTables_.push(tableName);
  }
  return result;
}









PROT_ListWarden.prototype.isWhiteURL = function(url, callback) {
  (new MultiTableQuerier(url,
                         this.whiteTables_,
                         [] ,
                         callback)).run();
}















PROT_ListWarden.prototype.isEvilURL = function(url, callback) {
  (new MultiTableQuerier(url,
                         this.whiteTables_,
                         this.blackTables_,
                         callback)).run();
}














function MultiTableQuerier(url, whiteTables, blackTables, callback) {
  this.debugZone = "multitablequerier";
  this.url_ = url;

  this.whiteTables_ = whiteTables;
  this.blackTables_ = blackTables;
  this.whiteIdx_ = 0;
  this.blackIdx_ = 0;

  this.callback_ = callback;
  this.listManager_ = Cc["@mozilla.org/url-classifier/listmanager;1"]
                      .getService(Ci.nsIUrlListManager);
}








MultiTableQuerier.prototype.run = function() {
  var whiteTable = this.whiteTables_[this.whiteIdx_];
  var blackTable = this.blackTables_[this.blackIdx_];
  if (whiteTable) {
    
    ++this.whiteIdx_;
    this.listManager_.safeExists(whiteTable, this.url_,
                                 BindToObject(this.whiteTableCallback_,
                                              this));
  } else if (blackTable) {
    
    ++this.blackIdx_;
    this.listManager_.safeExists(blackTable, this.url_,
                                 BindToObject(this.blackTableCallback_,
                                              this));
  } else {
    
    G_Debug(this, "Not found in any tables: " + this.url_);
    this.callback_(PROT_ListWarden.NOT_FOUND);

    
    this.callback_ = null;
    this.listManager_ = null;
  }
}





MultiTableQuerier.prototype.whiteTableCallback_ = function(isFound) {
  
  if (!isFound)
    this.run();
  else {
    G_Debug(this, "Found in whitelist: " + this.url_)
    this.callback_(PROT_ListWarden.IN_WHITELIST);

    
    this.callback_ = null;
    this.listManager_ = null;
  }
}





MultiTableQuerier.prototype.blackTableCallback_ = function(isFound) {
  
  if (!isFound) {
    this.run();
  } else {
    
    G_Debug(this, "Found in blacklist: " + this.url_)
    this.callback_(PROT_ListWarden.IN_BLACKLIST);

    
    this.callback_ = null;
    this.listManager_ = null;
  }
}
