




































var DownloadMonitor = {
  _dlmgr : null,
  _panel : null,
  _activeStr : null,
  _pausedStr : null,
  _lastTime : Infinity,
  _listening : false,

  init : function DM_init() {
    
    Cu.import("resource://gre/modules/DownloadUtils.jsm");
    Cu.import("resource://gre/modules/PluralForm.jsm");

    
    this._dlmgr = Cc["@mozilla.org/download-manager;1"].getService(Ci.nsIDownloadManager);
    this._panel = document.getElementById("download-monitor");
    this._status = document.getElementById("download-monitor-status");
    this._time = document.getElementById("download-monitor-time");

    
    





    this._activeStr = "1 active download (#2);#1 active downloads (#2)";
    this._pausedStr = "1 paused download;#1 paused downloads";

    this._dlmgr.addListener(this);
    this._listening = true;

    this.updateStatus();
  },

  uninit: function DM_uninit() {
    if (this._listening)
      this._dlmgr.removeListener(this);
  },

  


  updateStatus: function DM_updateStatus() {
    let numActive = this._dlmgr.activeDownloadCount;

    
    if (numActive == 0) {
      this._panel.hidePopup();
      this._lastTime = Infinity;
      return;
    }

    
    let numPaused = 0;
    let maxTime = -Infinity;
    let dls = this._dlmgr.activeDownloads;
    while (dls.hasMoreElements()) {
      let dl = dls.getNext().QueryInterface(Ci.nsIDownload);
      if (dl.state == this._dlmgr.DOWNLOAD_DOWNLOADING) {
        
        if (dl.speed > 0 && dl.size > 0)
          maxTime = Math.max(maxTime, (dl.size - dl.amountTransferred) / dl.speed);
        else
          maxTime = -1;
      }
      else if (dl.state == this._dlmgr.DOWNLOAD_PAUSED)
        numPaused++;
    }

    
    let timeLeft;
    [timeLeft, this._lastTime] = DownloadUtils.getTimeLeft(maxTime, this._lastTime);

    
    let numDls = numActive - numPaused;
    let status = this._activeStr;

    
    if (numDls == 0) {
      numDls = numPaused;
      status = this._pausedStr;
    }

    
    
    status = PluralForm.get(numDls, status);
    status = status.replace("#1", numDls);

    
    this._status.value = status;
    this._time.value = timeLeft;
    this._panel.hidden = false;
    this._panel.openPopup(null, "", 60, 50, false, false);
  },

  onProgressChange: function() {
    this.updateStatus();
  },

  onDownloadStateChange: function() {
    this.updateStatus();
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus, aDownload) {
  },

  onSecurityChange: function(aWebProgress, aRequest, aState, aDownload) {
  },

  QueryInterface : function(aIID) {
    if (aIID.equals(Components.interfaces.nsIDownloadProgressListener) ||
        aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};
