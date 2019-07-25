





































let EXPORTED_SYMBOLS = [
  "DownloadTaskbarProgress",
];




const Cc = Components.classes;
const Ci = Components.interfaces;

const kTaskbarID = "@mozilla.org/windows-taskbar;1";




const DownloadTaskbarProgress =
{
  







  onBrowserWindowLoad: function DTP_onBrowserWindowLoad(aWindow)
  {
    if (!DownloadTaskbarProgressUpdater) {
      return;
    }
    if (!DownloadTaskbarProgressUpdater._activeTaskbarProgress) {
      DownloadTaskbarProgressUpdater._setActiveWindow(aWindow, false);
    }
  },

  



  onDownloadWindowLoad: function DTP_onDownloadWindowLoad(aWindow)
  {
    if (!DownloadTaskbarProgressUpdater) {
      return;
    }
    DownloadTaskbarProgressUpdater._setActiveWindow(aWindow, true);
  },

  



  get activeTaskbarProgress() {
    if (!DownloadTaskbarProgressUpdater) {
      return null;
    }
    return DownloadTaskbarProgressUpdater._activeTaskbarProgress;
  },

  get activeWindowIsDownloadWindow() {
    if (!DownloadTaskbarProgressUpdater) {
      return null;
    }
    return DownloadTaskbarProgressUpdater._activeWindowIsDownloadWindow;
  },

  get taskbarState() {
    if (!DownloadTaskbarProgressUpdater) {
      return null;
    }
    return DownloadTaskbarProgressUpdater._taskbarState;
  },

};




var DownloadTaskbarProgressUpdater =
{
  
  _taskbar: null,

  
  _dm: null,

  


  _init: function DTPU_init()
  {
    if (!(kTaskbarID in Cc)) {
      
      DownloadTaskbarProgressUpdater = null;
      return;
    }

    this._taskbar = Cc[kTaskbarID].getService(Ci.nsIWinTaskbar);
    if (!this._taskbar.available) {
      
      DownloadTaskbarProgressUpdater = null;
      return;
    }

    this._dm = Cc["@mozilla.org/download-manager;1"].
               getService(Ci.nsIDownloadManager);
    this._dm.addListener(this);

    this._os = Cc["@mozilla.org/observer-service;1"].
               getService(Ci.nsIObserverService);
    this._os.addObserver(this, "quit-application-granted", false);

    this._updateStatus();
    
    
  },

  


  _uninit: function DTPU_uninit() {
    this._dm.removeListener(this);
    this._os.removeObserver(this, "quit-application-granted");
  },

  




  _activeTaskbarProgress: null,

  
  _activeWindowIsDownloadWindow: false,

  









  _setActiveWindow: function DTPU_setActiveWindow(aWindow, aIsDownloadWindow)
  {
    
    
    this._clearTaskbar();

    this._activeWindowIsDownloadWindow = aIsDownloadWindow;
    if (aWindow) {
      
      let docShell = aWindow.QueryInterface(Ci.nsIInterfaceRequestor).
                       getInterface(Ci.nsIWebNavigation).
                       QueryInterface(Ci.nsIDocShellTreeItem).treeOwner.
                       QueryInterface(Ci.nsIInterfaceRequestor).
                       getInterface(Ci.nsIXULWindow).docShell;
      let taskbarProgress = this._taskbar.getTaskbarProgress(docShell);
      this._activeTaskbarProgress = taskbarProgress;

      this._updateTaskbar();
      
      
      aWindow.addEventListener("unload", function () {
        DownloadTaskbarProgressUpdater._onActiveWindowUnload(taskbarProgress);
      }, false);
    }
    else {
      this._activeTaskbarProgress = null;
    }
  },

  
  _taskbarState: Ci.nsITaskbarProgress.STATE_NO_PROGRESS,
  _totalSize: 0,
  _totalTransferred: 0,

  








  _updateTaskbar: function DTPU_updateTaskbar()
  {
    if (!this._activeTaskbarProgress) {
      return;
    }

    
    
    if (this._activeWindowIsDownloadWindow ||
        (this._taskbarState == Ci.nsITaskbarProgress.STATE_NORMAL)) {
      this._activeTaskbarProgress.setProgressState(this._taskbarState,
                                                   this._totalTransferred,
                                                   this._totalSize);
    }
    
    else {
      this._clearTaskbar();
    }
  },

  





  _clearTaskbar: function DTPU_clearTaskbar()
  {
    if (this._activeTaskbarProgress) {
      this._activeTaskbarProgress.setProgressState(
        Ci.nsITaskbarProgress.STATE_NO_PROGRESS
      );
    }
  },

  















  _updateStatus: function DTPU_updateStatus()
  {
    let numActive = this._dm.activeDownloadCount;
    let totalSize = 0, totalTransferred = 0;

    if (numActive == 0) {
      this._taskbarState = Ci.nsITaskbarProgress.STATE_NO_PROGRESS;
    }
    else {
      let numPaused = 0, numScanning = 0;

      
      let downloads = this._dm.activeDownloads;
      while (downloads.hasMoreElements()) {
        let download = downloads.getNext().QueryInterface(Ci.nsIDownload);
        
        if (download.percentComplete != -1) {
          totalSize += download.size;
          totalTransferred += download.amountTransferred;
        }
        
        if (download.state == this._dm.DOWNLOAD_PAUSED) {
          numPaused++;
        } else if (download.state == this._dm.DOWNLOAD_SCANNING) {
          numScanning++;
        }
      }

      
      
      
      if (numActive == numPaused) {
        if (totalSize == 0) {
          this._taskbarState = Ci.nsITaskbarProgress.STATE_NO_PROGRESS;
          totalTransferred = 0;
        }
        else {
          this._taskbarState = Ci.nsITaskbarProgress.STATE_PAUSED;
        }
      }
      
      
      else if (totalSize == 0 || numActive == numScanning) {
        this._taskbarState = Ci.nsITaskbarProgress.STATE_INDETERMINATE;
        totalSize = 0;
        totalTransferred = 0;
      }
      
      else {
        this._taskbarState = Ci.nsITaskbarProgress.STATE_NORMAL;
      }
    }

    this._totalSize = totalSize;
    this._totalTransferred = totalTransferred;
  },

  










  _onActiveWindowUnload: function DTPU_onActiveWindowUnload(aTaskbarProgress)
  {
    if (this._activeTaskbarProgress == aTaskbarProgress) {
      let windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                           getService(Ci.nsIWindowMediator);
      let windows = windowMediator.getEnumerator(null);
      let newActiveWindow = null;
      if (windows.hasMoreElements()) {
        newActiveWindow = windows.getNext().QueryInterface(Ci.nsIDOMWindow);
      }

      
      
      this._setActiveWindow(newActiveWindow, false);
    }
  },

  
  

  


  onProgressChange: function DTPU_onProgressChange()
  {
    this._updateStatus();
    this._updateTaskbar();
  },

  


  onDownloadStateChange: function DTPU_onDownloadStateChange()
  {
    this._updateStatus();
    this._updateTaskbar();
  },

  onSecurityChange: function() { },

  onStateChange: function() { },

  observe: function DTPU_observe(aSubject, aTopic, aData) {
    if (aTopic == "quit-application-granted") {
      this._uninit();
    }
  }
};




DownloadTaskbarProgressUpdater._init();
