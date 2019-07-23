








































var gPrivacyPane = {

  



  init: function ()
  {
    this._updateHistoryDaysUI();
    this.updateClearNowButtonLabel();
  },

  

  



















  




  _updateHistoryDaysUI: function ()
  {
    var pref = document.getElementById("browser.history_expire_days");
    var mirror = document.getElementById("browser.history_expire_days.mirror");
    var textbox = document.getElementById("historyDays");
    var checkbox = document.getElementById("rememberHistoryDays");

    
    if (mirror.value === null || mirror.value != pref.value)
      mirror.value = pref.value ? pref.value : pref.defaultValue;

    checkbox.checked = (pref.value > 0);
    textbox.disabled = !checkbox.checked;

    
    textbox.setAttribute("onsynctopreference", "return gPrivacyPane._writeHistoryDaysMirror();");
    textbox.setAttribute("preference", "browser.history_expire_days.mirror");
    mirror.updateElements();
  },

  



  _writeHistoryDaysMirror: function ()
  {
    var pref = document.getElementById("browser.history_expire_days");
    var textbox = document.getElementById("historyDays");
    pref.value = textbox.value;

    
    return undefined;
  },

  




  onchangeHistoryDaysCheck: function ()
  {
    var pref = document.getElementById("browser.history_expire_days");
    var mirror = document.getElementById("browser.history_expire_days.mirror");
    var textbox = document.getElementById("historyDays");
    var checkbox = document.getElementById("rememberHistoryDays");

    pref.value = checkbox.checked ? mirror.value : 0;
    textbox.disabled = !checkbox.checked;
  },

  




  readDownloadRetention: function ()
  {
    var pref = document.getElementById("browser.download.manager.retention");
    return (pref.value == 2);
  },

  



  writeDownloadRetention: function ()
  {
    var checkbox = document.getElementById("rememberDownloads");
    return checkbox.checked ? 2 : 0;
  },

  

  
















  




  readAcceptCookies: function ()
  {
    var pref = document.getElementById("network.cookie.cookieBehavior");
    var keepUntil = document.getElementById("keepUntil");
    var menu = document.getElementById("keepCookiesUntil");

    
    var acceptCookies = (pref.value != 2);

    keepUntil.disabled = menu.disabled = !acceptCookies;
    
    return acceptCookies;
  },

  



  writeAcceptCookies: function ()
  {
    var checkbox = document.getElementById("acceptCookies");
    return checkbox.checked ? 0 : 2;
  },

  


  showCookieExceptions: function ()
  {
    var bundlePreferences = document.getElementById("bundlePreferences");
    var params = { blockVisible   : true, 
                   sessionVisible : true, 
                   allowVisible   : true, 
                   prefilledHost  : "", 
                   permissionType : "cookie",
                   windowTitle    : bundlePreferences.getString("cookiepermissionstitle"),
                   introText      : bundlePreferences.getString("cookiepermissionstext") };
    document.documentElement.openWindow("Browser:Permissions",
                                        "chrome://browser/content/preferences/permissions.xul",
                                        "", params);
  },

  

  
  showCookies: function (aCategory)
  {
    document.documentElement.openWindow("Browser:Cookies",
                                        "chrome://browser/content/preferences/cookies.xul",
                                        "", null);
  },

  

  









  






  updateClearNowButtonLabel: function ()
  {
    var pref = document.getElementById("privacy.sanitize.promptOnSanitize");
    var clearNowButton = document.getElementById("clearDataNow");

    if (pref.valueFromPreferences)
      clearNowButton.label = clearNowButton.getAttribute("label1"); 
    else
      clearNowButton.label = clearNowButton.getAttribute("label2"); 
  },

  


  showClearPrivateDataSettings: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/sanitize.xul",
                                           "", null);
    this.updateClearNowButtonLabel();
  },

  





  clearPrivateDataNow: function ()
  {
    const Cc = Components.classes, Ci = Components.interfaces;
    var glue = Cc["@mozilla.org/browser/browserglue;1"]
                 .getService(Ci.nsIBrowserGlue);
    glue.sanitize(window || null);
  }

};
