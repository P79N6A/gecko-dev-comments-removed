







"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-bug-658368-time-methods.html";

const TEST_URI2 = "data:text/html;charset=utf-8,<script>" +
                  "console.timeEnd('bTimer');</script>";

const TEST_URI3 = "data:text/html;charset=utf-8,<script>" +
                  "console.time('bTimer');</script>";

const TEST_URI4 = "data:text/html;charset=utf-8," +
                  "<script>console.timeEnd('bTimer');</script>";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud1 = yield openConsole();

  yield waitForMessages({
    webconsole: hud1,
    messages: [{
      name: "aTimer started",
      consoleTime: "aTimer",
    }, {
      name: "aTimer end",
      consoleTimeEnd: "aTimer",
    }],
  });

  
  
  let { browser } = yield loadTab(TEST_URI2);
  let hud2 = yield openConsole();

  testLogEntry(hud2.outputNode, "bTimer: timer started",
               "bTimer was not started", false, true);

  
  
  content.location = TEST_URI3;

  yield waitForMessages({
    webconsole: hud2,
    messages: [{
      name: "bTimer started",
      consoleTime: "bTimer",
    }],
  });

  hud2.jsterm.clearOutput();

  
  
  content.location = TEST_URI4;
  yield loadBrowser(browser);

  testLogEntry(hud2.outputNode, "bTimer: timer started",
               "bTimer was not started", false, true);
});
