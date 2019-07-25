
























































function isUsableDirectory(aDirectory)
{
  return aDirectory.exists() && aDirectory.isDirectory() &&
         aDirectory.isWritable();
}



function nsUnkownContentTypeDialogProgressListener(aHelperAppDialog) {
  this.helperAppDlg = aHelperAppDialog;
}

nsUnkownContentTypeDialogProgressListener.prototype = {
  
  
  onStatusChange: function( aWebProgress, aRequest, aStatus, aMessage ) {
    if ( aStatus != Components.results.NS_OK ) {
      
      var prompter = Components.classes[ "@mozilla.org/embedcomp/prompt-service;1" ]
                               .getService( Components.interfaces.nsIPromptService );
      
      
      prompter.alert( this.dialog, this.helperAppDlg.mTitle, aMessage );
      
      this.helperAppDlg.onCancel();
      if ( this.helperAppDlg.mDialog ) {
        this.helperAppDlg.mDialog.close();
      }
    }
  },

  
  onProgressChange: function( aWebProgress,
                              aRequest,
                              aCurSelfProgress,
                              aMaxSelfProgress,
                              aCurTotalProgress,
                              aMaxTotalProgress ) {
  },

  onProgressChange64: function( aWebProgress,
                                aRequest,
                                aCurSelfProgress,
                                aMaxSelfProgress,
                                aCurTotalProgress,
                                aMaxTotalProgress ) {
  },



  onStateChange: function( aWebProgress, aRequest, aStateFlags, aStatus ) {
  },

  onLocationChange: function( aWebProgress, aRequest, aLocation ) {
  },

  onSecurityChange: function( aWebProgress, aRequest, state ) {
  },

  onRefreshAttempted: function( aWebProgress, aURI, aDelay, aSameURI ) {
    return true;
  }
};















const PREF_BD_USEDOWNLOADDIR = "browser.download.useDownloadDir";
const nsITimer = Components.interfaces.nsITimer;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/DownloadLastDir.jsm");
Components.utils.import("resource://gre/modules/DownloadPaths.jsm");



function nsUnknownContentTypeDialog() {
  
  this.mLauncher = null;
  this.mContext  = null;
  this.mSourcePath = null;
  this.chosenApp = null;
  this.givenDefaultApp = false;
  this.updateSelf = true;
  this.mTitle    = "";
}

