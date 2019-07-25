




































var TabsPopup = {
  init: function() {
    Elements.tabs.addEventListener("TabOpen", this, true);
    Elements.tabs.addEventListener("TabRemove", this, true);

    this._updateTabsCount();
  },

  get box() {
    delete this.box;
    return this.box = document.getElementById("tabs-sidebar");
  },

  get list() {
    delete this.list;
    return this.list = document.getElementById("tabs");
  },

  get button() {
    delete this.button;
    return this.button = document.getElementById("tool-tabs");
  },

  hide: function hide() {
    this.box.removeAttribute("open");
    BrowserUI.popPopup(this);
  },

  show: function show() {
    
    this.box.setAttribute("open", "true");
    this.list.resize();
    BrowserUI.pushPopup(this, [this.box, this.button]);
  },

  toggle: function toggle() {
    if (this.box.hasAttribute("open"))
      this.hide();
    else
      this.show();
  },

  _updateTabsCount: function() {
    let cmd = document.getElementById("cmd_showTabs");
    cmd.setAttribute("label", Browser.tabs.length);
  },

  handleEvent: function handleEvent(aEvent) {
    this._updateTabsCount();
  }
};
