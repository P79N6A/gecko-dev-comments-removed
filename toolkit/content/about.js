




var formatter = Components.classes["@mozilla.org/toolkit/URLFormatterService;1"]
                          .getService(Components.interfaces.nsIURLFormatter);
var releaseNotesURL = formatter.formatURLPref("app.releaseNotesURL");
if (releaseNotesURL != "about:blank") {
  var relnotes = document.getElementById("releaseNotesURL");
  relnotes.setAttribute("href", releaseNotesURL);
  relnotes.parentNode.removeAttribute("hidden");
}

var vendorURL = formatter.formatURLPref("app.vendorURL");
if (vendorURL != "about:blank") {
  var vendor = document.getElementById("vendorURL");
  vendor.setAttribute("href", vendorURL);
}


var versionNum = Components.classes["@mozilla.org/xre/app-info;1"]
                           .getService(Components.interfaces.nsIXULAppInfo)
                           .version;
var version = document.getElementById("version");
version.textContent += " " + versionNum;


var ua = navigator.userAgent;
if (ua) {
  document.getElementById("buildID").textContent += " " + ua;
}
