





const kPositionPadding = 10;

var AutofillMenuUI = {
  _popupState: null,
  __menuPopup: null,

  get _panel() { return document.getElementById("autofill-container"); },
  get _popup() { return document.getElementById("autofill-popup"); },
  get _commands() { return this._popup.childNodes[0]; },

  get _menuPopup() {
    if (!this.__menuPopup)
      this.__menuPopup = new MenuPopup(this._panel, this._popup);
      this.__menuPopup._wantTypeBehind = true;

    return this.__menuPopup;
  },

  _firePopupEvent: function _firePopupEvent(aEventName) {
    let menupopup = this._currentControl.menupopup;
    if (menupopup.hasAttribute(aEventName)) {
      let func = new Function("event", menupopup.getAttribute(aEventName));
      func.call(this);
    }
  },

  _emptyCommands: function _emptyCommands() {
    while (this._commands.firstChild)
      this._commands.removeChild(this._commands.firstChild);
  },

  _positionOptions: function _positionOptions() {
    let options = {
      forcePosition: true
    };
    options.xPos = this._anchorRect.x;
    options.yPos = this._anchorRect.y + this._anchorRect.height;
    options.bottomAligned = false;
    options.leftAligned = true;
    options.source = Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH;
    return options;
  },

  show: function show(aAnchorRect, aSuggestionsList) {
    this._anchorRect = aAnchorRect;
    this._emptyCommands();
    for (let idx = 0; idx < aSuggestionsList.length; idx++) {
      let item = document.createElement("richlistitem");
      let label = document.createElement("label");
      label.setAttribute("value", aSuggestionsList[idx].label);
      item.setAttribute("data", aSuggestionsList[idx].value);
      item.appendChild(label);
      this._commands.appendChild(item);
    }

    this._menuPopup.show(this._positionOptions());
  },

  selectByIndex: function mn_selectByIndex(aIndex) {
    this._menuPopup.hide();
    FormHelperUI.doAutoComplete(this._commands.childNodes[aIndex].getAttribute("data"));
  },

  hide: function hide () {
    this._menuPopup.hide();
  }
};

var ContextMenuUI = {
  _popupState: null,
  __menuPopup: null,
  _defaultPositionOptions: {
    forcePosition: true,
    bottomAligned: true,
    rightAligned: false,
    centerHorizontally: true,
    moveBelowToFit: true
  },

  get _panel() { return document.getElementById("context-container"); },
  get _popup() { return document.getElementById("context-popup"); },
  get _commands() { return this._popup.childNodes[0]; },

  get _menuPopup() {
    if (!this.__menuPopup)
      this.__menuPopup = new MenuPopup(this._panel, this._popup);

    return this.__menuPopup;
  },

  



  



  get popupState() {
    return this._popupState;
  },

  













  showContextMenu: function ch_showContextMenu(aMessage) {
    this._popupState = aMessage.json;
    this._popupState.target = aMessage.target;
    let contentTypes = this._popupState.types;

    















    Util.dumpLn("contentTypes:", contentTypes);

    
    
    let multipleMediaTypes = false;
    if (contentTypes.indexOf("link") != -1 &&
        (contentTypes.indexOf("image") != -1  ||
         contentTypes.indexOf("video") != -1 ||
         contentTypes.indexOf("selected-text") != -1))
      multipleMediaTypes = true;

    for (let command of Array.slice(this._commands.childNodes)) {
      command.hidden = true;
    }

    let optionsAvailable = false;
    for (let command of Array.slice(this._commands.childNodes)) {
      let types = command.getAttribute("type").split(",");
      let lowPriority = (command.hasAttribute("priority") &&
        command.getAttribute("priority") == "low");
      let searchTextItem = (command.id == "context-search");

      
      if (multipleMediaTypes && lowPriority)
        continue;

      for (let i = 0; i < types.length; i++) {
        
        if (types[i].charAt(0) == '!' && contentTypes.indexOf(types[i].substring(1)) != -1) {
          break;
        }
        if (contentTypes.indexOf(types[i]) != -1) {
          
          if (searchTextItem && !ContextCommands.searchTextSetup(command, this._popupState.string)) {
            break;
          }
          optionsAvailable = true;
          command.hidden = false;
          break;
        }
      }
    }

    if (!optionsAvailable) {
      this._popupState = null;
      return false;
    }

    this._menuPopup.show(Util.extend({}, this._defaultPositionOptions, {
      xPos: aMessage.json.xPos,
      yPos: aMessage.json.yPos,
      source: aMessage.json.source
    }));
    return true;
  },

  hide: function hide () {
    this._menuPopup.hide();
    this._popupState = null;
  },

  reset: function reset() {
    this._popupState = null;
  }
};

