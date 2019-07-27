











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

  


  get image() {
    if (!this.download.target.path) {
      
      return "moz-icon://.unknown?size=32";
    }

    
    
    
    
    
    
    return "moz-icon://" + this.download.target.path + "?size=32" +
           (this.download.succeeded ? "&state=normal" : "");
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
    this.element.setAttribute("displayName", this.displayName);
    this.element.setAttribute("image", this.image);
    this.element.setAttribute("state",
                              DownloadsCommon.stateOfDownload(this.download));

    
    this.lastEstimatedSecondsLeft = Infinity;

    this._updateProgress();
  },

  



  _updateProgress() {
    if (this.download.succeeded) {
      
      if (this.download.target.exists) {
        this.element.setAttribute("exists", "true");
      } else {
        this.element.removeAttribute("exists");
      }
    }

    
    if (this.download.hasProgress) {
      this.element.setAttribute("progressmode", "normal");
      this.element.setAttribute("progress", this.download.progress);
    } else {
      this.element.setAttribute("progressmode", "undetermined");
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

    if (!this.download.stopped) {
      let total = this.download.hasProgress ? this.download.totalBytes : -1;
      
      
      
      [text] = DownloadUtils.getDownloadStatusNoRate(
                                          this.download.currentBytes,
                                          total,
                                          this.download.speed,
                                          this.lastEstimatedSecondsLeft);
      let newEstimatedSecondsLeft;
      [tip, newEstimatedSecondsLeft] = DownloadUtils.getDownloadStatus(
                                          this.download.currentBytes,
                                          total,
                                          this.download.speed,
                                          this.lastEstimatedSecondsLeft);
      this.lastEstimatedSecondsLeft = newEstimatedSecondsLeft;
    } else if (this.download.canceled && this.download.hasPartialData) {
      let total = this.download.hasProgress ? this.download.totalBytes : -1;
      let transfer = DownloadUtils.getTransferTotal(this.download.currentBytes,
                                                    total);

      
      
      text = s.statusSeparatorBeforeNumber(s.statePaused, transfer);
    } else if (!this.download.succeeded && !this.download.canceled &&
               !this.download.error) {
      text = s.stateStarting;
    } else {
      let stateLabel;

      if (this.download.succeeded) {
        
        if (this.download.target.size !== undefined) {
          let [size, unit] = DownloadUtils.convertByteUnits(
                                                  this.download.target.size);
          stateLabel = s.sizeWithUnits(size, unit);
        } else {
          
          stateLabel = s.sizeUnknown;
        }
      } else if (this.download.canceled) {
        stateLabel = s.stateCanceled;
      } else if (this.download.error.becauseBlockedByParentalControls) {
        stateLabel = s.stateBlockedParentalControls;
      } else if (this.download.error.becauseBlockedByReputationCheck) {
        stateLabel = s.stateDirty;
      } else {
        stateLabel = s.stateFailed;
      }

      let referrer = this.download.source.referrer || this.download.source.url;
      let [displayHost, fullHost] = DownloadUtils.getURIHost(referrer);

      let date = new Date(this.download.endTime);
      let [displayDate, fullDate] = DownloadUtils.getReadableDates(date);

      let firstPart = s.statusSeparator(stateLabel, displayHost);
      text = s.statusSeparator(firstPart, displayDate);
      tip = s.statusSeparator(fullHost, fullDate);
    }

    return { text, tip: tip || text };
  },
};
