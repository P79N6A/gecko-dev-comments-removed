







































let testURL_blank = chromeRoot + "browser_blank_01.html";
let testURL = function testURL(n) {
  return chromeRoot + "browser_viewport_" +
         (n<10 ? "0" : "") + n + ".html";
}

let working_tab;
function pageLoaded(url) {
  dump("------- pageLoaded: " + url + "\n")
  return function() {
    dump("------- waiting for pageLoaded: " + working_tab.browser.currentURI.spec + "\n")
    return !working_tab.isLoading() && working_tab.browser.currentURI.spec == url;
  }
}

let testData = [
  { width: 980,     scale: 800/980 },
  { width: 533.33,  scale: 1.5 },
  { width: 533.33,  scale: 1.5 },
  { width: 533.33,  scale: 1.5,    disableZoom: true },
  { width: 200,     scale: 4.00 },
  { width: 2000,    scale: 1.125,  minScale: 1.125 },
  { width: 266.67,  scale: 3,      maxScale: 3 },
  { width: 2000,    scale: 1.125 },
  { width: 10000,   scale: 4 },
  { width: 533.33,  scale: 1.5,    disableZoom: true }
];

let isLocalScheme = Util.isLocalScheme;



function test() {
  
  waitForExplicitFinish();

  
  
  Util.isLocalScheme = function() { return false; };

  working_tab = Browser.addTab(testURL_blank, true);
  ok(working_tab, "Tab Opened");

  waitFor(function() { startTest(0); }, pageLoaded(testURL_blank));
}

function startTest(n) {
  BrowserUI.goToURI(testURL_blank);
  waitFor(verifyBlank(n), pageLoaded(testURL_blank));
}

function verifyBlank(n) {
  return function() {
    
    var uri = working_tab.browser.currentURI.spec;
    is(uri, testURL_blank, "URL Matches blank page "+n);

    
    is(working_tab.browser.contentWindowWidth, 980, "Normal 'browser' width is 980 pixels");

    loadTest(n);
  }
}

function loadTest(n) {
  let url = testURL(n);
  BrowserUI.goToURI(url);
  waitFor(function() {
    
    
    
    
    
    
    
    
    
    
    setTimeout(verifyTest(n), 0);
  }, pageLoaded(url));
}

function is_approx(actual, expected, fuzz, description) {
  is(Math.abs(actual - expected) <= fuzz,
     true,
     description + " [got " + actual + ", expected " + expected + "]");
}

function verifyTest(n) {
  return function() {
    is(window.innerWidth, 800, "Test assumes window width is 800px");
    is(Services.prefs.getIntPref("zoom.dpiScale") / 100, 1.5, "Test assumes zoom.dpiScale is 1.5");

    
    var uri = working_tab.browser.currentURI.spec;
    is(uri, testURL(n), "URL is "+testURL(n));

    let data = testData[n];
    let actualWidth = working_tab.browser.contentWindowWidth;
    is_approx(actualWidth, parseFloat(data.width), .01, "Viewport width=" + data.width);

    let zoomLevel = getBrowser().scale;
    is_approx(zoomLevel, parseFloat(data.scale), .01, "Viewport scale=" + data.scale);

    
    if (data.disableZoom) {
      ok(!Browser.selectedTab.allowZoom, "Zoom disabled");

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
  if (n+1 < testData.length) {
    startTest(n+1);
  } else {
    Browser.closeTab(working_tab);
    Util.isLocalScheme = isLocalScheme;
    finish();
  }
}
