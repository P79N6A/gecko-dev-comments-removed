# -*- indent-tabs-mode: nil; js-indent-level: 4 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

var gTabsPane = {

  




















  init: function () {
#ifdef XP_WIN
    const Cc = Components.classes;
    const Ci = Components.interfaces;
    try {
      let sysInfo = Cc["@mozilla.org/system-info;1"].
                    getService(Ci.nsIPropertyBag2);
      let ver = parseFloat(sysInfo.getProperty("version"));
      let showTabsInTaskbar = document.getElementById("showTabsInTaskbar");
      showTabsInTaskbar.hidden = ver < 6.1;
    } catch (ex) {}
#endif

    
    
    
    
    let TransientPrefs =
      Components.utils.import("resource:///modules/TransientPrefs.jsm", {})
                .TransientPrefs;
    if (!TransientPrefs.prefShouldBeVisible("browser.tabs.warnOnClose"))
      document.getElementById("warnCloseMultiple").hidden = true;
    if (!TransientPrefs.prefShouldBeVisible("browser.tabs.warnOnOpen"))
      document.getElementById("warnOpenMany").hidden = true;
  },

  




  readLinkTarget: function() {
    var openNewWindow = document.getElementById("browser.link.open_newwindow");
    return openNewWindow.value != 2;
  },

  





  writeLinkTarget: function() {
    var linkTargeting = document.getElementById("linkTargeting");
    return linkTargeting.checked ? 3 : 2;
  }
};
