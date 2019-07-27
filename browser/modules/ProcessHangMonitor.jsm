




"use strict";

let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

this.EXPORTED_SYMBOLS = ["ProcessHangMonitor"];

Cu.import("resource://gre/modules/Services.jsm");











const HANG_EXPIRATION_TIME = 10000;

let ProcessHangMonitor = {
  





  _activeReports: new Map(),

  


  init: function() {
    Services.obs.addObserver(this, "process-hang-report", false);
    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.ww.registerNotification(this);
  },

  



  terminateScript: function(win) {
    this.handleUserInput(win, report => report.terminateScript());
  },

  



  debugScript: function(win) {
    this.handleUserInput(win, report => {
      function callback() {
        report.endStartingDebugger();
      }

      report.beginStartingDebugger();

      let svc = Cc["@mozilla.org/dom/slow-script-debug;1"].getService(Ci.nsISlowScriptDebug);
      let handler = svc.remoteActivationHandler;
      handler.handleSlowScriptDebug(report.scriptBrowser, callback);
    });
  },

  



  terminatePlugin: function(win) {
    this.handleUserInput(win, report => report.terminatePlugin());
  },

  



  terminateProcess: function(win) {
    this.handleUserInput(win, report => report.terminateProcess());
  },

  




  refreshMenu: function(win) {
    let report = this.findReport(win.gBrowser.selectedBrowser);
    if (!report) {
      return;
    }

    function setVisible(id, visible) {
      let item = win.document.getElementById(id);
      item.hidden = !visible;
    }

    if (report.hangType == report.SLOW_SCRIPT) {
      setVisible("processHangTerminateScript", true);
      setVisible("processHangDebugScript", true);
      setVisible("processHangTerminatePlugin", false);
    } else if (report.hangType == report.PLUGIN_HANG) {
      setVisible("processHangTerminateScript", false);
      setVisible("processHangDebugScript", false);
      setVisible("processHangTerminatePlugin", true);
    }
  },

  




  handleUserInput: function(win, func) {
    let report = this.findReport(win.gBrowser.selectedBrowser);
    if (!report) {
      return;
    }
    this.removeReport(report);

    return func(report);
  },

  observe: function(subject, topic, data) {
    switch (topic) {
      case "xpcom-shutdown":
        Services.obs.removeObserver(this, "xpcom-shutdown");
        Services.obs.removeObserver(this, "process-hang-report");
        Services.ww.unregisterNotification(this);
        break;

      case "process-hang-report":
        this.reportHang(subject.QueryInterface(Ci.nsIHangReport));
        break;

      case "domwindowopened":
        
        
        let win = subject.QueryInterface(Ci.nsIDOMWindow);
        let listener = (ev) => {
          win.removeEventListener("load", listener, true);
          this.updateWindows();
        };
        win.addEventListener("load", listener, true);
        break;
    }
  },

  


  findReport: function(browser) {
    let frameLoader = browser.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader;
    for (let [report, timer] of this._activeReports) {
      if (report.isReportForBrowser(frameLoader)) {
        return report;
      }
    }
    return null;
  },

  





  updateWindows: function() {
    let e = Services.wm.getEnumerator("navigator:browser");
    while (e.hasMoreElements()) {
      let win = e.getNext();

      this.updateWindow(win);

      
      if (this._activeReports.size) {
        this.trackWindow(win);
      } else {
        this.untrackWindow(win);
      }
    }
  },

  


  updateWindow: function(win) {
    let report = this.findReport(win.gBrowser.selectedBrowser);

    if (report) {
      this.showNotification(win, report);
    } else {
      this.hideNotification(win);
    }
  },

  


  showNotification: function(win, report) {
    let nb = win.document.getElementById("high-priority-global-notificationbox");
    let notification = nb.getNotificationWithValue("process-hang");
    if (notification) {
      return;
    }

    let bundle = win.gNavigatorBundle;
    let brandBundle = win.document.getElementById("bundle_brand");
    let appName = brandBundle.getString("brandShortName");
    let message = bundle.getFormattedString(
      "processHang.message",
      [appName]);

    let buttons = [{
      label: bundle.getString("processHang.button.label"),
      accessKey: bundle.getString("processHang.button.accessKey"),
      popup: "processHangOptions",
      callback: null,
    }];

    nb.appendNotification(message, "process-hang",
                          "chrome://browser/content/aboutRobots-icon.png",
                          nb.PRIORITY_WARNING_HIGH, buttons);
  },

  


  hideNotification: function(win) {
    let nb = win.document.getElementById("high-priority-global-notificationbox");
    let notification = nb.getNotificationWithValue("process-hang");
    if (notification) {
      nb.removeNotification(notification);
    }
  },

  



  trackWindow: function(win) {
    win.gBrowser.tabContainer.addEventListener("TabSelect", this, true);
    win.gBrowser.tabContainer.addEventListener("TabRemotenessChange", this, true);
  },

  untrackWindow: function(win) {
    win.gBrowser.tabContainer.removeEventListener("TabSelect", this, true);
    win.gBrowser.tabContainer.removeEventListener("TabRemotenessChange", this, true);
  },

  handleEvent: function(event) {
    let win = event.target.ownerDocument.defaultView;

    
    

    if (event.type == "TabSelect" || event.type == "TabRemotenessChange") {
      this.updateWindow(win);
    }
  },

  



  reportHang: function(report) {
    
    if (this._activeReports.has(report)) {
      let timer = this._activeReports.get(report);
      timer.cancel();
      timer.initWithCallback(this, HANG_EXPIRATION_TIME, timer.TYPE_ONE_SHOT);
      return;
    }

    
    
    if (report.hangType == report.SLOW_SCRIPT) {
      
      Services.telemetry.getHistogramById("SLOW_SCRIPT_NOTICE_COUNT").add();
    } else if (report.hangType == report.PLUGIN_HANG) {
      
      
      Services.telemetry.getHistogramById("PLUGIN_HANG_NOTICE_COUNT").add();
    }

    
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, HANG_EXPIRATION_TIME, timer.TYPE_ONE_SHOT);

    this._activeReports.set(report, timer);
    this.updateWindows();
  },

  



  removeReport: function(report) {
    this._activeReports.delete(report);
    this.updateWindows();
  },

  


  notify: function(timer) {
    for (let [otherReport, otherTimer] of this._activeReports) {
      if (otherTimer === timer) {
        this.removeReport(otherReport);
        break;
      }
    }
  },
};
