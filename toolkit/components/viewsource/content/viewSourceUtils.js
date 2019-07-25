













var gViewSourceUtils = {

  mnsIWebBrowserPersist: Components.interfaces.nsIWebBrowserPersist,
  mnsIWebProgress: Components.interfaces.nsIWebProgress,
  mnsIWebPageDescriptor: Components.interfaces.nsIWebPageDescriptor,

  
  viewSource: function(aURL, aPageDescriptor, aDocument, aLineNumber)
  {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch);
    if (prefs.getBoolPref("view_source.editor.external"))
      this.openInExternalEditor(aURL, aPageDescriptor, aDocument, aLineNumber);
    else
      this.openInInternalViewer(aURL, aPageDescriptor, aDocument, aLineNumber);
  },

  
  openInInternalViewer: function(aURL, aPageDescriptor, aDocument, aLineNumber)
  {
    
    var charset = null;
    var isForcedCharset = false;
    if (aDocument) {
      charset = "charset=" + aDocument.characterSet;
      try { 
        isForcedCharset =
          aDocument.defaultView
                   .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                   .getInterface(Components.interfaces.nsIDOMWindowUtils)
                   .docCharsetIsForced;
      } catch (ex) {
      }
    }
    openDialog("chrome://global/content/viewSource.xul",
               "_blank",
               "all,dialog=no",
               aURL, charset, aPageDescriptor, aLineNumber, isForcedCharset);
  },

  buildEditorArgs: function(aPath, aLineNumber) {
    
    
    
    var editorArgs = [];
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch);
    var args = prefs.getCharPref("view_source.editor.args");
    if (args) {
      args = args.replace("%LINE%", aLineNumber || "0");
      
      const argumentRE = /"([^"]+)"|(\S+)/g;
      while (argumentRE.test(args))
        editorArgs.push(RegExp.$1 || RegExp.$2);
    }
    editorArgs.push(aPath);
    return editorArgs;
  },

  
  
  openInExternalEditor: function(aURL, aPageDescriptor, aDocument, aLineNumber, aCallBack)
  {
    var data = {url: aURL, pageDescriptor: aPageDescriptor, doc: aDocument,
                lineNumber: aLineNumber};

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

        var editorArgs = this.buildEditorArgs(path, data.lineNumber);
        editor.runw(false, editorArgs, editorArgs.length);
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

          let fromPrivateWindow = false;
          if (aDocument) {
            try {
              fromPrivateWindow =
                aDocument.defaultView
                         .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                         .getInterface(Components.interfaces.nsIWebNavigation)
                         .QueryInterface(Components.interfaces.nsILoadContext)
                         .usePrivateBrowsing;
            } catch (e) {
            }
          }

          let helperService = Components.classes["@mozilla.org/uriloader/external-helper-app-service;1"]
                                        .getService(Components.interfaces.nsPIExternalAppLauncher);
          if (fromPrivateWindow) {
            
            helperService.deleteTemporaryPrivateFileWhenPossible(file);
          } else {
            
            helperService.deleteTemporaryFileOnExit(file);
          }
        } else {
          
          
          
          
          
          var webShell = Components.classes["@mozilla.org/docshell;1"].createInstance();
          webShell.QueryInterface(Components.interfaces.nsIBaseWindow).create();
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
      this.openInInternalViewer(data.url, data.pageDescriptor, data.doc, data.lineNumber);
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
    try {
      let viewSourceAppPath =
          Components.classes["@mozilla.org/preferences-service;1"]
                    .getService(Components.interfaces.nsIPrefBranch)
                    .getComplexValue("view_source.editor.path",
                                     Components.interfaces.nsIFile);
      let editor = Components.classes['@mozilla.org/process/util;1']
                             .createInstance(Components.interfaces.nsIProcess);
      editor.init(viewSourceAppPath);

      return editor;
    }
    catch (ex) {
      Components.utils.reportError(ex);
    }

    return null;
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
      if (this.webShell) {
        this.webShell.QueryInterface(Components.interfaces.nsIBaseWindow).destroy();
      }
      this.webShell = null;
      this.editor = null;
      this.callBack = null;
      this.data = null;
      this.file = null;
    },

    
    
    
    onStateChange: function(aProgress, aRequest, aFlag, aStatus) {
      
      if ((aFlag & this.mnsIWebProgressListener.STATE_STOP) && aStatus == 0) {
        if (!this.webShell) {
          
          
          this.onContentLoaded();
          return 0;
        }
        var webNavigation = this.webShell.QueryInterface(Components.interfaces.nsIWebNavigation);
        if (webNavigation.document.readyState == "complete") {
          
          this.onContentLoaded();
        } else {
          webNavigation.document.addEventListener("DOMContentLoaded",
                                                  this.onContentLoaded.bind(this));
        }
      }
      return 0;
    },

    onContentLoaded: function() {
      try {
        if (!this.file) {
          

          
          
          this.file = gViewSourceUtils.getTemporaryFile(this.data.uri, this.data.doc, 
                                                        this.data.doc.contentType);

          
          var webNavigation = this.webShell.QueryInterface(Components.interfaces.nsIWebNavigation);
          var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"]
                                   .createInstance(Components.interfaces.nsIFileOutputStream);
          foStream.init(this.file, 0x02 | 0x08 | 0x20, -1, 0); 
          var coStream = Components.classes["@mozilla.org/intl/converter-output-stream;1"]
                                   .createInstance(Components.interfaces.nsIConverterOutputStream);
          coStream.init(foStream, this.data.doc.characterSet, 0, null);

          
          coStream.writeString(webNavigation.document.body.textContent);
          
          
          coStream.close();
          foStream.close();

          let fromPrivateWindow =
            this.data.doc.defaultView
                         .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                         .getInterface(Components.interfaces.nsIWebNavigation)
                         .QueryInterface(Components.interfaces.nsILoadContext)
                         .usePrivateBrowsing;

          let helperService = Components.classes["@mozilla.org/uriloader/external-helper-app-service;1"]
                              .getService(Components.interfaces.nsPIExternalAppLauncher);
          if (fromPrivateWindow) {
            
            helperService.deleteTemporaryPrivateFileWhenPossible(this.file);
          } else {
            
            helperService.deleteTemporaryFileOnExit(this.file);
          }
        }

        var editorArgs = gViewSourceUtils.buildEditorArgs(this.file.path,
                                                          this.data.lineNumber);
        this.editor.runw(false, editorArgs, editorArgs.length);

        gViewSourceUtils.handleCallBack(this.callBack, true, this.data);
      } catch (ex) {
        
        Components.utils.reportError(ex);
        gViewSourceUtils.handleCallBack(this.callBack, false, this.data);
      } finally {
        this.destroy();
      }
    },

    onLocationChange: function() {return 0;},
    onProgressChange: function() {return 0;},
    onStatusChange: function() {return 0;},
    onSecurityChange: function() {return 0;},

    webShell: null,
    editor: null,
    callBack: null,
    data: null,
    file: null
  },

  
  getTemporaryFile: function(aURI, aDocument, aContentType) {
    
    if (!this._caUtils) {
      var scriptLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"]
                                   .getService(Components.interfaces.mozIJSSubScriptLoader);
      this._caUtils = {};
      scriptLoader.loadSubScript("chrome://global/content/contentAreaUtils.js", this._caUtils);
    }

    var fileLocator = Components.classes["@mozilla.org/file/directory_service;1"]
                                .getService(Components.interfaces.nsIProperties);
    var tempFile = fileLocator.get("TmpD", Components.interfaces.nsIFile);
    var fileName = this._caUtils.getDefaultFileName(null, aURI, aDocument, aContentType);
    var extension = this._caUtils.getDefaultExtension(fileName, aURI, aContentType);
    var leafName = this._caUtils.getNormalizedLeafName(fileName, extension);
    tempFile.append(leafName);
    return tempFile;
  }
}
