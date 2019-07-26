



var AlertsHelper = {
  _listener: null,

  showAlertNotification: function ah_show(aImageURL, aTitle, aText, aTextClickable, aCookie, aListener) {
    if (aListener) {
      Services.obs.addObserver(this, "metro_native_toast_clicked", false);
      Services.obs.addObserver(this, "metro_native_toast_dismissed", false);
      Services.obs.addObserver(this, "metro_native_toast_shown", false);
    }
    this._listener = aListener;

    Services.metro.showNativeToast(aTitle, aText, aImageURL, aCookie);
  },

  closeAlert: function ah_close() {
    if (this._listener) {
      Services.obs.removeObserver(this, "metro_native_toast_shown");
      Services.obs.removeObserver(this, "metro_native_toast_clicked");
      Services.obs.removeObserver(this, "metro_native_toast_dismissed");
      this._listener = null;
    }
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "metro_native_toast_clicked":
        this._listener.observe(null, "alertclickcallback", aData);
        break;
      case "metro_native_toast_shown":
        this._listener.observe(null, "alertshow", aData);
        break;
      case "metro_native_toast_dismissed":
        this._listener.observe(null, "alertfinished", aData);
        break;
    }
  }
};
