


"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;

function test() {
  UITourTest();
}

let tests = [
  function test_openSearchPanel(done) {
    let searchbar = document.getElementById("searchbar");

    
    
    Services.prefs.setBoolPref("browser.search.suggest.enabled", false);
    registerCleanupFunction(() => {
      Services.prefs.clearUserPref("browser.search.suggest.enabled");
    });

    ok(!searchbar.textbox.open, "Popup starts as closed");
    gContentAPI.openSearchPanel(() => {
      ok(searchbar.textbox.open, "Popup was opened");
      searchbar.textbox.closePopup();
      ok(!searchbar.textbox.open, "Popup was closed");
      done();
    });
  },
];
