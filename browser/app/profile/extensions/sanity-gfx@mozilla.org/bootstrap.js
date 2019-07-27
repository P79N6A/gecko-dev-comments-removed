



const {classes: Cc, interfaces: Ci, utils: Cu} = Components;
Cu.import('resource://gre/modules/Services.jsm');
Cu.import('resource://gre/modules/Preferences.jsm');

const PAGE_WIDTH=72;
const PAGE_HEIGHT=136;
const DRIVER_PREF="sanity-test.driver-version";
const DEVICE_PREF="sanity-test.device-id";
const VERSION_PREF="sanity-test.version";
const DISABLE_VIDEO_PREF="media.hardware-video-decoding.failed";
const RUNNING_PREF="sanity-test.running";


const TEST_PASSED=0;
const TEST_FAILED_RENDER=1;
const TEST_FAILED_VIDEO=2;
const TEST_CRASHED=3;

function install() {}
function uninstall() {}

function testPixel(ctx, x, y, r, g, b, a, fuzz) {
  var data = ctx.getImageData(x, y, 1, 1);

  if (Math.abs(data.data[0] - r) <= fuzz &&
      Math.abs(data.data[1] - g) <= fuzz &&
      Math.abs(data.data[2] - b) <= fuzz &&
      Math.abs(data.data[3] - a) <= fuzz) {
    return true;
  }

  return false;
}













function testVideoRendering(ctx) {
  return testPixel(ctx, 18, 82, 255, 255, 255, 255, 64) &&
         testPixel(ctx, 50, 82, 0, 255, 0, 255, 64) &&
         testPixel(ctx, 18, 114, 0, 0, 255, 255, 64) &&
         testPixel(ctx, 50, 114, 255, 0, 0, 255, 64);
}

function hasHardwareLayers(win) {
  var winUtils = win.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
  if (winUtils.layerManagerType != "Basic") {
    return true;
  }
  return false;
}

function reportResult(val) {
  try {
    let histogram = Services.telemetry.getHistogramById("GRAPHICS_SANITY_TEST");
    histogram.add(val);
  } catch (e) {}

  Preferences.set(RUNNING_PREF, false);
  Services.prefs.savePrefFile(null);
}

function windowLoaded(event) {
  var win = event.target;

  
  var canvas = win.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
  canvas.setAttribute("width", PAGE_WIDTH);
  canvas.setAttribute("height", PAGE_HEIGHT);

  
  
  
  var ctx = canvas.getContext("2d");
  var flags = ctx.DRAWWINDOW_DRAW_CARET | ctx.DRAWWINDOW_DRAW_VIEW | ctx.DRAWWINDOW_USE_WIDGET_LAYERS;
  ctx.drawWindow(win.ownerGlobal, 0, 0, PAGE_WIDTH, PAGE_HEIGHT, "rgb(255,255,255)", flags);

  win.ownerGlobal.close();

  
  

  
  
  if (!testVideoRendering(ctx)) {
    reportResult(TEST_FAILED_VIDEO);
    if (Preferences.get("layers.hardware-video-decoding.enabled", false)) {
      Preferences.set(DISABLE_VIDEO_PREF, true);
    }
    return;
  }

  reportResult(TEST_PASSED);
}

function startup(data, reason) {
  
  
  var gfxinfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfo);

  
  
  if (Preferences.get(RUNNING_PREF, false)) {
    reportResult(TEST_CRASHED);
    return;
  }

  
  if (Preferences.get(DRIVER_PREF, "") == gfxinfo.adapterDriver &&
      Preferences.get(DEVICE_PREF, "") == gfxinfo.adapterDeviceID &&
      Preferences.get(VERSION_PREF, "") == data.version) {
    return;
  }

  
  Preferences.set(DRIVER_PREF, gfxinfo.adapterDriver);
  Preferences.set(DEVICE_PREF, gfxinfo.adapterDeviceID);
  Preferences.set(VERSION_PREF, data.version);
  Preferences.set(RUNNING_PREF, true);
  Services.prefs.savePrefFile(null);

  
  
  Preferences.set(DISABLE_VIDEO_PREF, false);

  
  var win = Services.ww.openWindow(null,
                                   "chrome://sanity-test/content/sanitytest.html",
                                   "Test Page",
                                   "width=" + PAGE_WIDTH + ",height=" + PAGE_HEIGHT + ",chrome,titlebar=0,scrollbars=0",
                                   null);
  win.onload = windowLoaded;
}

function shutdown() {}
