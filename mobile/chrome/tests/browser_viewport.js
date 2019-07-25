








































let testURL_blank = "chrome://mochikit/content/browser/mobile/chrome/browser_blank_01.html";
let testURL = function testURL(n) {
  return "chrome://mochikit/content/browser/mobile/chrome/browser_viewport_" +
         (n<10 ? "0" : "") + n + ".html";
}

let working_tab;
function pageLoaded(url) {
  return function() {
    return !working_tab.isLoading() && working_tab.browser.currentURI.spec == url;
  }
}

let numberTests = 10;



function test() {
  
  waitForExplicitFinish();

  working_tab = Browser.addTab("", true);
  ok(working_tab, "Tab Opened");
  startTest(0);
}

function startTest(n) {
  BrowserUI.goToURI(testURL_blank);
  waitFor(verifyBlank(n), pageLoaded(testURL_blank));
}

function verifyBlank(n) {
  return function() {
    
    var uri = working_tab.browser.currentURI.spec;
    is(uri, testURL_blank, "URL Matches blank page "+n);

    
    let style = window.getComputedStyle(working_tab.browser, null);
    is(style.width, "800px", "Normal 'browser' width is 800 pixels");

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

function getMetaTag(name) {
  let doc = working_tab.browser.contentDocument;
  let elements = doc.querySelectorAll("meta[name=\"" + name + "\"]");
  return elements[0] ? elements[0].getAttribute("content") : undefined;
}

function verifyTest(n) {
  return function() {
    is(window.innerWidth, 800, "Test assumes window width is 800px");
    is(gPrefService.getIntPref("zoom.dpiScale") / 100, 1.5, "Test assumes zoom.dpiScale is 1.5");

    
    var uri = working_tab.browser.currentURI.spec;
    is(uri, testURL(n), "URL is "+testURL(n));

    
    let data = (function() {
      let result = {};
      for (let i = 0; i < arguments.length; i++)
        result[arguments[i]] = getMetaTag("expected-" + arguments[i]);

      return result;
    })("width", "scale", "minWidth", "maxWidth", "disableZoom");

    let style = window.getComputedStyle(working_tab.browser, null);
    let actualWidth = parseFloat(style.width.replace(/[^\d\.]+/, ""));
    is_approx(actualWidth, parseFloat(data.width), .01, "Viewport width="+data.width);

    let bv = Browser._browserView;
    let zoomLevel = bv.getZoomLevel();
    is_approx(bv.getZoomLevel(), parseFloat(data.scale), .01, "Viewport scale="+data.scale);

    
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
  if (n+1 < numberTests) {
    startTest(n+1);
  } else {
    Browser.closeTab(working_tab);
    finish();
  }
}