var MenuControlUI = {
  _currentControl: null,
  __menuPopup: null,

  get _panel() { return document.getElementById("menucontrol-container"); },
  get _popup() { return document.getElementById("menucontrol-popup"); },
  get _commands() { return this._popup.childNodes[0]; },

  get _menuPopup() {
    if (!this.__menuPopup)
      this.__menuPopup = new MenuPopup(this._panel, this._popup);

    return this.__menuPopup;
  },

  _firePopupEvent: function _firePopupEvent(aEventName) {
    let menupopup = this._currentControl.menupopup;
    if (menupopup.hasAttribute(aEventName)) {
      let func = new Function("event", menupopup.getAttribute(aEventName));
      func.call(this);
    }
  },

  _emptyCommands: function _emptyCommands() {
    while (this._commands.firstChild)
      this._commands.removeChild(this._commands.firstChild);
  },

  _positionOptions: function _positionOptions() {
    let position = this._currentControl.menupopup.position || "after_start";
    let rect = this._currentControl.getBoundingClientRect();

    let options = {
      forcePosition: true
    };

    

    switch (position) {
      case "before_start":
        options.xPos = rect.left;
        options.yPos = rect.top;
        options.bottomAligned = true;
        options.leftAligned = true;
        break;
      case "before_end":
        options.xPos = rect.right;
        options.yPos = rect.top;
        options.bottomAligned = true;
        options.rightAligned = true;
        break;
      case "after_start":
        options.xPos = rect.left;
        options.yPos = rect.bottom;
        options.topAligned = true;
        options.leftAligned = true;
        break;
      case "after_end":
        options.xPos = rect.right;
        options.yPos = rect.bottom;
        options.topAligned = true;
        options.rightAligned = true;
        break;

      
    }

    return options;
  },

  show: function show(aMenuControl) {
    this._currentControl = aMenuControl;
    this._panel.setAttribute("for", aMenuControl.id);
    this._firePopupEvent("onpopupshowing");

    this._emptyCommands();
    let children = this._currentControl.menupopup.children;
    for (let i = 0; i < children.length; i++) {
      let child = children[i];
      let item = document.createElement("richlistitem");

      if (child.disabled)
        item.setAttribute("disabled", "true");

      if (child.hidden)
        item.setAttribute("hidden", "true");

      
      
      if (child.selected)
        item.setAttribute("class", "selected");

      let image = document.createElement("image");
      image.setAttribute("src", child.image || "");
      item.appendChild(image);

      let label = document.createElement("label");
      label.setAttribute("value", child.label);
      item.appendChild(label);

      this._commands.appendChild(item);
    }

    this._menuPopup.show(this._positionOptions());
  },

  selectByIndex: function mn_selectByIndex(aIndex) {
    this._currentControl.selectedIndex = aIndex;

    
    if (this._currentControl.dispatchEvent) {
      let evt = document.createEvent("XULCommandEvent");
      evt.initCommandEvent("command", true, true, window, 0, false, false, false, false, null);
      this._currentControl.dispatchEvent(evt);
    }

    this._menuPopup.hide();
  }
};

function MenuPopup(aPanel, aPopup) {
  this._panel = aPanel;
  this._popup = aPopup;
  this._wantTypeBehind = false;
}

