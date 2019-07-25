












function DownloadProgressListener() {}

DownloadProgressListener.prototype = {
  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadProgressListener]),

  
  

  onDownloadStateChange: function dlPL_onDownloadStateChange(aState, aDownload)
  {
    
    onUpdateProgress();

    let dl;
    let state = aDownload.state;
    switch (state) {
      case nsIDM.DOWNLOAD_QUEUED:
        prependList(aDownload);
        break;

      case nsIDM.DOWNLOAD_BLOCKED_POLICY:
        prependList(aDownload);
        
        
      case nsIDM.DOWNLOAD_FAILED:
      case nsIDM.DOWNLOAD_CANCELED:
      case nsIDM.DOWNLOAD_BLOCKED_PARENTAL:
      case nsIDM.DOWNLOAD_DIRTY:
      case nsIDM.DOWNLOAD_FINISHED:
        downloadCompleted(aDownload);
        if (state == nsIDM.DOWNLOAD_FINISHED)
          autoRemoveAndClose(aDownload);
        break;
      case nsIDM.DOWNLOAD_DOWNLOADING: {
        dl = getDownload(aDownload.id);

        
        dl.setAttribute("progressmode", aDownload.percentComplete == -1 ?
                                        "undetermined" : "normal");

        
        let referrer = aDownload.referrer;
        if (referrer)
          dl.setAttribute("referrer", referrer.spec);

        break;
      }
    }

    
    try {
      if (!dl)
        dl = getDownload(aDownload.id);

      
      dl.setAttribute("state", state);

      
      updateTime(dl);
      updateStatus(dl);
      updateButtons(dl);
    } catch (e) { }
  },

  onProgressChange: function dlPL_onProgressChange(aWebProgress, aRequest,
                                                   aCurSelfProgress,
                                                   aMaxSelfProgress,
                                                   aCurTotalProgress,
                                                   aMaxTotalProgress, aDownload)
  {
    var download = getDownload(aDownload.id);

    
    if (aDownload.percentComplete != -1) {
      download.setAttribute("progress", aDownload.percentComplete);

      
      let event = document.createEvent("Events");
      event.initEvent("ValueChange", true, true);
      document.getAnonymousElementByAttribute(download, "anonid", "progressmeter")
              .dispatchEvent(event);
    }

    
    download.setAttribute("currBytes", aDownload.amountTransferred);
    download.setAttribute("maxBytes", aDownload.size);

    
    
    updateStatus(download, aDownload);

    
    onUpdateProgress();
  },

  onStateChange: function(aWebProgress, aRequest, aState, aStatus, aDownload)
  {
  },

  onSecurityChange: function(aWebProgress, aRequest, aState, aDownload)
  {
  }
};
