




var PreferencesPanelView = {
  init: function pv_init() {
    
    Elements.prefsFlyout.addEventListener("PopupChanged", function onShow(aEvent) {
      if (aEvent.detail && aEvent.target === Elements.prefsFlyout) {
        Elements.prefsFlyout.removeEventListener("PopupChanged", onShow, false);
        SanitizeUI.init();
      }
    }, false);
  },
  onDNTPreferenceChanged: function onDNTPreferenceChanged() {
    let dntNoPref = document.getElementById("prefs-dnt-nopref");

    
    Services.prefs.setBoolPref("privacy.donottrackheader.enabled", !dntNoPref.selected);
  }
};
