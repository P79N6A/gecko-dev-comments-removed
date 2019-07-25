










































let pb = Cc["@mozilla.org/privatebrowsing;1"].
         getService(Ci.nsIPrivateBrowsingService);

function test() {
  waitForExplicitFinish();
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);

  
  pb.privateBrowsingEnabled = false;
  openLibrary(testPBoff);
}

function openLibrary(callback) {
  var library = window.openDialog("chrome://browser/content/places/places.xul",
                                  "", "chrome,toolbar=yes,dialog=no,resizable");
  waitForFocus(function () {
    callback(library);
  }, library);
}

function testPBoff(win) {
  
  let importMenuItem = win.document.getElementById("OrganizerCommand_browserImport");

  
  ok(!importMenuItem.hasAttribute("disabled"),
    "Import From Another Browser menu item should be enabled outside PB mode when opening the Library");

  
  pb.privateBrowsingEnabled = true;
  ok(importMenuItem.hasAttribute("disabled"),
    "Import From Another Browser menu item should be disabled after starting PB mode");

  
  pb.privateBrowsingEnabled = false;
  ok(!importMenuItem.hasAttribute("disabled"),
    "Import From Another Browser menu item should not be disabled after leaving the PB mode");

  win.close();

  
  pb.privateBrowsingEnabled = true;
  openLibrary(testPBon);
}

function testPBon(win) {
  let importMenuItem = win.document.getElementById("OrganizerCommand_browserImport");

  
  ok(importMenuItem.hasAttribute("disabled"),
    "Import From Another Browser menu item should be disabled in PB mode when opening the Libary");

  
  pb.privateBrowsingEnabled = false;
  ok(!importMenuItem.hasAttribute("disabled"),
    "Import From Another Browser menu item should not be disabled after leaving PB mode");

  win.close();

  
  gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
  finish();
}
