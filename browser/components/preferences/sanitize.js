




let gSanitizeDialog = Object.freeze({
  onClearHistoryChanged: function () {
    let downloadsPref = document.getElementById("privacy.clearOnShutdown.downloads");
    let historyPref = document.getElementById("privacy.clearOnShutdown.history");
    downloadsPref.value = historyPref.value;
  }
});
