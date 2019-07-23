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
#   Tony Chang <tony@google.com> (original author)
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














function MultiQuerier(tokens, tableName, callback) {
  this.tokens_ = tokens;
  this.tableName_ = tableName;
  this.callback_ = callback;
  this.dbservice_ = Cc["@mozilla.org/url-classifier/dbservice;1"]
                    .getService(Ci.nsIUrlClassifierDBService);
  
  this.key_ = null;
}




MultiQuerier.prototype.run = function() {
  if (this.tokens_.length == 0) {
    this.callback_.handleEvent(false);
    this.dbservice_ = null;
    this.callback_ = null;
    return;
  }
  
  this.key_ = this.tokens_.pop();
  G_Debug(this, "Looking up " + this.key_ + " in " + this.tableName_);
  this.dbservice_.exists(this.tableName_, this.key_,
                         BindToObject(this.result_, this));
}





MultiQuerier.prototype.result_ = function(value) {
  if (this.condition_(value)) {
    this.callback_.handleEvent(true)
    this.dbservice_ = null;
    this.callback_ = null;
  } else {
    this.run();
  }
}


MultiQuerier.prototype.condition_ = function(value) {
  throw "MultiQuerier is an abstract base class";
}





function ExistsMultiQuerier(tokens, tableName, callback) {
  MultiQuerier.call(this, tokens, tableName, callback);
  this.debugZone = "existsMultiQuerier";
}
ExistsMultiQuerier.inherits(MultiQuerier);

ExistsMultiQuerier.prototype.condition_ = function(value) {
  return value.length > 0;
}







function EnchashMultiQuerier(tokens, tableName, callback, url) {
  MultiQuerier.call(this, tokens, tableName, callback);
  this.url_ = url;
  this.enchashDecrypter_ = new PROT_EnchashDecrypter();
  this.debugZone = "enchashMultiQuerier";
}
EnchashMultiQuerier.inherits(MultiQuerier);

EnchashMultiQuerier.prototype.run = function() {
  if (this.tokens_.length == 0) {
    this.callback_.handleEvent(false);
    this.dbservice_ = null;
    this.callback_ = null;
    return;
  }
  var host = this.tokens_.pop();
  this.key_ = host;
  var lookupKey = this.enchashDecrypter_.getLookupKey(host);
  this.dbservice_.exists(this.tableName_, lookupKey,
                         BindToObject(this.result_, this));
}

EnchashMultiQuerier.prototype.condition_ = function(encryptedValue) {
  if (encryptedValue.length > 0) {
    
    
    var decrypted = this.enchashDecrypter_.decryptData(encryptedValue,
                                                       this.key_);
    var res = this.enchashDecrypter_.parseRegExps(decrypted);
    for (var j = 0; j < res.length; j++) {
      if (res[j].test(this.url_)) {
        return true;
      }
    }
  }
  return false;
}
