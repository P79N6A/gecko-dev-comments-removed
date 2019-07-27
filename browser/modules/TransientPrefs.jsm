



"use strict";

this.EXPORTED_SYMBOLS = ["TransientPrefs"];

Components.utils.import("resource://gre/modules/Preferences.jsm");

let prefVisibility = new Map;





this.TransientPrefs = {
  prefShouldBeVisible: function (prefName) {
    if (Preferences.isSet(prefName))
      prefVisibility.set(prefName, true);

    return !!prefVisibility.get(prefName);
  }
};
