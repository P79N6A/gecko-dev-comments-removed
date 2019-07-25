




































var TabsPopup = {
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
  }
};
