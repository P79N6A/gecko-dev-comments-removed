




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const kPrivateBrowsingNotification = "private-browsing";
const kPrivateBrowsingCancelVoteNotification = "private-browsing-cancel-vote";
const kEnter = "enter";
const kExit = "exit";

function LOG(aMsg) {
  aMsg = ("*** PRIVATEBROWSING TESTS: " + aMsg);
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).
                                      logStringMessage(aMsg);
  print(aMsg);
}