nsUnknownContentTypeDialog.prototype = {
  classID: Components.ID("{F68578EB-6EC2-4169-AE19-8C6243F0ABE1}"),

  nsIMIMEInfo  : Components.interfaces.nsIMIMEInfo,

  QueryInterface: function (iid) {
    if (!iid.equals(Components.interfaces.nsIHelperAppLauncherDialog) &&
        !iid.equals(Components.interfaces.nsITimerCallback) &&
        !iid.equals(Components.interfaces.nsISupports)) {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },

  

  
  
  
  show: function(aLauncher, aContext, aReason)  {
    this.mLauncher = aLauncher;
    this.mContext  = aContext;

    const nsITimer = Components.interfaces.nsITimer;
    this._showTimer = Components.classes["@mozilla.org/timer;1"]
                                .createInstance(nsITimer);
    this._showTimer.initWithCallback(this, 0, nsITimer.TYPE_ONE_SHOT);
  },

  
  
  
  
  reallyShow: function() {
    try {
      var ir = this.mContext.QueryInterface(Components.interfaces.nsIInterfaceRequestor);
      var dwi = ir.getInterface(Components.interfaces.nsIDOMWindowInternal);
      var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                         .getService(Components.interfaces.nsIWindowWatcher);
      this.mDialog = ww.openWindow(dwi,
                                   "chrome://mozapps/content/downloads/unknownContentType.xul",
                                   null,
                                   "chrome,centerscreen,titlebar,dialog=yes,dependent",
                                   null);
    } catch (ex) {
      
      
      const NS_BINDING_ABORTED = 0x804b0002;
      this.mLauncher.cancel(NS_BINDING_ABORTED);
      return;
    }

    
    this.mDialog.dialog = this;

    
    this.getSpecialFolderKey = this.mDialog.getSpecialFolderKey;

    
    var progressListener = new nsUnkownContentTypeDialogProgressListener(this);
    this.mLauncher.setWebProgressListener(progressListener);
  },

  
  
  
  
  
  
  
  
  
  
  
  promptForSaveToFile: function(aLauncher, aContext, aDefaultFile, aSuggestedFileExtension, aForcePrompt) {
    var result = null;

    this.mLauncher = aLauncher;

    let prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch);
    let bundle = Components.classes["@mozilla.org/intl/stringbundle;1"].
                            getService(Components.interfaces.nsIStringBundleService).
                            createBundle("chrome://mozapps/locale/downloads/unknownContentType.properties");

    if (!aForcePrompt) {
      
      
      let autodownload = false;
      try {
        autodownload = prefs.getBoolPref(PREF_BD_USEDOWNLOADDIR);
      } catch (e) { }

      if (autodownload) {
        
        let dnldMgr = Components.classes["@mozilla.org/download-manager;1"]
                                .getService(Components.interfaces.nsIDownloadManager);
        let defaultFolder = dnldMgr.userDownloadsDirectory;

        try {
          result = this.validateLeafName(defaultFolder, aDefaultFile, aSuggestedFileExtension);
        }
        catch (ex) {
          if (ex.result == Components.results.NS_ERROR_FILE_ACCESS_DENIED) {
            let prompter = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].
                                      getService(Components.interfaces.nsIPromptService);

            
            prompter.alert(this.dialog,
                           bundle.GetStringFromName("badPermissions.title"),
                           bundle.GetStringFromName("badPermissions"));

            return;
          }
        }

        
        if (result)
          return result;
      }
    }

    
    var nsIFilePicker = Components.interfaces.nsIFilePicker;
    var picker = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
    var windowTitle = bundle.GetStringFromName("saveDialogTitle");
    var parent = aContext.QueryInterface(Components.interfaces.nsIInterfaceRequestor).getInterface(Components.interfaces.nsIDOMWindowInternal);
    picker.init(parent, windowTitle, nsIFilePicker.modeSave);
    picker.defaultString = aDefaultFile;

    if (aSuggestedFileExtension) {
      
      picker.defaultExtension = aSuggestedFileExtension.substring(1);
    }
    else {
      try {
        picker.defaultExtension = this.mLauncher.MIMEInfo.primaryExtension;
      }
      catch (ex) { }
    }

    var wildCardExtension = "*";
    if (aSuggestedFileExtension) {
      wildCardExtension += aSuggestedFileExtension;
      picker.appendFilter(this.mLauncher.MIMEInfo.description, wildCardExtension);
    }

    picker.appendFilters( nsIFilePicker.filterAll );

    
    
    
    var dnldMgr = Components.classes["@mozilla.org/download-manager;1"]
                            .getService(Components.interfaces.nsIDownloadManager);
    picker.displayDirectory = dnldMgr.userDownloadsDirectory;

    var relatedURI = null;
    if (aContext.document)
      relatedURI = aContext.document.documentURIObject;

    
    try {
      var lastDir = gDownloadLastDir.getFile(relatedURI);
      if (isUsableDirectory(lastDir))
        picker.displayDirectory = lastDir;
    }
    catch (ex) {
    }

    if (picker.show() == nsIFilePicker.returnCancel) {
      
      return null;
    }

    
    
    
    result = picker.file;

    if (result) {
      try {
        
        
        
        if (result.exists())
          result.remove(false);
      }
      catch (e) { }
      var newDir = result.parent.QueryInterface(Components.interfaces.nsILocalFile);

      
      gDownloadLastDir.setFile(relatedURI, newDir);

      result = this.validateLeafName(newDir, result.leafName, null);
    }
    return result;
  },

  















  validateLeafName: function (aLocalFile, aLeafName, aFileExt)
  {
    if (!(aLocalFile && isUsableDirectory(aLocalFile)))
      return null;

    
    
    aLeafName = aLeafName.replace(/^\.+/, "");

    if (aLeafName == "")
      aLeafName = "unnamed" + (aFileExt ? "." + aFileExt : "");
    aLocalFile.append(aLeafName);

    var createdFile = DownloadPaths.createNiceUniqueFile(aLocalFile);

#ifdef XP_WIN
    let ext;
    try {
      
      ext = "." + this.mLauncher.MIMEInfo.primaryExtension;
    } catch (e) { }

    
    
    let leaf = createdFile.leafName;
    if (ext && leaf.slice(-ext.length) != ext && createdFile.isExecutable()) {
      createdFile.remove(false);
      aLocalFile.leafName = leaf + ext;
      createdFile = DownloadPaths.createNiceUniqueFile(aLocalFile);
    }
#endif

    return createdFile;
  },

  

  
  initDialog : function() {
    
    var suggestedFileName = this.mLauncher.suggestedFileName;

    
    var url   = this.mLauncher.source;
    var fname = "";
    var iconPath = "goat";
    this.mSourcePath = url.prePath;
    if (url instanceof Components.interfaces.nsIURL) {
      
      fname = iconPath = url.fileName;
      this.mSourcePath += url.directory;
    } else {
      
      fname = url.path;
      this.mSourcePath += url.path;
    }

    if (suggestedFileName)
      fname = iconPath = suggestedFileName;

    var displayName = fname.replace(/ +/g, " ");

    this.mTitle = this.dialogElement("strings").getFormattedString("title", [displayName]);
    this.mDialog.document.title = this.mTitle;

    
    this.initIntro(url, fname, displayName);

    var iconString = "moz-icon://" + iconPath + "?size=16&contentType=" + this.mLauncher.MIMEInfo.MIMEType;
    this.dialogElement("contentTypeImage").setAttribute("src", iconString);

    
    
    var mimeType = this.mLauncher.MIMEInfo.MIMEType;
    var shouldntRememberChoice = (mimeType == "application/octet-stream" ||
                                  mimeType == "application/x-msdownload" ||
                                  this.mLauncher.targetFileIsExecutable);
    if (shouldntRememberChoice && !this.openWithDefaultOK()) {
      
      this.dialogElement("normalBox").collapsed = true;
      
      this.dialogElement("basicBox").collapsed = false;
      
      
      let acceptButton = this.mDialog.document.documentElement
                                              .getButton("accept");
      acceptButton.label = this.dialogElement("strings")
                               .getString("unknownAccept.label");
      acceptButton.setAttribute("icon", "save");
      this.mDialog.document.documentElement.getButton("cancel").label = this.dialogElement("strings").getString("unknownCancel.label");
      
      this.dialogElement("openHandler").collapsed = true;
      
      this.dialogElement("mode").selectedItem = this.dialogElement("save");
    }
    else {
      this.initAppAndSaveToDiskValues();

      
      
      
      
      var rememberChoice = this.dialogElement("rememberChoice");

#if 0
      
      
      
      
      
      
      
      
      
      
      
      

      
      
      
      

      
#endif
      if (shouldntRememberChoice) {
        rememberChoice.checked = false;
        rememberChoice.disabled = true;
      }
      else {
        rememberChoice.checked = !this.mLauncher.MIMEInfo.alwaysAskBeforeHandling;
      }
      this.toggleRememberChoice(rememberChoice);

      
      var openHandler = this.dialogElement("openHandler");
      openHandler.parentNode.removeChild(openHandler);
      var openHandlerBox = this.dialogElement("openHandlerBox");
      openHandlerBox.appendChild(openHandler);
    }

    this.mDialog.setTimeout("dialog.postShowCallback()", 0);

    this.mDialog.document.documentElement.getButton("accept").disabled = true;
    this._showTimer = Components.classes["@mozilla.org/timer;1"]
                                .createInstance(nsITimer);
    this._showTimer.initWithCallback(this, 250, nsITimer.TYPE_ONE_SHOT);
  },

  notify: function (aTimer) {
    if (aTimer == this._showTimer) {
      if (!this.mDialog) {
        this.reallyShow();
      } else {
        
        try {
          if (!this._blurred) {
            this.mDialog.document.documentElement.getButton("accept").disabled = false;
          }
        } catch (ex) {}
        this._delayExpired = true;
      }
      
      this._showTimer = null;
    }
    else if (aTimer == this._saveToDiskTimer) {
      
      
      this.mLauncher.saveToDisk(null, false);
      this._saveToDiskTimer = null;
    }
  },

  postShowCallback: function () {
    this.mDialog.sizeToContent();

    
    this.dialogElement("mode").focus();
  },

  
  initIntro: function(url, filename, displayname) {
    this.dialogElement( "location" ).value = displayname;
    this.dialogElement( "location" ).setAttribute("realname", filename);
    this.dialogElement( "location" ).setAttribute("tooltiptext", displayname);

    
    
    var pathString = this.mSourcePath;
    try
    {
      var fileURL = url.QueryInterface(Components.interfaces.nsIFileURL);
      if (fileURL)
      {
        var fileObject = fileURL.file;
        if (fileObject)
        {
          var parentObject = fileObject.parent;
          if (parentObject)
          {
            pathString = parentObject.path;
          }
        }
      }
    } catch(ex) {}

    if (pathString == this.mSourcePath)
    {
      
      var tmpurl = url.clone(); 
      try {
        tmpurl.userPass = "";
      } catch (ex) {}
      pathString = tmpurl.prePath;
    }

    
    var location = this.dialogElement( "source" );
    location.value = pathString;
    location.setAttribute("tooltiptext", this.mSourcePath);

    
    var type = this.dialogElement("type");
    var mimeInfo = this.mLauncher.MIMEInfo;

    
    var typeString = mimeInfo.description;

    if (typeString == "") {
      
      var primaryExtension = "";
      try {
        primaryExtension = mimeInfo.primaryExtension;
      }
      catch (ex) {
      }
      if (primaryExtension != "")
        typeString = this.dialogElement("strings").getFormattedString("fileType", [primaryExtension.toUpperCase()]);
      
      else
        typeString = mimeInfo.MIMEType;
    }

    type.value = typeString;
  },

  _blurred: false,
  _delayExpired: false,
  onBlur: function(aEvent) {
    this._blurred = true;
    this.mDialog.document.documentElement.getButton("accept").disabled = true;
  },

  onFocus: function(aEvent) {
    this._blurred = false;
    if (this._delayExpired) {
      var script = "document.documentElement.getButton('accept').disabled = false";
      this.mDialog.setTimeout(script, 250);
    }
  },

  
  openWithDefaultOK: function() {
    
#ifdef XP_WIN
    
    
    
    
    
    

    
    return !this.mLauncher.targetFileIsExecutable;
#else
    
    
    
    return this.mLauncher.MIMEInfo.hasDefaultHandler;
#endif
  },

  
  initDefaultApp: function() {
    
    var desc = this.mLauncher.MIMEInfo.defaultDescription;
    if (desc) {
      var defaultApp = this.dialogElement("strings").getFormattedString("defaultApp", [desc]);
      this.dialogElement("defaultHandler").label = defaultApp;
    }
    else {
      this.dialogElement("modeDeck").setAttribute("selectedIndex", "1");
      
      
      this.dialogElement("defaultHandler").hidden = true;
    }
  },

  
  getPath: function (aFile) {
#ifdef XP_MACOSX
    return aFile.leafName || aFile.path;
#else
    return aFile.path;
#endif
  },

  
  initAppAndSaveToDiskValues: function() {
    var modeGroup = this.dialogElement("mode");

    
    
    var openWithDefaultOK = this.openWithDefaultOK();
    var mimeType = this.mLauncher.MIMEInfo.MIMEType;
    if (this.mLauncher.targetFileIsExecutable || (
      (mimeType == "application/octet-stream" ||
       mimeType == "application/x-msdownload") &&
        !openWithDefaultOK)) {
      this.dialogElement("open").disabled = true;
      var openHandler = this.dialogElement("openHandler");
      openHandler.disabled = true;
      openHandler.selectedItem = null;
      modeGroup.selectedItem = this.dialogElement("save");
      return;
    }

    
    try {
      this.chosenApp =
        this.mLauncher.MIMEInfo.preferredApplicationHandler
                               .QueryInterface(Components.interfaces.nsILocalHandlerApp);
    } catch (e) {
      this.chosenApp = null;
    }
    
    this.initDefaultApp();

    var otherHandler = this.dialogElement("otherHandler");

    
    if (this.chosenApp && this.chosenApp.executable &&
        this.chosenApp.executable.path) {
      otherHandler.setAttribute("path",
                                this.getPath(this.chosenApp.executable));

      otherHandler.label = this.getFileDisplayName(this.chosenApp.executable);
      otherHandler.hidden = false;
    }

    var useDefault = this.dialogElement("useSystemDefault");
    var openHandler = this.dialogElement("openHandler");
    openHandler.selectedIndex = 0;

    if (this.mLauncher.MIMEInfo.preferredAction == this.nsIMIMEInfo.useSystemDefault) {
      
      modeGroup.selectedItem = this.dialogElement("open");
    } else if (this.mLauncher.MIMEInfo.preferredAction == this.nsIMIMEInfo.useHelperApp) {
      
      modeGroup.selectedItem = this.dialogElement("open");
      openHandler.selectedIndex = 1;
    } else {
      
      modeGroup.selectedItem = this.dialogElement("save");
    }

    
    if (!openWithDefaultOK) {
      var useDefault = this.dialogElement("defaultHandler");
      var isSelected = useDefault.selected;

      
      useDefault.hidden = true;
      
      if (isSelected) {
        openHandler.selectedIndex = 1;
        modeGroup.selectedItem = this.dialogElement("save");
      }
    }

    otherHandler.nextSibling.hidden = otherHandler.nextSibling.nextSibling.hidden = false;
    this.updateOKButton();
  },

  
  helperAppChoice: function() {
    return this.chosenApp;
  },

  get saveToDisk() {
    return this.dialogElement("save").selected;
  },

  get useOtherHandler() {
    return this.dialogElement("open").selected && this.dialogElement("openHandler").selectedIndex == 1;
  },

  get useSystemDefault() {
    return this.dialogElement("open").selected && this.dialogElement("openHandler").selectedIndex == 0;
  },

  toggleRememberChoice: function (aCheckbox) {
    this.dialogElement("settingsChange").hidden = !aCheckbox.checked;
    this.mDialog.sizeToContent();
  },

  openHandlerCommand: function () {
    var openHandler = this.dialogElement("openHandler");
    if (openHandler.selectedItem.id == "choose")
      this.chooseApp();
    else
      openHandler.setAttribute("lastSelectedItemID", openHandler.selectedItem.id);
  },

  updateOKButton: function() {
    var ok = false;
    if (this.dialogElement("save").selected) {
      
      ok = true;
    }
    else if (this.dialogElement("open").selected) {
      switch (this.dialogElement("openHandler").selectedIndex) {
      case 0:
        
        ok = true;
        break;
      case 1:
        
        
        ok = this.chosenApp || /\S/.test(this.dialogElement("otherHandler").getAttribute("path")); 
        break;
      }
    }

    
    this.mDialog.document.documentElement.getButton("accept").disabled = !ok;
  },

  
  appChanged: function() {
    return this.helperAppChoice() != this.mLauncher.MIMEInfo.preferredApplicationHandler;
  },

  updateMIMEInfo: function() {
    var needUpdate = false;
    
    
    if (this.saveToDisk) {
      needUpdate = this.mLauncher.MIMEInfo.preferredAction != this.nsIMIMEInfo.saveToDisk;
      if (needUpdate)
        this.mLauncher.MIMEInfo.preferredAction = this.nsIMIMEInfo.saveToDisk;
    }
    else if (this.useSystemDefault) {
      needUpdate = this.mLauncher.MIMEInfo.preferredAction != this.nsIMIMEInfo.useSystemDefault;
      if (needUpdate)
        this.mLauncher.MIMEInfo.preferredAction = this.nsIMIMEInfo.useSystemDefault;
    }
    else {
      
      
      needUpdate = this.mLauncher.MIMEInfo.preferredAction != this.nsIMIMEInfo.useHelperApp || this.appChanged();
      if (needUpdate) {
        this.mLauncher.MIMEInfo.preferredAction = this.nsIMIMEInfo.useHelperApp;
        
        var app = this.helperAppChoice();
        this.mLauncher.MIMEInfo.preferredApplicationHandler = app;
      }
    }
    
    needUpdate = needUpdate || this.mLauncher.MIMEInfo.alwaysAskBeforeHandling != (!this.dialogElement("rememberChoice").checked);

    
    
    
    
    
    
    needUpdate = needUpdate || !this.mLauncher.MIMEInfo.alwaysAskBeforeHandling;

    
    this.mLauncher.MIMEInfo.alwaysAskBeforeHandling = !this.dialogElement("rememberChoice").checked;

    return needUpdate;
  },

  
  
  updateHelperAppPref: function() {
    var ha = new this.mDialog.HelperApps();
    ha.updateTypeInfo(this.mLauncher.MIMEInfo);
    ha.destroy();
  },

  
  onOK: function() {
    
    if (this.useOtherHandler) {
      var helperApp = this.helperAppChoice();
      if (!helperApp || !helperApp.executable ||
          !helperApp.executable.exists()) {
        
        var bundle = this.dialogElement("strings");
        var msg = bundle.getFormattedString("badApp", [this.dialogElement("otherHandler").getAttribute("path")]);
        var svc = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
        svc.alert(this.mDialog, bundle.getString("badApp.title"), msg);

        
        this.mDialog.document.documentElement.getButton("accept").disabled = true;
        this.dialogElement("mode").focus();

        
        this.chosenApp = null;

        
        return false;
      }
    }

    
    
    this.mLauncher.setWebProgressListener(null);

    
    
    
    
    try {
      var needUpdate = this.updateMIMEInfo();

      if (this.dialogElement("save").selected) {
        
        
        

#if 0
        var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
        var targetFile = null;
        try {
          targetFile = prefs.getComplexValue("browser.download.defaultFolder",
                                             Components.interfaces.nsILocalFile);
          var leafName = this.dialogElement("location").getAttribute("realname");
          
          targetFile = this.validateLeafName(targetFile, leafName, null);
        }
        catch(e) { }

        this.mLauncher.saveToDisk(targetFile, false);
#endif

        
        
        this._saveToDiskTimer = Components.classes["@mozilla.org/timer;1"]
                                          .createInstance(nsITimer);
        this._saveToDiskTimer.initWithCallback(this, 0,
                                               nsITimer.TYPE_ONE_SHOT);
      }
      else
        this.mLauncher.launchWithApplication(null, false);

      
      
      
      
      
      if (needUpdate && this.mLauncher.MIMEInfo.MIMEType != "application/octet-stream")
        this.updateHelperAppPref();
    } catch(e) { }

    
    this.mDialog.dialog = null;

    
    return true;
  },

  
  onCancel: function() {
    
    this.mLauncher.setWebProgressListener(null);

    
    try {
      const NS_BINDING_ABORTED = 0x804b0002;
      this.mLauncher.cancel(NS_BINDING_ABORTED);
    } catch(exception) {
    }

    
    this.mDialog.dialog = null;

    
    return true;
  },

  
  dialogElement: function(id) {
    return this.mDialog.document.getElementById(id);
  },

  
  getFileDisplayName: function getFileDisplayName(file)
  {
#ifdef XP_WIN
    if (file instanceof Components.interfaces.nsILocalFileWin) {
      try {
        return file.getVersionInfoField("FileDescription");
      } catch (e) {}
    }
#endif
#ifdef XP_MACOSX
    if (file instanceof Components.interfaces.nsILocalFileMac) {
      try {
        return file.bundleDisplayName;
      } catch (e) {}
    }
#endif
    return file.leafName;
  },

  
  chooseApp: function() {
#ifdef XP_WIN
    
    var fileExtension = "";
    try {
      fileExtension = this.mLauncher.MIMEInfo.primaryExtension;
    } catch(ex) {
    }

    
    var typeString = this.mLauncher.MIMEInfo.description;

    if (!typeString) {
      
      
      if (fileExtension) {
        typeString =
          this.dialogElement("strings").
          getFormattedString("fileType", [fileExtension.toUpperCase()]);
      } else {
        
        typeString = this.mLauncher.MIMEInfo.MIMEType;
      }
    }

    var params = {};
    params.title =
      this.dialogElement("strings").getString("chooseAppFilePickerTitle");
    params.description = typeString;
    params.filename    = this.mLauncher.suggestedFileName;
    params.mimeInfo    = this.mLauncher.MIMEInfo;
    params.handlerApp  = null;

    this.mDialog.openDialog("chrome://global/content/appPicker.xul", null,
                            "chrome,modal,centerscreen,titlebar,dialog=yes",
                            params);

    if (params.handlerApp &&
        params.handlerApp.executable &&
        params.handlerApp.executable.isFile()) {
      
      this.chosenApp = params.handlerApp;

#else
    var nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);
    fp.init(this.mDialog,
            this.dialogElement("strings").getString("chooseAppFilePickerTitle"),
            nsIFilePicker.modeOpen);

    fp.appendFilters(nsIFilePicker.filterApps);

    if (fp.show() == nsIFilePicker.returnOK && fp.file) {
      
      var localHandlerApp =
        Components.classes["@mozilla.org/uriloader/local-handler-app;1"].
                   createInstance(Components.interfaces.nsILocalHandlerApp);
      localHandlerApp.executable = fp.file;
      this.chosenApp = localHandlerApp;
#endif

      
      
      this.dialogElement("modeDeck").setAttribute("selectedIndex", "0");

      
      var otherHandler = this.dialogElement("otherHandler");
      otherHandler.removeAttribute("hidden");
      otherHandler.setAttribute("path", this.getPath(this.chosenApp.executable));
      otherHandler.label = this.getFileDisplayName(this.chosenApp.executable);
      this.dialogElement("openHandler").selectedIndex = 1;
      this.dialogElement("openHandler").setAttribute("lastSelectedItemID", "otherHandler");

      this.dialogElement("mode").selectedItem = this.dialogElement("open");
    }
    else {
      var openHandler = this.dialogElement("openHandler");
      var lastSelectedID = openHandler.getAttribute("lastSelectedItemID");
      if (!lastSelectedID)
        lastSelectedID = "defaultHandler";
      openHandler.selectedItem = this.dialogElement(lastSelectedID);
    }
  },

  
  debug: false,

  
  dump: function( text ) {
    if ( this.debug ) {
      dump( text );
    }
  },

  
  dumpObj: function( spec ) {
    var val = "<undefined>";
    try {
      val = eval( "this."+spec ).toString();
    } catch( exception ) {
    }
    this.dump( spec + "=" + val + "\n" );
  },

  
  dumpObjectProperties: function( desc, obj ) {
    for( prop in obj ) {
      this.dump( desc + "." + prop + "=" );
      var val = "<undefined>";
      try {
        val = obj[ prop ];
      } catch ( exception ) {
      }
      this.dump( val + "\n" );
    }
  }
}

var NSGetFactory = XPCOMUtils.generateNSGetFactory([nsUnknownContentTypeDialog]);
