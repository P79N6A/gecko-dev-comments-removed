"use strict";








const URL = 'data:text/html;charset=utf-8,' +
            '<body><button onblur="this.remove()">' +
            '<script>document.body.firstChild.focus()</script></body>';

function getFocusedLocalName(browser) {
  return ContentTask.spawn(browser, null, function* () {
    return content.document.activeElement.localName;
  });
}

add_task(function* () {
  gBrowser.selectedTab = gBrowser.addTab(URL);
  let browser = gBrowser.selectedBrowser;
  yield BrowserTestUtils.browserLoaded(browser);

  is((yield getFocusedLocalName(browser)), "button", "button is focused");

  let promiseFocused = ContentTask.spawn(browser, null, function* () {
    return new Promise(resolve => {
      content.addEventListener("focus", function onFocus({target}) {
        if (String(target.location).startsWith("data:")) {
          content.removeEventListener("focus", onFocus);
          resolve();
        }
      });
    });
  });

  
  
  
  gBrowser.selectedTab = gBrowser.addTab("about:blank");
  yield BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);
  gBrowser.removeCurrentTab();

  
  yield promiseFocused;

  
  is((yield getFocusedLocalName(browser)), "body", "body is focused");

  
  gBrowser.removeCurrentTab();
});
