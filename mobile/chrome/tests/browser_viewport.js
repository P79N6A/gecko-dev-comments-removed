







































let baseURI = "http://mochi.test:8888/browser/mobile/chrome/";
let testURL_blank = baseURI + "browser_blank_01.html";

function testURL(n) {
  return baseURI + "browser_viewport.sjs?" + encodeURIComponent(gTestData[n].metadata);
}
function scaleRatio(n) {
  if ("scaleRatio" in gTestData[n])
    return gTestData[n].scaleRatio;
  return 150; 
}

let working_tab;

let loadUrl = function loadUrl(tab, url, callback) {
  messageManager.addMessageListener("MozScrolledAreaChanged", function(msg) {
    if (working_tab.browser.currentURI.spec == url) {
      messageManager.removeMessageListener(msg.name, arguments.callee);
      
      
      
      setTimeout(callback, 0);
    }
  });
  BrowserUI.goToURI(url);
};

let gTestData = [
  { metadata: "", width: 980, scale: 1 },
  { metadata: "width=device-width, initial-scale=1", width: 533.33, scale: 1.5 },
  { metadata: "width=device-width", width: 533.33, scale: 1.5 },
  { metadata: "width=device-width, initial-scale=1", scaleRatio: 100, width: 800, scale: 1 },
  { metadata: "width=320, initial-scale=1", width: 533.33, scale: 1.5 },
  { metadata: "initial-scale=1.0, user-scalable=no", width: 533.33, scale: 1.5, disableZoom: true },
  { metadata: "initial-scale=1.0, user-scalable=0", width: 533.33, scale: 1.5, disableZoom: true },
  { metadata: "initial-scale=1.0, user-scalable=false", width: 533.33, scale: 1.5, disableZoom: true },
  { metadata: "initial-scale=1.0, user-scalable=NO", width: 533.33, scale: 1.5, disableZoom: false }, 
  { metadata: "width=200,height=500", width: 200, scale: 4 },
  { metadata: "width=2000, minimum-scale=0.75", width: 2000, scale: 1.125, minScale: 1.125 },
  { metadata: "width=100, maximum-scale=2.0", width: 266.67, scale: 3, maxScale: 3 },
  { metadata: "width=2000, initial-scale=0.75", width: 2000, scale: 1.125 },
  { metadata: "width=20000, initial-scale=100", width: 10000, scale: 4 },
  { metadata: "XHTML", width: 533.33, scale: 1.5, disableZoom: false },
  
  { metadata: "width= 2000, minimum-scale=0.75", width: 2000, scale: 1.125 },
  { metadata: "width = 2000, minimum-scale=0.75", width: 2000, scale: 1.125 },
  { metadata: "width = 2000 , minimum-scale=0.75", width: 2000, scale: 1.125 },
  { metadata: "width = 2000 , minimum-scale =0.75", width: 2000, scale: 1.125 },
  { metadata: "width = 2000 , minimum-scale = 0.75", width: 2000, scale: 1.125 },
  { metadata: "width =  2000   ,    minimum-scale      =       0.75", width: 2000, scale: 1.125 }
];





function test() {
  
  waitForExplicitFinish();
  requestLongerTimeout(2);

  working_tab = Browser.addTab("about:blank", true);
  ok(working_tab, "Tab Opened");

  startTest(0);
}

function startTest(n) {
  BrowserUI.goToURI(testURL_blank);
  loadUrl(working_tab, testURL_blank, verifyBlank(n));
  Services.prefs.setIntPref("browser.viewport.scaleRatio", scaleRatio(n));
}

function verifyBlank(n) {
  return function() {
    
    var uri = working_tab.browser.currentURI.spec;
    is(uri, testURL_blank, "URL Matches blank page "+n);

    
    is(working_tab.browser.contentWindowWidth, 980, "Normal 'browser' width is 980 pixels");

    loadUrl(working_tab, testURL(n), verifyTest(n));
  }
}

function is_approx(actual, expected, fuzz, description) {
  ok(Math.abs(actual - expected) <= fuzz,
     description + " [got " + actual + ", expected " + expected + "]");
}

function verifyTest(n) {
  return function() {
    is(window.innerWidth, 800, "Test assumes window width is 800px");

    
    var uri = working_tab.browser.currentURI.spec;
    is(uri, testURL(n), "URL is "+testURL(n));

    let data = gTestData[n];
    let actualWidth = working_tab.browser.contentWindowWidth;
    is_approx(actualWidth, parseFloat(data.width), .01, "Viewport width=" + data.width);

    let zoomLevel = getBrowser().scale;
    is_approx(zoomLevel, parseFloat(data.scale), .01, "Viewport scale=" + data.scale);

    
    if (data.disableZoom) {
      ok(!working_tab.allowZoom, "Zoom disabled");

      Browser.zoom(-1);
      is(getBrowser().scale, zoomLevel, "Zoom in does nothing");

      Browser.zoom(1);
      is(getBrowser().scale, zoomLevel, "Zoom out does nothing");
    }
    else {
      ok(Browser.selectedTab.allowZoom, "Zoom enabled");
    }


    if (data.minScale) {
      do { 
        zoomLevel = getBrowser().scale;
        Browser.zoom(1);
      } while (getBrowser().scale != zoomLevel);
      ok(getBrowser().scale >= data.minScale, "Zoom out limited");
    }

    if (data.maxScale) {
      do { 
        zoomLevel = getBrowser().scale;
        Browser.zoom(-1);
      } while (getBrowser().scale != zoomLevel);
      ok(getBrowser().scale <= data.maxScale, "Zoom in limited");
    }

    finishTest(n);
  }
}

function finishTest(n) {
  Services.prefs.clearUserPref("browser.viewport.scaleRatio");
  if (n+1 < gTestData.length) {
    startTest(n+1);
  } else {
    Browser.closeTab(working_tab);
    finish();
  }
}
