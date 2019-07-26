


"use strict";

function test() {
  runTests();
}

gTests.push({
  desc: "rotating divs",
  run: function run() {
    yield addTab(chromeRoot + "res/divs_test.html", true);

    yield hideContextUI();

    let stopwatch = new StopWatch();

    let win = Browser.selectedTab.browser.contentWindow;

    PerfTest.declareTest("B924F3FA-4CB5-4453-B131-53E3611E0765",
                         "rotating divs w/text", "graphics", "content",
                         "Measures animation frames for rotating translucent divs on top of a background of text.");


    stopwatch.start();
    
    let event = yield waitForEvent(win, "testfinished", 20000);
    let msec = stopwatch.stop();

    PerfTest.declareNumericalResult((event.detail.frames / msec) * 1000.0, "fps");
  }
});


