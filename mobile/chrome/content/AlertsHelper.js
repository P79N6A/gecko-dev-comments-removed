var AlertsHelper = {
  _timeoutID: -1,
  _listener: null,
  _cookie: "",
  _clickable: false,
  get container() {
    delete this.container;
    let container = document.getElementById("alerts-container");

    
    let [leftSidebar, rightSidebar] = [Elements.tabs.getBoundingClientRect(), Elements.controls.getBoundingClientRect()];
    if (leftSidebar.left > rightSidebar.left)
      container.setAttribute("left", "0");
    else
      container.setAttribute("right", "0");

    let self = this;
    container.addEventListener("transitionend", function() {
      self.alertTransitionOver();
    }, true);

    return this.container = container;
  },

  showAlertNotification: function ah_show(aImageURL, aTitle, aText, aTextClickable, aCookie, aListener) {
    this._clickable = aTextClickable || false;
    this._listener = aListener || null;
    this._cookie = aCookie || "";

    
    let container = this.container;
    container.removeAttribute("width");
    let alertText = document.getElementById("alerts-text");
    alertText.style.whiteSpace = "";

    document.getElementById("alerts-image").setAttribute("src", aImageURL);
    document.getElementById("alerts-title").value = aTitle;
    alertText.textContent = aText;

    container.hidden = false;
    let bcr = container.getBoundingClientRect();
    if (bcr.width > window.innerWidth - 50) {
      
      container.setAttribute("width", window.innerWidth - 50); 
      alertText.style.whiteSpace = "pre-wrap"; 
      bcr = container.getBoundingClientRect(); 
    }
    container.setAttribute("width", bcr.width); 
    container.setAttribute("height", bcr.height);

#ifdef ANDROID
    let offset = (window.innerWidth - container.width) / 2;
    if (offset < 0)
      Cu.reportError("showAlertNotification called before the window is ready");
    else if (container.hasAttribute("left"))
      container.setAttribute("left", offset);
    else
      container.setAttribute("right", offset);
#endif

    container.classList.add("showing");

    let timeout = Services.prefs.getIntPref("alerts.totalOpenTime");
    let self = this;
    if (this._timeoutID)
      clearTimeout(this._timeoutID);
    this._timeoutID = setTimeout(function() { self._timeoutAlert(); }, timeout);
  },

  _timeoutAlert: function ah__timeoutAlert() {
    this._timeoutID = -1;

    this.container.classList.remove("showing");
    if (this._listener)
      this._listener.observe(null, "alertfinished", this._cookie);
  },

  alertTransitionOver: function ah_alertTransitionOver() {
    let container = this.container;
    if (!container.classList.contains("showing")) {
      container.height = 0;
      container.hidden = true;
    }
  },

  click: function ah_click(aEvent) {
    if (this._clickable && this._listener)
      this._listener.observe(null, "alertclickcallback", this._cookie);

    if (this._timeoutID != -1) {
      clearTimeout(this._timeoutID);
      this._timeoutAlert();
    }
  }
};
