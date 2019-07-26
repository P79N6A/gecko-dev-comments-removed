



var gTabsPane = {

  




















#ifdef XP_WIN
  


  init: function () {
    try {
      let sysInfo = Cc["@mozilla.org/system-info;1"].
                    getService(Ci.nsIPropertyBag2);
      let ver = parseFloat(sysInfo.getProperty("version"));
      let showTabsInTaskbar = document.getElementById("showTabsInTaskbar");
      showTabsInTaskbar.hidden = ver < 6.1 || (ver >= 6.1 && history.state != "tabs");
    } catch (ex) {}
  },
#endif

  




  readLinkTarget: function() {
    var openNewWindow = document.getElementById("browser.link.open_newwindow");
    return openNewWindow.value != 2;
  },

  





  writeLinkTarget: function() {
    var linkTargeting = document.getElementById("linkTargeting");
    return linkTargeting.checked ? 3 : 2;
  }
};
