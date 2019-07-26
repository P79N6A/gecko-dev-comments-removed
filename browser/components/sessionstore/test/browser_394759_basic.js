



const TEST_URL = "data:text/html;charset=utf-8,<input%20id=txt>" +
                 "<input%20type=checkbox%20id=chk>";






function test() {
  waitForExplicitFinish();

  let uniqueKey = "bug 394759";
  let uniqueValue = "unik" + Date.now();
  let uniqueText = "pi != " + Math.random();

  
  while (SessionStore.getClosedWindowCount()) {
    SessionStore.forgetClosedWindow(0);
  }

  provideWindow(function onTestURLLoaded(newWin) {
    newWin.gBrowser.addTab().linkedBrowser.stop();

    
    ss.setWindowValue(newWin, uniqueKey, uniqueValue);
    let [txt, chk] = newWin.content.document.querySelectorAll("#txt, #chk");
    txt.value = uniqueText;

    
    EventUtils.sendMouseEvent({type: "click"}, chk);

    let browser = newWin.gBrowser.selectedBrowser;
    waitForContentMessage(browser, "SessionStore:input", 1000, result => {
      ok(result, "received message for input changes");

      newWin.close();

      
      executeSoon(function() {
        is(ss.getClosedWindowCount(), 1,
           "The closed window was added to Recently Closed Windows");
        let data = JSON.parse(ss.getClosedWindowData())[0];
        ok(data.title == TEST_URL && JSON.stringify(data).indexOf(uniqueText) > -1,
           "The closed window data was stored correctly");

        
        let newWin2 = ss.undoCloseWindow(0);

        ok(newWin2 instanceof ChromeWindow,
           "undoCloseWindow actually returned a window");
        is(ss.getClosedWindowCount(), 0,
           "The reopened window was removed from Recently Closed Windows");

        
        let restoredTabs = 0;
        let expectedTabs = data.tabs.length;
        newWin2.addEventListener("SSTabRestored", function sstabrestoredListener(aEvent) {
          ++restoredTabs;
          info("Restored tab " + restoredTabs + "/" + expectedTabs);
          if (restoredTabs < expectedTabs) {
            return;
          }

          is(restoredTabs, expectedTabs, "correct number of tabs restored");
          newWin2.removeEventListener("SSTabRestored", sstabrestoredListener, true);

          is(newWin2.gBrowser.tabs.length, 2,
             "The window correctly restored 2 tabs");
          is(newWin2.gBrowser.currentURI.spec, TEST_URL,
             "The window correctly restored the URL");

          let [txt, chk] = newWin2.content.document.querySelectorAll("#txt, #chk");
          ok(txt.value == uniqueText && chk.checked,
             "The window correctly restored the form");
          is(ss.getWindowValue(newWin2, uniqueKey), uniqueValue,
             "The window correctly restored the data associated with it");

          
          newWin2.close();
          finish();
        }, true);
      });
    });
  }, TEST_URL);
}
