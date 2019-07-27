



const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

var stringBundle = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService)
                                                         .createBundle("chrome://browser/locale/aboutPrivateBrowsing.properties");

if (!PrivateBrowsingUtils.isWindowPrivate(window)) {
  document.title = stringBundle.GetStringFromName("titleNormal");
  setFavIcon("chrome://global/skin/icons/question-16.png");
} else {
  document.title = stringBundle.GetStringFromName("title");
  setFavIcon("chrome://browser/skin/Privacy-16.png");
}

var mainWindow = window.QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIWebNavigation)
                       .QueryInterface(Ci.nsIDocShellTreeItem)
                       .rootTreeItem
                       .QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIDOMWindow);

function setFavIcon(url) {
  var icon = document.createElement("link");
  icon.setAttribute("rel", "icon");
  icon.setAttribute("type", "image/png");
  icon.setAttribute("href", url);
  var head = document.getElementsByTagName("head")[0];
  head.insertBefore(icon, head.firstChild);
}

document.addEventListener("DOMContentLoaded", function () {
  if (!PrivateBrowsingUtils.isWindowPrivate(window)) {
    document.body.setAttribute("class", "normal");
  }

  
  let learnMoreURL = Cc["@mozilla.org/toolkit/URLFormatterService;1"]
                     .getService(Ci.nsIURLFormatter)
                     .formatURLPref("app.support.baseURL");
  let learnMore = document.getElementById("learnMore");
  if (learnMore) {
    learnMore.setAttribute("href", learnMoreURL + "private-browsing");
  }

  let startPrivateBrowsing = document.getElementById("startPrivateBrowsing");
  if (startPrivateBrowsing)
    startPrivateBrowsing.addEventListener("command", openPrivateWindow);
}, false);

function openPrivateWindow() {
  mainWindow.OpenBrowserWindow({private: true});
}