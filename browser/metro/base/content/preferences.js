




var PreferencesPanelView = {
  init: function pv_init() {
    
    Elements.prefsFlyout.addEventListener("PopupChanged", function onShow(aEvent) {
      if (aEvent.detail && aEvent.target === Elements.prefsFlyout) {
        Elements.prefsFlyout.removeEventListener("PopupChanged", onShow, false);
        SanitizeUI.init();
      }
    }, false);
  }
};
