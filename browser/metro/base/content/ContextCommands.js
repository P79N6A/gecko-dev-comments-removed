




 



var ContextCommands = {
  _picker: null,

  get clipboard() {
    return Cc["@mozilla.org/widget/clipboardhelper;1"]
             .getService(Ci.nsIClipboardHelper);
  },

  get docRef() {
    return Browser.selectedBrowser.contentWindow.document;
  },

  



  

  copy: function cc_copy() {
    let target = ContextMenuUI.popupState.target;
    if (target.localName == "browser") {
      
      if (ContextMenuUI.popupState.string != "undefined") {
        this.clipboard.copyString(ContextMenuUI.popupState.string,
                                  this.docRef);
        this.showToast(Strings.browser.GetStringFromName("selectionHelper.textCopied"));
        SelectionHelperUI.closeEditSessionAndClear();
      }
    } else {
      
      target.editor.copy();
      this.showToast(Strings.browser.GetStringFromName("selectionHelper.textCopied"));
    }

    if (target)
      target.focus();
  },

  paste: function cc_paste() {
    let target = ContextMenuUI.popupState.target;
    if (target.localName == "browser") {
      
      let x = ContextMenuUI.popupState.x;
      let y = ContextMenuUI.popupState.y;
      let json = {x: x, y: y, command: "paste" };
      target.messageManager.sendAsyncMessage("Browser:ContextCommand", json);
      SelectionHelperUI.closeEditSessionAndClear();
    } else {
      
      target.editor.paste(Ci.nsIClipboard.kGlobalClipboard);
      target.focus();
    }
  },

  pasteAndGo: function cc_pasteAndGo() {
    let target = ContextMenuUI.popupState.target;
    target.editor.selectAll();
    target.editor.paste(Ci.nsIClipboard.kGlobalClipboard);
    BrowserUI.goToURI();
  },

  select: function cc_select() {
    let contextInfo = { name: "",
                        json: ContextMenuUI.popupState,
                        target: ContextMenuUI.popupState.target };
    SelectionHelperUI.openEditSession(contextInfo);
  },

  selectAll: function cc_selectAll() {
    let target = ContextMenuUI.popupState.target;
    if (target.localName == "browser") {
      
      let x = ContextMenuUI.popupState.x;
      let y = ContextMenuUI.popupState.y;
      let json = {x: x, y: y, command: "select-all" };
      target.messageManager.sendAsyncMessage("Browser:ContextCommand", json);
      let contextInfo = { name: "",
                          json: ContextMenuUI.popupState,
                          target: ContextMenuUI.popupState.target };
      SelectionHelperUI.attachEditSession(contextInfo);
    } else {
      
      target.editor.selectAll();
      target.focus();
    }
  },

  
  searchTextSetup: function cc_searchTextSetup(aRichListItem, aSearchString) {
    let defaultURI;
    let defaultName;
    try {
      let defaultPB = Services.prefs.getDefaultBranch(null);
      const nsIPLS = Ci.nsIPrefLocalizedString;
      defaultName = defaultPB.getComplexValue("browser.search.defaultenginename", nsIPLS).data;
      let defaultEngine = Services.search.getEngineByName(defaultName);
      defaultURI = defaultEngine.getSubmission(aSearchString).uri.spec;
    } catch (ex) {
      Cu.reportError(ex);
      return false;
    }
    
    let label = Services.strings
                        .createBundle("chrome://browser/locale/browser.properties")
                        .formatStringFromName("browser.search.contextTextSearchLabel",
                                              [defaultName], 1);
    aRichListItem.childNodes[0].setAttribute("value", label);
    aRichListItem.setAttribute("searchString", defaultURI);
    return true;
  },

  searchText: function cc_searchText(aRichListItem) {
    let defaultURI = aRichListItem.getAttribute("searchString");
    aRichListItem.childNodes[0].setAttribute("value", "");
    aRichListItem.setAttribute("searchString", "");
    BrowserUI.newTab(defaultURI, Browser.selectedTab);
  },

  

  openLinkInNewTab: function cc_openLinkInNewTab() {
    BrowserUI.newTab(ContextMenuUI.popupState.linkURL, Browser.selectedTab);
  },

  copyLink: function cc_copyLink() {
    this.clipboard.copyString(ContextMenuUI.popupState.linkURL,
                              this.docRef);
    this.showToast(Strings.browser.GetStringFromName("selectionHelper.linkCopied"));
  },

  bookmarkLink: function cc_bookmarkLink() {
    let state = ContextMenuUI.popupState;
    let uri = Util.makeURI(state.linkURL);
    let title = state.linkTitle || state.linkURL;

    try {
      Bookmarks.addForURI(uri, title);
    } catch (e) {
      return;
    }

    this.showToast(Strings.browser.GetStringFromName("alertLinkBookmarked"));
  },

  

  saveImageToLib: function cc_saveImageToLib() {
    this.saveToWinLibrary("Pict");
  },

  copyImage: function cc_copyImage() {
    
    this.sendCommand("copy-image-contents");
  },

  copyImageSrc: function cc_copyImageSrc() {
    this.clipboard.copyString(ContextMenuUI.popupState.mediaURL,
                              this.docRef);
    this.showToast(Strings.browser.GetStringFromName("selectionHelper.linkCopied"));
  },

  openImageInNewTab: function cc_openImageInNewTab() {
    BrowserUI.newTab(ContextMenuUI.popupState.mediaURL, Browser.selectedTab);
  },

  

  saveVideoToLib: function cc_saveVideoToLib() {
    this.saveToWinLibrary("Vids");
  },

  copyVideoSrc: function cc_copyVideoSrc() {
    this.clipboard.copyString(ContextMenuUI.popupState.mediaURL,
                              this.docRef);
    this.showToast(Strings.browser.GetStringFromName("selectionHelper.linkCopied"));
  },

  openVideoInNewTab: function cc_openVideoInNewTab() {
    BrowserUI.newTab(ContextMenuUI.popupState.mediaURL, Browser.selectedTab);
  },

  openVideoInFullscreen: function cc_openVideoInFullscreen() {
    
    this.sendCommand('videotab');
  },

  

  editBookmark: function cc_editBookmark() {
    let target = ContextMenuUI.popupState.target;
    target.startEditing();
  },

  removeBookmark: function cc_removeBookmark() {
    let target = ContextMenuUI.popupState.target;
    target.remove();
  },

  

  findInPage: function cc_findInPage() {
    FindHelperUI.show();
  },

  viewOnDesktop: function cc_viewOnDesktop() {
    Appbar.onViewOnDesktop();
  },

  



  saveToWinLibrary: function cc_saveToWinLibrary(aType) {
    let popupState = ContextMenuUI.popupState;
    let browser = popupState.target;

    
    
    
    

    let dirSvc = Components.classes["@mozilla.org/file/directory_service;1"]
                           .getService(Components.interfaces.nsIProperties);
    let saveLocationPath = dirSvc.get(aType, Components.interfaces.nsIFile);

    let fileInfo = new ContentAreaUtils.FileInfo();
    ContentAreaUtils.initFileInfo(fileInfo, popupState.mediaURL,
                                  browser.documentURI.originCharset,
                                  null, null, null);
    let filename =
      ContentAreaUtils.getNormalizedLeafName(fileInfo.fileName, fileInfo.fileExt);
    saveLocationPath.append(filename);

    
    ContentAreaUtils.internalPersist({
      sourceURI         : fileInfo.uri,
      sourceDocument    : null,
      sourceReferrer    : browser.documentURI,
      targetContentType : popupState.contentType,
      targetFile        : saveLocationPath,
      sourceCacheKey    : null,
      sourcePostData    : null,
      bypassCache       : false,
      initiatingWindow  : this.docRef.defaultView
    });
  },

  showToast: function showToast(aString) {
    let toaster = Cc["@mozilla.org/toaster-alerts-service;1"].getService(Ci.nsIAlertsService);
    toaster.showAlertNotification(null, aString, "", false, "", null);
  },

  sendCommand: function cc_playVideo(aCommand) {
    
    let browser = ContextMenuUI.popupState.target;
    browser.messageManager.sendAsyncMessage("Browser:ContextCommand", { command: aCommand });
  },

  




  isAccessibleDirectory: function isAccessibleDirectory(aDirectory) {
    return aDirectory && aDirectory.exists() && aDirectory.isDirectory() &&
           aDirectory.isWritable();
  },

  







  saveFileAs: function saveFileAs(aPopupState) {
    let srcUri = null;
    let mediaURL = aPopupState.mediaURL;
    try {
      srcUri = Util.makeURI(mediaURL, null, null);
    } catch (ex) {
      Util.dumpLn("could not parse:", mediaURL);
      return;
    }

    let picker = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    let windowTitle = Strings.browser.GetStringFromName("browserForSaveLocation");

    picker.init(window, windowTitle, Ci.nsIFilePicker.modeSave);

    
    let fileName = mediaURL.substring(mediaURL.lastIndexOf("/") + 1);
    if (fileName.length)
      picker.defaultString = fileName;

    
    let fileExtension = mediaURL.substring(mediaURL.lastIndexOf(".") + 1);
    if (fileExtension.length)
      picker.defaultExtension = fileExtension;
    picker.appendFilters(Ci.nsIFilePicker.filterImages);

    
    var dnldMgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
    picker.displayDirectory = dnldMgr.userDownloadsDirectory;
    try {
      let lastDir = Services.prefs.getComplexValue("browser.download.lastDir", Ci.nsILocalFile);
      if (this.isAccessibleDirectory(lastDir))
        picker.displayDirectory = lastDir;
    }
    catch (e) { }

    this._picker = picker;
    this._pickerUrl = mediaURL;
    this._pickerContentDisp = aPopupState.contentDisposition;
    this._contentType = aPopupState.contentType;
    picker.open(ContextCommands);
  },

  


  done: function done(aSuccess) {
    let picker = this._picker;
    this._picker = null;

    if (aSuccess == Ci.nsIFilePicker.returnCancel)
      return;

    let file = picker.file;
    if (file == null)
      return;

    try {
      if (file.exists())
        file.remove(false);
    } catch (e) {}

    ContentAreaUtils.internalSave(this._pickerUrl, null, null,
                                  this._pickerContentDisp,
                                  this._contentType, false, "SaveImageTitle",
                                  new AutoChosen(file, Util.makeURI(this._pickerUrl, null, null)),
                                  getBrowser().documentURI, this.docRef, true, null);

    var newDir = file.parent.QueryInterface(Ci.nsILocalFile);
    Services.prefs.setComplexValue("browser.download.lastDir", Ci.nsILocalFile, newDir);
  },

};

function AutoChosen(aFileAutoChosen, aUriAutoChosen) {
  this.file = aFileAutoChosen;
  this.uri  = aUriAutoChosen;
}

