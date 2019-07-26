




var FindHelperUI = {
  type: "find",
  commands: {
    next: "cmd_findNext",
    previous: "cmd_findPrevious",
    close: "cmd_findClose"
  },

  _open: false,
  _status: null,

  



  get isActive() {
    return this._open;
  },

  get status() {
    return this._status;
  },

  set status(val) {
    if (val != this._status) {
      this._status = val;
      if (!val)
        this._textbox.removeAttribute("status");
      else
        this._textbox.setAttribute("status", val);
      this.updateCommands(this._textbox.value);
    }
  },

  init: function findHelperInit() {
    this._textbox = document.getElementById("find-helper-textbox");
    this._container = Elements.contentNavigator;

    this._cmdPrevious = document.getElementById(this.commands.previous);
    this._cmdNext = document.getElementById(this.commands.next);

    this._textbox.addEventListener('keydown', this);

    
    messageManager.addMessageListener("FindAssist:Show", this);
    messageManager.addMessageListener("FindAssist:Hide", this);

    
    Elements.tabList.addEventListener("TabSelect", this, true);
    Elements.browsers.addEventListener("URLChanged", this, true);
    window.addEventListener("MozContextUIShow", this, true);
    window.addEventListener("MozContextUIExpand", this, true);
  },

  receiveMessage: function findHelperReceiveMessage(aMessage) {
    let json = aMessage.json;
    switch(aMessage.name) {
      case "FindAssist:Show":
        ContextUI.dismiss();
        this.status = json.result;
        if (json.rect)
          this._zoom(Rect.fromRect(json.rect));
        break;

      case "FindAssist:Hide":
        if (this._container.getAttribute("type") == this.type)
          this.hide();
        break;
    }
  },

  handleEvent: function findHelperHandleEvent(aEvent) {
    switch (aEvent.type) {
      case "MozContextUIShow":
      case "MozContextUIExpand":
      case "TabSelect":
        this.hide();
        break;

      case "URLChanged":
        if (aEvent.detail && aEvent.target == getBrowser())
          this.hide();
        break;

      case "keydown":
        if (aEvent.keyCode == Ci.nsIDOMKeyEvent.DOM_VK_RETURN) {
	  if (aEvent.shiftKey) {
	    this.goToPrevious();
	  } else {
	    this.goToNext();
	  }
        }
    }
  },

  show: function findHelperShow() {
    if (this._open)
      return;

    
    ContextUI.dismiss();

    
    SelectionHelperUI.closeEditSession();

    
    this._container.show(this);

    this.search(this._textbox.value);
    this._textbox.select();
    this._textbox.focus();
    this._open = true;
    Elements.browsers.setAttribute("findbar", true);
    setTimeout(() => this._container.setAttribute("showing", true), 0);

    
    Browser.selectedBrowser.scrollSync = false;
  },

  hide: function findHelperHide() {
    if (!this._open)
      return;

    let onTransitionEnd = () => {
      this._container.removeEventListener("transitionend", onTransitionEnd, true);
      this._textbox.value = "";
      this.status = null;
      this._textbox.blur();
      this._container.hide(this);
      this._open = false;

      
      Browser.selectedBrowser.scrollSync = true;
    };

    this._container.addEventListener("transitionend", onTransitionEnd, true);
    this._container.removeAttribute("showing");
    Elements.browsers.removeAttribute("findbar");
  },

  goToPrevious: function findHelperGoToPrevious() {
    Browser.selectedBrowser.messageManager.sendAsyncMessage("FindAssist:Previous", { });
  },

  goToNext: function findHelperGoToNext() {
    Browser.selectedBrowser.messageManager.sendAsyncMessage("FindAssist:Next", { });
  },

  search: function findHelperSearch(aValue) {
    this.updateCommands(aValue);

    Browser.selectedBrowser.messageManager.sendAsyncMessage("FindAssist:Find", { searchString: aValue });
  },

  updateCommands: function findHelperUpdateCommands(aValue) {
    let disabled = (this._status == Ci.nsITypeAheadFind.FIND_NOTFOUND) || (aValue == "");
    this._cmdPrevious.setAttribute("disabled", disabled);
    this._cmdNext.setAttribute("disabled", disabled);
  },

  _zoom: function _findHelperZoom(aElementRect) {
    let autozoomEnabled = Services.prefs.getBoolPref("findhelper.autozoom");
    if (!aElementRect || !autozoomEnabled)
      return;

    if (Browser.selectedTab.allowZoom) {
      let zoomLevel = Browser._getZoomLevelForRect(aElementRect);

      
      let defaultZoomLevel = Browser.selectedTab.getDefaultZoomLevel();
      zoomLevel = Util.clamp(zoomLevel, (defaultZoomLevel * kBrowserFormZoomLevelMin),
                                        (defaultZoomLevel * kBrowserFormZoomLevelMax));
      zoomLevel = Browser.selectedTab.clampZoomLevel(zoomLevel);

      let zoomRect = Browser._getZoomRectForPoint(aElementRect.center().x, aElementRect.y, zoomLevel);
      AnimatedZoom.animateTo(zoomRect);
    } else {
      
      
      let zoomRect = Browser._getZoomRectForPoint(aElementRect.center().x, aElementRect.y, getBrowser().scale);
      AnimatedZoom.animateTo(zoomRect);
    }
  }
};