MenuPopup.prototype = {
  get _visible() { return !this._panel.hidden; },
  get _commands() { return this._popup.childNodes[0]; },

  show: function (aPositionOptions) {
    if (this._visible)
      return;

    window.addEventListener("keypress", this, true);
    window.addEventListener("mousedown", this, true);
    Elements.stack.addEventListener("PopupChanged", this, false);

    this._panel.hidden = false;
    this._position(aPositionOptions || {});

    let self = this;
    this._panel.addEventListener("transitionend", function () {
      self._panel.removeEventListener("transitionend", arguments.callee);
      self._panel.removeAttribute("showingfrom");

      let event = document.createEvent("Events");
      event.initEvent("popupshown", true, false);
      document.dispatchEvent(event);
    });

    let popupFrom = (aPositionOptions.forcePosition && !aPositionOptions.bottomAligned) ? "above" : "below";
    this._panel.setAttribute("showingfrom", popupFrom);

    
    setTimeout(function () {
      self._panel.setAttribute("showing", "true");
    }, 0);
  },

  hide: function () {
    if (!this._visible)
      return;

    window.removeEventListener("keypress", this, true);
    window.removeEventListener("mousedown", this, true);
    Elements.stack.removeEventListener("PopupChanged", this, false);

    let self = this;
    this._panel.addEventListener("transitionend", function () {
      self._panel.removeEventListener("transitionend", arguments.callee);
      self._panel.hidden = true;
      self._popupState = null;
      let event = document.createEvent("Events");
      event.initEvent("popuphidden", true, false);
      document.dispatchEvent(event);
    });

    this._panel.removeAttribute("showing");
  },

  _position: function _position(aPositionOptions) {
    let aX = aPositionOptions.xPos;
    let aY = aPositionOptions.yPos;
    let aSource = aPositionOptions.source;

    let width = this._popup.boxObject.width;
    let height = this._popup.boxObject.height;
    let halfWidth = width / 2;
    let halfHeight = height / 2;
    let screenWidth = ContentAreaObserver.width;
    let screenHeight = ContentAreaObserver.height;

    if (aPositionOptions.forcePosition) {
      if (aPositionOptions.rightAligned)
        aX -= width;

      if (aPositionOptions.bottomAligned)
        aY -= height;

      if (aPositionOptions.centerHorizontally)
        aX -= halfWidth;
    } else {
      let leftHand = MetroUtils.handPreference == MetroUtils.handPreferenceLeft;

      
      if (aSource && aSource == Ci.nsIDOMMouseEvent.MOZ_SOURCE_TOUCH) {
        if (leftHand) {
          this._commands.setAttribute("left-hand", true);
          this._commands.removeAttribute("right-hand");
        } else {
          this._commands.setAttribute("right-hand", true);
          this._commands.removeAttribute("left-hand");
        }
      }

      let hLeft = (aX - halfWidth - width - kPositionPadding) > kPositionPadding;
      let hRight = (aX + width + kPositionPadding) < screenWidth;
      let hCenter = (aX - halfWidth - kPositionPadding) > kPositionPadding;

      let vTop = (aY - height - kPositionPadding) > kPositionPadding;
      let vCenter = (aY - halfHeight - kPositionPadding) > kPositionPadding &&
                    aY + halfHeight < screenHeight;
      let vBottom = (aY + height + kPositionPadding) < screenHeight;

      if (leftHand && hLeft && vCenter) {
        dump('leftHand && hLeft && vCenter\n');
        aX -= (width + halfWidth);
        aY -= halfHeight; 
      } else if (!leftHand && hRight && vCenter) {
        dump('!leftHand && hRight && vCenter\n');
        aX += kPositionPadding;
        aY -= halfHeight; 
      } else if (vBottom && hCenter) {
        dump('vBottom && hCenter\n');
        aX -= halfWidth;
      } else if (vTop && hCenter) {
        dump('vTop && hCenter\n');
        aX -= halfWidth;
        aY -= height;
      } else if (hCenter && vCenter) {
        dump('hCenter && vCenter\n');
        aX -= halfWidth;
        aY -= halfHeight;
      } else {
        dump('None, left hand: ' + leftHand + '!\n');
      }
    }

    if (aX < 0) {
      aX = 0;
    } else if (aX + width + kPositionPadding > screenWidth){
      aX = screenWidth - width - kPositionPadding;
    }

    if (aY < 0 && aPositionOptions.moveBelowToFit) {
      
      aY = aPositionOptions.yPos;
    } else if (aY < 0) {
      aY = 0;
    }

    this._panel.left = aX;
    this._panel.top = aY;

    let excessY = (aY + height + kPositionPadding - screenHeight);
    this._popup.style.maxHeight = (excessY > 0) ? (height - excessY) + "px" : "none";

    let excessX = (aX + width + kPositionPadding - screenWidth);
    this._popup.style.maxWidth = (excessX > 0) ? (width - excessX) + "px" : "none";
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "keypress":
        if (!this._wantTypeBehind) {
          
          aEvent.stopPropagation();
          aEvent.preventDefault();
          if (aEvent.keyCode != aEvent.DOM_VK_ESCAPE)
            this.hide();
        }
        break;
      case "mousedown":
        if (!this._popup.contains(aEvent.target)) {
          aEvent.stopPropagation();
          this.hide();
        }
        break;
      case "PopupChanged":
        if (aEvent.detail) {
          this.hide();
        }
        break;
    }
  }
};
