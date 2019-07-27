




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SimpleServiceDiscovery.jsm");

function ok(passed, text) {
  do_report_result(passed, text, Components.stack.caller, false);
}


let chromeWin;


let browser;

function middle(element) {
  let rect = element.getBoundingClientRect();
  let x = (rect.right - rect.left) / 2 + rect.left;
  let y = (rect.bottom - rect.top) / 2 + rect.top;
  return [x, y];
}


var testDevice = {
  id: "test:dummy",
  target: "test:service",
  factory: function(service) {   },
  types: ["video/mp4", "video/webm"],
  extensions: ["mp4", "webm"]
};

add_test(function setup_browser() {
  chromeWin = Services.wm.getMostRecentWindow("navigator:browser");
  let BrowserApp = chromeWin.BrowserApp;

  do_register_cleanup(function cleanup() {
    BrowserApp.closeTab(BrowserApp.getTabForBrowser(browser));
    SimpleServiceDiscovery.unregisterDevice(testDevice);
  });

  
  SimpleServiceDiscovery.registerDevice(testDevice);

  
  let service = {
    location: "http://mochi.test:8888/tests/robocop/simpleservice.xml",
    target: "test:service"
  };

  do_print("Force a detailed ping from a pretend service");

  
  SimpleServiceDiscovery._processService(service);

  
  let url = "http://mochi.test:8888/tests/robocop/video_discovery.html";
  browser = BrowserApp.addTab(url, { selected: true, parentId: BrowserApp.selectedTab.id }).browser;
  browser.addEventListener("load", function startTests(event) {
    browser.removeEventListener("load", startTests, true);
    Services.tm.mainThread.dispatch(run_next_test, Ci.nsIThread.DISPATCH_NORMAL);
  }, true);
});

let videoDiscoveryTests = [
  { id: "simple-mp4", source: "http://mochi.test:8888/simple.mp4", poster: "http://mochi.test:8888/simple.png", text: "simple video with mp4 src" },
  { id: "simple-fail", pass: false, text: "simple video with no mp4 src" },
  { id: "with-sources-mp4", source: "http://mochi.test:8888/simple.mp4", text: "video with mp4 extension source child" },
  { id: "with-sources-webm", source: "http://mochi.test:8888/simple.webm", text: "video with webm extension source child" },
  { id: "with-sources-fail", pass: false, text: "video with no mp4 extension source child" },
  { id: "with-sources-mimetype-mp4", source: "http://mochi.test:8888/simple-video-mp4", text: "video with mp4 mimetype source child" },
  { id: "with-sources-mimetype-webm", source: "http://mochi.test:8888/simple-video-webm", text: "video with webm mimetype source child" },
  { id: "video-overlay", source: "http://mochi.test:8888/simple.mp4", text: "div overlay covering a simple video with mp4 src" }
];

function execute_video_test(test) {
  let element = browser.contentDocument.getElementById(test.id);
  if (element) {
    let [x, y] = middle(element);
    let video = chromeWin.CastingApps.getVideo(element, x, y);
    if (video) {
      let matchPoster = (test.poster == video.poster);
      let matchSource = (test.source == video.source);
      ok(matchPoster && matchSource && test.pass, test.text);
    } else {
      ok(!test.pass, test.text);
    }
  } else {
    ok(false, "test element not found: [" + test.id + "]");
  }
  run_next_test();
}

let videoTest;
while ((videoTest = videoDiscoveryTests.shift())) {
  if (!("poster" in videoTest)) {
    videoTest.poster = "";
  }
  if (!("pass" in videoTest)) {
    videoTest.pass = true;
  }

  add_test(execute_video_test.bind(this, videoTest));
}

run_next_test();
