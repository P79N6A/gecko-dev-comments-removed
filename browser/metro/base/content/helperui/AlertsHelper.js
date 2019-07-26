



var AlertsHelper = {
  _listener: null,
  _cookie: "",

  showAlertNotification: function ah_show(aImageURL, aTitle, aText, aTextClickable, aCookie, aListener) {
    Services.obs.addObserver(this, "metro_native_toast_clicked", false);
    this._listener = aListener;
    this._cookie = aCookie;

    Services.metro.showNativeToast(aTitle, aText, aImageURL);
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "metro_native_toast_clicked":
        Services.obs.removeObserver(this, "metro_native_toast_clicked");
        this._listener.observe(null, "alertclickcallback", this._cookie);
        break;
    }
  }
};
