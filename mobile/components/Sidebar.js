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
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1999
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Stephen Lamm            <slamm@netscape.com>
#   Robert John Churchill   <rjc@netscape.com>
#   David Hyatt             <hyatt@mozilla.org>
#   Christopher A. Aillon   <christopher@aillon.com>
#   Myk Melez               <myk@mozilla.org>
#   Pamela Greene           <pamg.bugs@gmail.com>
#   Gavin Sharp             <gavin@gavinsharp.com>
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

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const SIDEBAR_CID = Components.ID("{22117140-9c6e-11d3-aaf1-00805f8a4905}");
const SIDEBAR_CONTRACTID = "@mozilla.org/sidebar;1";

function Sidebar() {
  
  
  var appInfo = Cc["@mozilla.org/xre/app-info;1"];
  if (!appInfo || appInfo.getService(Ci.nsIXULRuntime).processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
    

    this.inContentProcess = false;

    
    this.wrappedJSObject = this;

    
    
    this.receiveMessage = function(aMessage) {
      switch (aMessage.name) {
        case "Sidebar:AddSearchProvider":
          this.AddSearchProvider(aMessage.json.descriptionURL);
      }
    };
  } else {
    

    this.inContentProcess = true;
    this.messageManager = Cc["@mozilla.org/childprocessmessagemanager;1"].getService(Ci.nsISyncMessageSender);
  }
}

Sidebar.prototype = {
  
  _validateSearchEngine: function validateSearchEngine(engineURL, iconURL) {
    try {
      
      if (! /^(https?|ftp):\/\//i.test(engineURL))
        throw "Unsupported search engine URL";
    
      
      
      if (iconURL &&
          ! /^(https?|ftp):\/\/.+\.(gif|jpg|jpeg|png|ico)$/i.test(iconURL))
        throw "Unsupported search icon URL.";
    } catch(ex) {
      Cu.reportError("Invalid argument passed to window.sidebar.addSearchEngine: " + ex);
      
      var searchBundle = Services.strings.createBundle("chrome://global/locale/search/search.properties");
      var brandBundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");
      var brandName = brandBundle.GetStringFromName("brandShortName");
      var title = searchBundle.GetStringFromName("error_invalid_engine_title");
      var msg = searchBundle.formatStringFromName("error_invalid_engine_msg",
                                                  [brandName], 1);
      Services.prompt.alert(null, title, msg);
      return false;
    }
    
    return true;
  },

  
  addPanel: function addPanel(aTitle, aContentURL, aCustomizeURL) {
    
  },

  addPersistentPanel: function addPersistentPanel(aTitle, aContentURL, aCustomizeURL) {
    
  },

  
  
  addSearchEngine: function addSearchEngine(engineURL, iconURL, suggestedTitle,
                                            suggestedCategory) {
    if (!this._validateSearchEngine(engineURL, iconURL))
      return;

    
    const SHERLOCK_FILE_EXT_REGEXP = /\.src$/i;

    
    
    
    var dataType;
    if (SHERLOCK_FILE_EXT_REGEXP.test(engineURL))
      dataType = Ci.nsISearchEngine.DATA_TEXT;
    else
      dataType = Ci.nsISearchEngine.DATA_XML;

    Services.search.addEngine(engineURL, dataType, iconURL, true);
  },

  
  
  
  
  AddSearchProvider: function AddSearchProvider(aDescriptionURL) {
    if (!this._validateSearchEngine(aDescriptionURL, ""))
      return;

    if (this.inContentProcess) {
      this.messageManager.sendAsyncMessage("Sidebar:AddSearchProvider",
        { descriptionURL: aDescriptionURL });
      return;
    }

    const typeXML = Ci.nsISearchEngine.DATA_XML;
    Services.search.addEngine(aDescriptionURL, typeXML, "", true);
  },

  
  
  
  
  
  
  
  
  IsSearchProviderInstalled: function IsSearchProviderInstalled(aSearchURL) {
    return 0;
  },

  
  classInfo: XPCOMUtils.generateCI({classID: SIDEBAR_CID,
                                    contractID: SIDEBAR_CONTRACTID,
                                    interfaces: [Ci.nsISidebar,
                                                 Ci.nsISidebarExternal],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "Sidebar"}),

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISidebar,
                                         Ci.nsISidebarExternal]),

  
  classID: SIDEBAR_CID,
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([Sidebar]);
