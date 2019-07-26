


"use strict";

var SelectHelper = {
  _uiBusy: false,

  handleEvent: function(aEvent) {
    this.handleClick(aEvent.target);
  },

  handleClick: function(aTarget) {
    
    
    if (this._uiBusy || !this._isMenu(aTarget) || aTarget.disabled)
        return;

    this._uiBusy = true;
    this.show(aTarget);
    this._uiBusy = false;
  },

  show: function(aElement) {
    let list = this.getListForElement(aElement);

    let p = new Prompt({
      window: aElement.contentDocument
    });

    if (aElement.multiple) {
      p.addButton({
        label: Strings.browser.GetStringFromName("selectHelper.closeMultipleSelectDialog")
      }).setMultiChoiceItems(list);
    } else {
      p.setSingleChoiceItems(list);
    }

    p.show((function(data) {
      let selected = data.button;
      if (selected == -1)
          return;

      let changed = false;
      if (aElement instanceof Ci.nsIDOMXULMenuListElement) {
        aElement.selectedIndex = selected;
      } else if (aElement instanceof HTMLSelectElement) {
        if (!Array.isArray(selected)) {
          let temp = [];
          for (let i = 0; i <= list.length; i++) {
            temp[i] = (i == selected);
          }
          selected = temp;
        }

        let i = 0;
        this.forOptions(aElement, function(aNode) {
          if (aNode.selected != selected[i]) {
            changed = true;
            aNode.selected = selected[i];
          }
          i++
        });
      }

      if (changed)
        this.fireOnChange(aElement);
    }).bind(this));
  },

  _isMenu: function(aElement) {
    return (aElement instanceof HTMLSelectElement ||
            aElement instanceof Ci.nsIDOMXULMenuListElement);
  },

  getListForElement: function(aElement) {
    let index = 0;
    let items = [];
    this.forOptions(aElement, function(aNode, aOptions, aParent) {
      let item = {
        label: aNode.text || aNode.label,
        header: aOptions.isGroup,
        disabled: aNode.disabled,
        id: index,
        selected: aNode.selected
      }

      if (aParent) {
        item.child = true;
        item.disabled = item.disabled || aParent.disabled;
      }
      items.push(item);

      index++;
    });
    return items;
  },

  forOptions: function(aElement, aFunction, aParent = null) {
    let element = aElement;
    if (aElement instanceof Ci.nsIDOMXULMenuListElement)
      element = aElement.menupopup;
    let children = element.children;
    let numChildren = children.length;

    
    if (numChildren == 0)
      aFunction.call(this, { label: "" }, { isGroup: false }, aParent);

    for (let i = 0; i < numChildren; i++) {
      let child = children[i];
      if (child instanceof HTMLOptionElement ||
          child instanceof Ci.nsIDOMXULSelectControlItemElement) {
        aFunction.call(this, child, { isGroup: false }, aParent);
      } else if (child instanceof HTMLOptGroupElement) {
        aFunction.call(this, child, { isGroup: true });
        this.forOptions(child, aFunction, child);

      }
    }
  },

  fireOnChange: function(aElement) {
    let evt = aElement.ownerDocument.createEvent("Events");
    evt.initEvent("change", true, true, aElement.defaultView, 0,
                  false, false,
                  false, false, null);
    setTimeout(function() {
      aElement.dispatchEvent(evt);
    }, 0);
  }
};
