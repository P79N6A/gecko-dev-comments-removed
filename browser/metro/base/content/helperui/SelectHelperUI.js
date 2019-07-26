



"use strict";





var SelectHelperUI = {
  _list: null,

  get _container() {
    delete this._container;
    return this._container = document.getElementById("select-container");
  },

  get _listbox() {
    delete this._listbox;
    return this._listbox = document.getElementById("select-commands");
  },

  get _menuPopup() {
    let popup = document.getElementById("select-popup");
    delete this._menuPopup;
    return this._menuPopup = new MenuPopup(this._container, popup);
  },

  show: function selectHelperShow(aList, aTitle, aRect) {
    if (this._list)
      this.reset();

    this._list = aList;

    
    this._container.setAttribute("multiple", aList.multiple ? "true" : "false");

    let firstSelected = null;

    
    let fragment = document.createDocumentFragment();
    let choices = aList.choices;
    for (let i = 0; i < choices.length; i++) {
      let choice = choices[i];
      let item = document.createElement("richlistitem");
      let label = document.createElement("label");

      item.setAttribute("class", "option-command listitem-iconic");
      item.setAttribute("flex", "1");
      item.setAttribute("crop", "center");
      label.setAttribute("value", choice.text);
      item.appendChild(label);

      choice.selected ? item.setAttribute("selected", "true")
                      : item.removeAttribute("selected");

      choice.disabled ? item.setAttribute("disabled", "true")
                      : item.removeAttribute("disabled");
      fragment.appendChild(item);

      if (choice.group) {
        item.classList.add("optgroup");
        continue;
      }

      item.optionIndex = choice.optionIndex;
      item.choiceIndex = i;

      if (choice.inGroup)
        item.classList.add("in-optgroup");

      if (choice.selected) {
        item.classList.add("selected");
        firstSelected = firstSelected || item;
      }
    }
    this._listbox.appendChild(fragment);

    this._container.addEventListener("click", this, false);
    this._menuPopup.show(this._positionOptions(aRect));
    this._listbox.ensureElementIsVisible(firstSelected);
  },

  reset: function selectHelperReset() {
    this._updateControl();
    while (this._listbox.hasChildNodes())
      this._listbox.removeChild(this._listbox.lastChild);
    this._list = null;
  },

  hide: function selectHelperHide() {
    if (!this._list)
      return;

    this._container.removeEventListener("click", this, false);
    this._menuPopup.hide();
    this.reset();
  },

  _positionOptions: function _positionOptions(aRect) {
    let browser = Browser.selectedBrowser;
    let p0 = browser.ptBrowserToClient(aRect.left, aRect.top);
    let p1 = browser.ptBrowserToClient(aRect.right, aRect.bottom);

    return {
      xPos: p0.x,
      yPos: p1.y,
      bottomAligned: false,
      leftAligned: true
    };
  },

  _forEachOption: function _selectHelperForEachOption(aCallback) {
    let children = this._listbox.childNodes;
    for (let i = 0; i < children.length; i++) {
      let item = children[i];
      if (!item.hasOwnProperty("optionIndex"))
        continue;
      aCallback(item, i);
    }
  },

  _updateControl: function _selectHelperUpdateControl() {
    Browser.selectedBrowser.messageManager.sendAsyncMessage("FormAssist:ChoiceChange", { });
  },

  handleEvent: function selectHelperHandleEvent(aEvent) {
    switch (aEvent.type) {
      case "click":
        let item = aEvent.target;
        if (item && item.hasOwnProperty("optionIndex")) {
          if (this._list.multiple) {
            item.classList.toggle("selected");
          } else {
            item.classList.add("selected");
          }
          this.onSelect(item.optionIndex, item.classList.contains("selected"));
        }
        break;
    }
  },

  onSelect: function selectHelperOnSelect(aIndex, aSelected) {
    Browser.selectedBrowser.messageManager.sendAsyncMessage("FormAssist:ChoiceSelect", {
      index: aIndex,
      selected: aSelected
    });
    if (!this._list.multiple) {
      this.hide();
    }
  }
};
