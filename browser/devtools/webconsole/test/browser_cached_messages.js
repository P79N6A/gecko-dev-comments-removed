







"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-webconsole-error-observer.html";

function test() {
  waitForExplicitFinish();

  expectUncaughtException();

  loadTab(TEST_URI).then(testOpenUI);
}

function testOpenUI(aTestReopen) {
  openConsole().then((hud) => {
    waitForMessages({
      webconsole: hud,
      messages: [
        {
          text: "log Bazzle",
          category: CATEGORY_WEBDEV,
          severity: SEVERITY_LOG,
        },
        {
          text: "error Bazzle",
          category: CATEGORY_WEBDEV,
          severity: SEVERITY_ERROR,
        },
        {
          text: "bazBug611032",
          category: CATEGORY_JS,
          severity: SEVERITY_ERROR,
        },
        {
          text: "cssColorBug611032",
          category: CATEGORY_CSS,
          severity: SEVERITY_WARNING,
        },
      ],
    }).then(() => {
      closeConsole(gBrowser.selectedTab).then(() => {
        aTestReopen && info("will reopen the Web Console");
        executeSoon(aTestReopen ? testOpenUI : finishTest);
      });
    });
  });
}
