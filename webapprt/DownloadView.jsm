



"use strict";

this.EXPORTED_SYMBOLS = ["DownloadView"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Downloads.jsm");

this.DownloadView = {
  init: function() {
    Downloads.getList(Downloads.ALL)
             .then(list => list.addView(this))
             .catch(Cu.reportError);
  },

  onDownloadAdded: function(aDownload) {
    let dmWindow = Services.wm.getMostRecentWindow("Download:Manager");
    if (dmWindow) {
      dmWindow.focus();
    } else {
      Services.ww.openWindow(null,
                             "chrome://webapprt/content/downloads/downloads.xul",
                             "Download:Manager",
                             "chrome,dialog=no,resizable",
                             null);
    }
  },
};

DownloadView.init();
