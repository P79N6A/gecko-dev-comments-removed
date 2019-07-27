











"use strict";

let { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadUtils",
                                  "resource://gre/modules/DownloadUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");













function DownloadElementShell() {}

DownloadElementShell.prototype = {
  


  element: null,

  


  dataItem: null,

  



  get download() this.dataItem.download,

  


  get image() {
    if (this.download.target.path) {
      return "moz-icon://" + this.download.target.path + "?size=32";
    }

    
    return "moz-icon://.unknown?size=32";
  },

  




  get displayName() {
    if (!this.download.target.path) {
      return this.download.source.url;
    }
    return OS.Path.basename(this.download.target.path);
  },

  



  get _progressElement() {
    if (!this.__progressElement) {
      
      this.__progressElement = document.getAnonymousElementByAttribute(
                               this.element, "anonid", "progressmeter");
    }
    return this.__progressElement;
  },

  




  _updateState() {
    this.element.setAttribute("state", this.dataItem.state);
    this.element.setAttribute("displayName", this.displayName);
    this.element.setAttribute("image", this.image);

    
    this.lastEstimatedSecondsLeft = Infinity;

    this._updateProgress();
  },

  



  _updateProgress() {
    if (this.dataItem.starting) {
      
      this.element.setAttribute("progressmode", "normal");
      this.element.setAttribute("progress", "0");
    } else if (this.dataItem.state == Ci.nsIDownloadManager.DOWNLOAD_SCANNING ||
               this.dataItem.percentComplete == -1) {
      
      
      this.element.setAttribute("progressmode", "undetermined");
    } else {
      
      this.element.setAttribute("progressmode", "normal");
      this.element.setAttribute("progress", this.dataItem.percentComplete);
    }

    
    if (this._progressElement) {
      let event = document.createEvent("Events");
      event.initEvent("ValueChange", true, true);
      this._progressElement.dispatchEvent(event);
    }

    let status = this.statusTextAndTip;
    this.element.setAttribute("status", status.text);
    this.element.setAttribute("statusTip", status.tip);
  },

  lastEstimatedSecondsLeft: Infinity,

  




  get statusTextAndTip() this.rawStatusTextAndTip,

  


  get rawStatusTextAndTip() {
    const nsIDM = Ci.nsIDownloadManager;
    let s = DownloadsCommon.strings;

    let text = "";
    let tip = "";

    if (this.dataItem.paused) {
      let transfer = DownloadUtils.getTransferTotal(this.download.currentBytes,
                                                    this.dataItem.maxBytes);

      
      
      text = s.statusSeparatorBeforeNumber(s.statePaused, transfer);
    } else if (this.dataItem.state == nsIDM.DOWNLOAD_DOWNLOADING) {
      
      
      
      [text] = DownloadUtils.getDownloadStatusNoRate(
                                          this.download.currentBytes,
                                          this.dataItem.maxBytes,
                                          this.download.speed,
                                          this.lastEstimatedSecondsLeft);
      let newEstimatedSecondsLeft;
      [tip, newEstimatedSecondsLeft] = DownloadUtils.getDownloadStatus(
                                          this.download.currentBytes,
                                          this.dataItem.maxBytes,
                                          this.download.speed,
                                          this.lastEstimatedSecondsLeft);
      this.lastEstimatedSecondsLeft = newEstimatedSecondsLeft;
    } else if (this.dataItem.starting) {
      text = s.stateStarting;
    } else if (this.dataItem.state == nsIDM.DOWNLOAD_SCANNING) {
      text = s.stateScanning;
    } else {
      let stateLabel;
      switch (this.dataItem.state) {
        case nsIDM.DOWNLOAD_FAILED:
          stateLabel = s.stateFailed;
          break;
        case nsIDM.DOWNLOAD_CANCELED:
          stateLabel = s.stateCanceled;
          break;
        case nsIDM.DOWNLOAD_BLOCKED_PARENTAL:
          stateLabel = s.stateBlockedParentalControls;
          break;
        case nsIDM.DOWNLOAD_BLOCKED_POLICY:
          stateLabel = s.stateBlockedPolicy;
          break;
        case nsIDM.DOWNLOAD_DIRTY:
          stateLabel = s.stateDirty;
          break;
        case nsIDM.DOWNLOAD_FINISHED:
          
          if (this.dataItem.maxBytes !== undefined &&
              this.dataItem.maxBytes >= 0) {
            let [size, unit] =
                DownloadUtils.convertByteUnits(this.dataItem.maxBytes);
            stateLabel = s.sizeWithUnits(size, unit);
            break;
          }
          
        default:
          stateLabel = s.sizeUnknown;
          break;
      }

      let referrer = this.download.source.referrer ||
                     this.download.source.url;
      let [displayHost, fullHost] = DownloadUtils.getURIHost(referrer);

      let date = new Date(this.dataItem.endTime);
      let [displayDate, fullDate] = DownloadUtils.getReadableDates(date);

      let firstPart = s.statusSeparator(stateLabel, displayHost);
      text = s.statusSeparator(firstPart, displayDate);
      tip = s.statusSeparator(fullHost, fullDate);
    }

    return { text, tip: tip || text };
  },
};
