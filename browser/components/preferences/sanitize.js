




let gSanitizeDialog = Object.freeze({
  init: function() {
    let customWidthElements = document.getElementsByAttribute("dialogWidth", "*");
    let isInSubdialog = document.documentElement.hasAttribute("subdialog");
    for (let element of customWidthElements) {
      element.style.width = element.getAttribute(isInSubdialog ? "subdialogWidth" : "dialogWidth");
    }
    onClearHistoryChanged();
  },

  onClearHistoryChanged: function () {
    let downloadsPref = document.getElementById("privacy.clearOnShutdown.downloads");
    let historyPref = document.getElementById("privacy.clearOnShutdown.history");
    downloadsPref.value = historyPref.value;
  }
});
