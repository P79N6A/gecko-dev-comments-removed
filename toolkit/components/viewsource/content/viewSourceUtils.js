# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
# The Original Code is View Source Utilities.
#
# The Initial Developer of the Original Code is
# Jason Barnabe.
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
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








var gViewSourceUtils = {

  mnsIWebBrowserPersist: Components.interfaces.nsIWebBrowserPersist,
  mnsIWebProgress: Components.interfaces.nsIWebProgress,
  mnsIWebPageDescriptor: Components.interfaces.nsIWebPageDescriptor,

  
  openInInternalViewer: function(aURL, aPageDescriptor, aDocument)
  {
    
    var charset = null;
    if (aDocument)
      charset = "charset=" + aDocument.characterSet;
    openDialog("chrome://global/content/viewSource.xul",
               "_blank",
               "all,dialog=no",
               aURL, charset, aPageDescriptor);
  },

  
  
  openInExternalEditor: function(aURL, aPageDescriptor, aDocument, aCallBack)
  {
    var data = {url: aURL, pageDescriptor: aPageDescriptor, doc: aDocument};

    try {
      var editor = this.getExternalViewSourceEditor();    
      if (!editor) {
        this.handleCallBack(aCallBack, false, data);
        return;
      }

      
      var ios = Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService);
      var charset = aDocument ? aDocument.characterSet : null;
      var uri = ios.newURI(aURL, charset, null);
      data.uri = uri;

      var path;
      var contentType = aDocument ? aDocument.contentType : null;
      if (uri.scheme == "file") {    
        
        path = uri.QueryInterface(Components.interfaces.nsIFileURL).file.path;
        editor.run(false, [path], 1);
        this.handleCallBack(aCallBack, true, data);
      } else {
        
        this.viewSourceProgressListener.editor = editor;
        this.viewSourceProgressListener.callBack = aCallBack;
        this.viewSourceProgressListener.data = data;      
        if (!aPageDescriptor) {
          
          var file = this.getTemporaryFile(uri, aDocument, contentType);
          this.viewSourceProgressListener.file = file;

          var webBrowserPersist = Components
                                  .classes["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                                  .createInstance(this.mnsIWebBrowserPersist);
          
          webBrowserPersist.persistFlags = this.mnsIWebBrowserPersist.PERSIST_FLAGS_REPLACE_EXISTING_FILES;
          webBrowserPersist.progressListener = this.viewSourceProgressListener;
          webBrowserPersist.saveURI(uri, null, null, null, null, file);
        } else {
          
          
          var webShell = Components.classes["@mozilla.org/webshell;1"].createInstance();
          this.viewSourceProgressListener.webShell = webShell;
          var progress = webShell.QueryInterface(this.mnsIWebProgress);
          progress.addProgressListener(this.viewSourceProgressListener,
                                       this.mnsIWebProgress.NOTIFY_STATE_DOCUMENT);
          var pageLoader = webShell.QueryInterface(this.mnsIWebPageDescriptor);    
          pageLoader.loadPage(aPageDescriptor, this.mnsIWebPageDescriptor.DISPLAY_AS_SOURCE);
        }
      }
    } catch (ex) {
      
      Components.utils.reportError(ex);
      this.handleCallBack(aCallBack, false, data);
      return;
    }
  },

  
  internalViewerFallback: function(result, data)
  {
    if (!result) {
      this.openInInternalViewer(data.url, data.pageDescriptor, data.doc);
    }
  },

  
  handleCallBack: function(aCallBack, result, data)
  {
    
    if (aCallBack === undefined) {
      this.internalViewerFallback(result, data);
    } else if (aCallBack) {
      aCallBack(result, data);
    }
  },

  
  getExternalViewSourceEditor: function()
  {
    var editor = null;
    var viewSourceAppPath = null;
    try {
      var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefBranch);
      var prefPath = prefs.getCharPref("view_source.editor.path");
      if (prefPath.length > 0) {
        viewSourceAppPath = Components.classes["@mozilla.org/file/local;1"]
                                      .createInstance(Components.interfaces.nsILocalFile);
        viewSourceAppPath.initWithPath(prefPath);
        
        if (viewSourceAppPath.exists() && viewSourceAppPath.isExecutable()) {
          editor = Components.classes['@mozilla.org/process/util;1']
                             .createInstance(Components.interfaces.nsIProcess);
          editor.init(viewSourceAppPath);
        }
      }
    }
    catch (ex) {
      Components.utils.reportError(ex);
    }
    return editor;
  },

  viewSourceProgressListener: {

    mnsIWebProgressListener: Components.interfaces.nsIWebProgressListener,

    QueryInterface: function(aIID) {
     if (aIID.equals(this.mnsIWebProgressListener) ||
         aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
         aIID.equals(Components.interfaces.nsISupports))
       return this;
     throw Components.results.NS_NOINTERFACE;
    },

    destroy: function() {
        this.webShell = null;
        this.editor = null;
        this.callBack = null;
        this.data = null;
        this.file = null;
    },

    onStateChange: function(aProgress, aRequest, aFlag, aStatus) {
      
      if ((aFlag & this.mnsIWebProgressListener.STATE_STOP) && aStatus == 0) {
        try {
          if (!this.file) {
            

            
            
            this.file = gViewSourceUtils.getTemporaryFile(this.data.uri, this.data.doc, 
                                                          this.data.doc.contentType);

            
            var webNavigation = this.webShell.QueryInterface(Components.interfaces.nsIWebNavigation);
            var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                                     .createInstance(Components.interfaces.nsIFileOutputStream);
            foStream.init(this.file, 0x02 | 0x08 | 0x20, 0664, 0); 
            var coStream = Components.classes["@mozilla.org/intl/converter-output-stream;1"]
                                     .createInstance(Components.interfaces.nsIConverterOutputStream);
            coStream.init(foStream, this.data.doc.characterSet, 0, null);

            
            coStream.writeString(webNavigation.document.body.textContent);
          
            
            coStream.close();
            foStream.close();
          }
          
          this.editor.run(false, [this.file.path], 1);

          gViewSourceUtils.handleCallBack(this.callBack, true, this.data);
        } catch (ex) {
          
          Components.utils.reportError(ex);
          gViewSourceUtils.handleCallBack(this.callBack, false, this.data);
        } finally {
          this.destroy();
        }
      }
      return 0;
    },

    onLocationChange: function() {return 0;},
    onProgressChange: function() {return 0;},
    onStatusChange: function() {return 0;},
    onSecurityChange: function() {return 0;},
    onLinkIconAvailable: function() {return 0;},

    webShell: null,
    editor: null,
    callBack: null,
    data: null,
    file: null
  },

  
  getTemporaryFile: function(aURI, aDocument, aContentType) {
    var fileLocator = Components.classes["@mozilla.org/file/directory_service;1"]
                                .getService(Components.interfaces.nsIProperties);
    var tempFile = fileLocator.get("TmpD", Components.interfaces.nsIFile);
    var fileName = getDefaultFileName(null, aURI, aDocument, aContentType);
    var extension = getDefaultExtension(fileName, aURI, aContentType);
    var leafName = getNormalizedLeafName(fileName, extension);
    tempFile.append(leafName);
    return tempFile;
  }
}
