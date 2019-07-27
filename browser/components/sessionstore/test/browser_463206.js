



"use strict";

const TEST_URL = "http://mochi.test:8888/browser/" +
                 "browser/components/sessionstore/test/browser_463206_sample.html";

add_task(function* () {
  
  let tab = gBrowser.addTab(TEST_URL);
  yield BrowserTestUtils.browserLoaded(tab.linkedBrowser);

  
  yield ContentTask.spawn(tab.linkedBrowser, null, function* () {
    function typeText(aTextField, aValue) {
      aTextField.value = aValue;

      let event = aTextField.ownerDocument.createEvent("UIEvents");
      event.initUIEvent("input", true, true, aTextField.ownerDocument.defaultView, 0);
      aTextField.dispatchEvent(event);
    }

    typeText(content.document.getElementById("out1"), Date.now());
    typeText(content.document.getElementsByName("1|#out2")[0], Math.random());
    typeText(content.frames[0].frames[1].document.getElementById("in1"), new Date());
  });

  
  let tab2 = gBrowser.duplicateTab(tab);
  yield promiseTabRestored(tab2);

  
  let query = ContentTask.spawn(tab2.linkedBrowser, null, function* () {
    return [
      content.document.getElementById("out1").value,
      content.frames[1].document.getElementById("out1").value,
      content.document.getElementsByName("1|#out2")[0].value,
      content.frames[1].document.getElementById("out2").value,
      content.frames[0].frames[1].document.getElementById("in1").value,
      content.frames[1].frames[0].document.getElementById("in1").value
    ];
  });

  let [v1, v2, v3, v4, v5, v6] = yield query;
  isnot(v1, v2, "text isn't reused for frames");
  isnot(v3, "", "text containing | and # is correctly restored");
  is(v4, "", "id prefixes can't be faked");
  
  
  is(v6, "", "id prefixes aren't mixed up");

  
  gBrowser.removeTab(tab2);
  gBrowser.removeTab(tab);
});
