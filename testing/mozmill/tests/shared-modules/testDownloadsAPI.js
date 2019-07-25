












































var MODULE_NAME = 'DownloadsAPI';

const gTimeout = 5000;




const downloadState = {
  notStarted      : -1,
  downloading     : 0,
  finished        : 1,
  failed          : 2,
  canceled        : 3,
  paused          : 4,
  queued          : 5,
  blockedParental : 6,
  scanning        : 7,
  dirty           : 8,
  blockedPolicy   : 9
}




function downloadManager()
{
  this._controller = null;
  this._dms = Cc["@mozilla.org/download-manager;1"]
                 .getService(Ci.nsIDownloadManager);

  this.downloadState = downloadState;
}




downloadManager.prototype = {
  





  get controller() {
    return this._controller;
  },

  





  get activeDownloadCount() {
    return this._dms.activeDownloadCount;
  },

  


  cancelActiveDownloads : function downloadManager_cancelActiveDownloads() {
    
    var downloads = this._dms.activeDownloads;
    
    
    while (downloads.hasMoreElements()) {
      var download = downloads.getNext().QueryInterface(Ci.nsIDownload);
      this._dms.cancelDownload(download.id);
    }
  },

  


  cleanUp : function downloadManager_cleanUp()
  {
    this._dms.cleanUp();
  },

  



  cleanAll : function downloadManager_cleanAll()
  {
    
    this.cancelActiveDownloads();

    
    var downloads = this.getAllDownloads();
    this.deleteDownloadedFiles(downloads);

    
    this.cleanUp();
  },

  





  close : function downloadManager_close(force)
  {
    var windowCount = mozmill.utils.getWindows().length;

    if (this._controller) {
      
      if (force) {
        this._controller.window.close();
      } else {
        this._controller.keypress(null, 'w', {accelKey: true});
      }

      this._controller.waitForEval("subject.getWindows().length == " + (windowCount - 1),
                                   gTimeout, 100, mozmill.utils);
      this._controller = null;
    }
  },

  





  deleteDownloadedFiles : function downloadManager_deleteDownloadedFiles(downloads)
  {
    downloads.forEach(function(download) {
      try {
        var file = getLocalFileFromNativePathOrUrl(download.target);
        file.remove(false);
      } catch (ex) {
      }
    });
  },

  





  getAllDownloads : function downloadManager_getAllDownloads()
  {
    var dbConn = this._dms.DBConnection;
    var stmt = null;

    if (dbConn.schemaVersion < 3)
      return new Array();

    
    var downloads = [];
    stmt = dbConn.createStatement("SELECT * FROM moz_downloads");
    while (stmt.executeStep()) {
      downloads.push({
        id: stmt.row.id, name: stmt.row.name, target: stmt.row.target,
        tempPath: stmt.row.tempPath, startTime: stmt.row.startTime,
        endTime: stmt.row.endTime, state: stmt.row.state,
        referrer: stmt.row.referrer, entityID: stmt.row.entityID,
        currBytes: stmt.row.currBytes, maxBytes: stmt.row.maxBytes,
        mimeType : stmt.row.mimeType, autoResume: stmt.row.autoResume,
        preferredApplication: stmt.row.preferredApplication,
        preferredAction: stmt.row.preferredAction
      });
    };
    stmt.reset();

    return downloads;
  },

  





  getDownloadState : function downloadManager_getDownloadState(download) {
    return download.getNode().getAttribute('state');
  },

  










  getElement : function downloadManager_getElement(spec) {
    var elem = null;

    switch(spec.type) {
      



      case "download":
        
        var download = new elementslib.Lookup(this._controller.window.document,
                                              '/id("downloadManager")/id("downloadView")/' +
                                              '{"' + spec.subtype + '":"' + spec.value + '"}');
        this._controller.waitForElement(download, gTimeout);

        
        elem = new elementslib.Lookup(this._controller.window.document,
                                      '/id("downloadManager")/id("downloadView")/' +
                                      'id("' + download.getNode().getAttribute('id') + '")');
        break;

      



      case "download_button":
        
        this._controller.sleep(0);

        elem = new elementslib.Lookup(this._controller.window.document, spec.value.expression +
                                      '/anon({"flex":"1"})/[1]/[1]/{"cmd":"cmd_' + spec.subtype + '"}');
        break;
      default:
        throw new Error(arguments.callee.name + ": Unknown element type - " + spec.type);
    }

    return elem;
  },

  







  open : function downloadManager_open(controller, shortcut) {
    if (shortcut) {
      
      if (mozmill.isLinux)
        controller.keypress(null, "y", {ctrlKey: true, shiftKey: true});
      else
        controller.keypress(null, "j", {accelKey: true});
    } else {
      controller.click(new elementslib.Elem(controller.menus["tools-menu"].menu_openDownloads));
    }

    controller.sleep(500);
    this.waitForOpened(controller);
  },

  









  waitForDownloadState : function downloadManager_waitForDownloadState(download, state, timeout) {
    this._controller.waitForEval("subject.manager.getDownloadState(subject.download) == subject.state", timeout, 100,
                                 {manager: this, download: download, state: state});
  },

  





  waitForOpened : function downloadManager_waitForOpened(controller) {
    controller.waitForEval("subject.getMostRecentWindow('Download:Manager') != null",
                           gTimeout, 100, mozmill.wm);

    var window = mozmill.wm.getMostRecentWindow('Download:Manager');
    this._controller = new mozmill.controller.MozMillController(window);
  }
};










var downloadFileOfUnknownType = function(controller, url)
{
  controller.open(url);

  
  controller.waitForEval("subject.getMostRecentWindow('').document.documentElement.id == 'unknownContentType'",
                         gTimeout, 100, mozmill.wm);

  var window = mozmill.wm.getMostRecentWindow('');
  var utController = new mozmill.controller.MozMillController(window);
  utController.sleep(500);

  
  var saveFile = new elementslib.ID(utController.window.document, "save");
  utController.waitThenClick(saveFile, gTimeout);
  utController.waitForEval("subject.selected == true", gTimeout, 100,
                         saveFile.getNode());

  
  var button = new elementslib.Lookup(utController.window.document,
                                      '/id("unknownContentType")/anon({"anonid":"buttons"})/{"dlgtype":"accept"}');
  utController.waitForElement(button, gTimeout);
  utController.waitForEval("subject.okButton.hasAttribute('disabled') == false", gTimeout, 100,
                           {okButton: button.getNode()});
  utController.click(button);

  utController.waitForEval("subject.getMostRecentWindow('') != this.window", gTimeout, 100,
                           mozmill.wm);
}








function getLocalFileFromNativePathOrUrl(aPathOrUrl)
{
  if (aPathOrUrl.substring(0,7) == "file://") {
    
    let ioSvc = Cc["@mozilla.org/network/io-service;1"]
                   .getService(Ci.nsIIOService);

    
    const fileUrl = ioSvc.newURI(aPathOrUrl, null, null)
                         .QueryInterface(Ci.nsIFileURL);
    return fileUrl.file.clone().QueryInterface(Ci.nsILocalFile);
  } else {
    
    var f = new nsLocalFile(aPathOrUrl);
    return f;
  }
}
