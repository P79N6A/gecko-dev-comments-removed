




var PreferencesPanelView = {
  init: function pv_init() {
    
    Elements.prefsFlyout.addEventListener("PopupChanged", function onShow(aEvent) {
      if (aEvent.detail && aEvent.popup === Elements.prefsFlyout) {
        Elements.prefsFlyout.removeEventListener("PopupChanged", onShow, false);
      }
    }, false);
  }
};
