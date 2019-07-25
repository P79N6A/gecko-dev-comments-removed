








































let testURL_blank = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_01.html";
let testURL = function testURL(n) {
  return "chrome://mochikit/content/browser/mobile/chrome/browser_viewport_" +
         (n<10 ? "0" : "") + n + ".html";
}
let deviceWidth = {};

let working_tab;
let isLoading = function() { return !working_tab.isLoading(); };

let testData = [
  { width: undefined,   scale: undefined },
  { width: deviceWidth, scale: 1 },
  { width: 320,         scale: 1 },
  { width: undefined,   scale: 1,        disableZoom: true },
  { width: 200,         scale: undefined },
  { width: 2000,        minScale: 0.75 },
  { width: 100,         maxScale: 2 },
  { width: 2000,        scale: 0.75 }
];



function test() {
  
  waitForExplicitFinish();

  working_tab = Browser.addTab("", true);
  ok(working_tab, "Tab Opened");
  startTest(0);
}

function startTest(n) {
  BrowserUI.goToURI(testURL_blank);
  waitFor(verifyBlank(n), isLoading);
}

function verifyBlank(n) {
  return function() {
    
    var uri = working_tab.browser.currentURI.spec;
    is(uri, testURL_blank, "URL Matches blank page "+n);

    
    ok(working_tab.browser.classList.contains("browser"), "Normal 'browser' class");
    let style = window.getComputedStyle(working_tab.browser, null);
    is(style.width, "800px", "Normal 'browser' width is 800 pixels");

    loadTest(n);
  }
}

function loadTest(n) {
  BrowserUI.goToURI(testURL(n));
  waitFor(verifyTest(n), isLoading);
}

function verifyTest(n) {
  return function() {
    let data = testData[n];

    
    var uri = working_tab.browser.currentURI.spec;
    is(uri, testURL(n), "URL Matches newly created Tab "+n);

    
    let width = data.width || (data.scale ? window.innerWidth : 800);
    if (width == deviceWidth) {
      width = window.innerWidth;
      ok(working_tab.browser.classList.contains("browser-viewport"), "Viewport 'browser-viewport' class");
    }

    let scale = data.scale || window.innerWidth / width;
    let minScale = data.minScale || 0.2;
    let maxScale = data.maxScale || 4;

    scale = Math.min(scale, maxScale);
    scale = Math.max(scale, minScale);

    
    if (0.9 < scale && scale < 1)
      scale = 1;

    
    if (width * scale < window.innerWidth)
      width = window.innerWidth / scale;

    let style = window.getComputedStyle(working_tab.browser, null);
    is(style.width, width + "px", "Viewport width="+width);

    let bv = Browser._browserView;
    let zoomLevel = bv.getZoomLevel();
    is(bv.getZoomLevel(), scale, "Viewport scale="+scale);

    
    if (data.disableZoom) {
      ok(!bv.allowZoom, "Zoom disabled");

      Browser.zoom(-1);
      is(bv.getZoomLevel(), zoomLevel, "Zoom in does nothing");

      Browser.zoom(1);
      is(bv.getZoomLevel(), zoomLevel, "Zoom out does nothing");
    }
    else {
      ok(bv.allowZoom, "Zoom enabled");
    }


    if (data.minScale) {
      do { 
        zoomLevel = bv.getZoomLevel();
        Browser.zoom(1);
      } while (bv.getZoomLevel() != zoomLevel);
      ok(bv.getZoomLevel() >= data.minScale, "Zoom out limited");
    }

    if (data.maxScale) {
      do { 
        zoomLevel = bv.getZoomLevel();
        Browser.zoom(-1);
      } while (bv.getZoomLevel() != zoomLevel);
      ok(bv.getZoomLevel() <= data.maxScale, "Zoom in limited");
    }

    finishTest(n);
  }
}

function finishTest(n) {
  if (n+1 < testData.length) {
    startTest(n+1);
  } else {
    Browser.closeTab(working_tab);
    finish();
  }
}
