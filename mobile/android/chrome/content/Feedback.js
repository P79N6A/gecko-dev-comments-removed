


"use strict";

var Feedback = {
  observe: function(aMessage, aTopic, aData) {
    if (aTopic !== "Feedback:Show")
      return;

    
    try {
      Services.prefs.getCharPref("distribution.id");
    } catch (e) {
      BrowserApp.addTab("about:feedback?source=feedback-prompt", { selected: true, parentId: BrowserApp.selectedTab.id });
    }
  }
};
