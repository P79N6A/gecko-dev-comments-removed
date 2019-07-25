




let gTests = [];
let gCurrentTest = null;

function test() {
  
  
  waitForExplicitFinish();

  
  setTimeout(runNextTest, 200);
}



function runNextTest() {
  
  if (gTests.length > 0) {
    gCurrentTest = gTests.shift();
    info(gCurrentTest.desc);
    gCurrentTest.run();
  }
  else {
    BrowserUI.closeAutoComplete(true);
    finish();
  }
}



gTests.push({
  desc: "Test opening the awesome panel and checking the urlbar readonly state",

  run: function() {
    is(BrowserUI._edit.readOnly, true, "urlbar input textbox should be readonly");

    let popup = document.getElementById("popup_autocomplete");
    popup.addEventListener("popupshown", function(aEvent) {
      popup.removeEventListener("popupshown", arguments.callee, true);
      gCurrentTest.onPopupReady();
    }, true);

    BrowserUI.doCommand("cmd_openLocation");
  },

  onPopupReady: function() {
    is(Elements.urlbarState.getAttribute("mode"), "edit", "bcast_urlbarState mode attribute should be equal to 'edit'");
    is(BrowserUI._edit.readOnly, true, "urlbar input textbox be readonly once it is open in landscape");

    BrowserUI._edit.click();
    is(BrowserUI._edit.readOnly, false, "urlbar input textbox should not be readonly once it is open in landscape and click again");

    runNextTest();
  }
});
