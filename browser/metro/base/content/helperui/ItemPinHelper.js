




"use strict";

function ItemPinHelper(aUnpinnedPrefName) {
  this._prefKey = aUnpinnedPrefName;
}



ItemPinHelper._prefValue = {};

ItemPinHelper.prototype = {
  _getPrefValue: function _getPrefValue() {
    if (ItemPinHelper._prefValue[this._prefKey])
      return ItemPinHelper._prefValue[this._prefKey];

    try {
      
      let prefValue = Services.prefs.getComplexValue(this._prefKey, Ci.nsISupportsString);
      ItemPinHelper._prefValue[this._prefKey] = JSON.parse(prefValue.data);
    } catch(e) {
      ItemPinHelper._prefValue[this._prefKey] = [];
    }

    return ItemPinHelper._prefValue[this._prefKey];
  },

  _setPrefValue: function _setPrefValue(aNewValue) {
    let stringified = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
    stringified.data = JSON.stringify(aNewValue);

    Services.prefs.setComplexValue(this._prefKey, Ci.nsISupportsString, stringified);
    ItemPinHelper._prefValue[this._prefKey] = aNewValue;
  },

  isPinned: function isPinned(aItemId) {
    
    return this._getPrefValue().indexOf(aItemId) === -1;
  },

  setUnpinned: function setPinned(aItemId) {
    let unpinned = this._getPrefValue();
    unpinned.push(aItemId);
    this._setPrefValue(unpinned);
  },

  setPinned: function unsetPinned(aItemId) {
    let unpinned = this._getPrefValue();

    let index = unpinned.indexOf(aItemId);
    unpinned.splice(index, 1);

    this._setPrefValue(unpinned);
  },
}
