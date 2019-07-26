


"use strict";

var InputWidgetHelper = {
  _uiBusy: false,

  handleEvent: function(aEvent) {
    this.handleClick(aEvent.target);
  },

  handleClick: function(aTarget) {
    
    
    if (this._uiBusy || !this.hasInputWidget(aTarget))
      return;

    this._uiBusy = true;
    this.show(aTarget);
    this._uiBusy = false;
  },

  show: function(aElement) {
    let type = aElement.getAttribute('type');
    let p = new Prompt({
      window: aElement.ownerDocument.defaultView,
      title: Strings.browser.GetStringFromName("inputWidgetHelper." + aElement.getAttribute('type')),
      buttons: [
        Strings.browser.GetStringFromName("inputWidgetHelper.set"),
        Strings.browser.GetStringFromName("inputWidgetHelper.clear"),
        Strings.browser.GetStringFromName("inputWidgetHelper.cancel")
      ],
    }).addDatePicker({
      value: aElement.value,
      type: type,
    }).show((function(data) {
      let changed = false;
      if (data.button == -1) {
        
        return;
      }
      if (data.button == 1) {
        
        if (aElement.value != "") {
          aElement.value = "";
          changed = true;
        }
      } else if (data.button == 0) {
        
        if (aElement.value != data[type]) {
          aElement.value = data[type + "0"];
          changed = true;
        }
      }
      

      if (changed)
        this.fireOnChange(aElement);
    }).bind(this));
  },

  hasInputWidget: function(aElement) {
    if (!aElement instanceof HTMLInputElement)
      return false;

    let type = aElement.getAttribute('type');
    if (type == "date" || type == "datetime" || type == "datetime-local" ||
        type == "week" || type == "month" || type == "time") {
      return true;
    }

    return false;
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
