var BookmarkPopup = {
  get box() {
    delete this.box;
    this.box = document.getElementById("bookmark-popup");

    let [tabsSidebar, controlsSidebar] = [Elements.tabs.getBoundingClientRect(), Elements.controls.getBoundingClientRect()];
    this.box.setAttribute(tabsSidebar.left < controlsSidebar.left ? "right" : "left", controlsSidebar.width - this.box.offset);
    this.box.top = BrowserUI.starButton.getBoundingClientRect().top - this.box.offset;

    
    let self = this;
    messageManager.addMessageListener("pagehide", function(aMessage) {
      self.hide();
    });

    return this.box;
  },

  hide : function hide() {
    this.box.hidden = true;
    BrowserUI.popPopup(this);
  },

  show : function show() {
    this.box.hidden = false;
    this.box.anchorTo(BrowserUI.starButton);

    
    BrowserUI.pushPopup(this, [this.box, BrowserUI.starButton]);
  },

  toggle : function toggle() {
    if (this.box.hidden)
      this.show();
    else
      this.hide();
  }
};
