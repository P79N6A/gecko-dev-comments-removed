




XPCOMUtils.defineLazyModuleGetter(this, "DownloadsCommon",
                                  "resource:///modules/DownloadsCommon.jsm");

let gSanitizeDialog = Object.freeze({
  


  init: function ()
  {
    let downloadsPref = document.getElementById("privacy.clearOnShutdown.downloads");
    downloadsPref.disabled = !DownloadsCommon.useToolkitUI;
    this.onClearHistoryChanged();
  },

  onClearHistoryChanged: function () {
    if (DownloadsCommon.useToolkitUI)
      return;
    let downloadsPref = document.getElementById("privacy.clearOnShutdown.downloads");
    let historyPref = document.getElementById("privacy.clearOnShutdown.history");
    downloadsPref.value = historyPref.value;
  }
});
