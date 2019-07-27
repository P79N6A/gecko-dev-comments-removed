















'use strict';

this.EXPORTED_SYMBOLS = ['ShumwayTelemetry'];

const Cu = Components.utils;
Cu.import('resource://gre/modules/Services.jsm');

const BANNER_SIZES = [
  "88x31", "120x60", "120x90", "120x240", "120x600", "125x125", "160x600",
  "180x150", "234x60", "240x400", "250x250", "300x100", "300x250", "300x600",
  "300x1050", "336x280", "468x60", "550x480", "720x100", "728x90", "970x90",
  "970x250"];

function getBannerType(width, height) {
  return BANNER_SIZES.indexOf(width + 'x' + height) + 1;
}

this.ShumwayTelemetry = {
  onFirstFrame: function (timeToDisplay) {
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_TIME_TO_VIEW_MS");
    histogram.add(timeToDisplay);
  },
  onParseInfo: function (parseInfo) {
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_PARSING_MS");
    histogram.add(parseInfo.parseTime);
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_SWF_SIZE_KB");
    histogram.add(parseInfo.size / 1024);
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_SWF_VERSION");
    histogram.add(parseInfo.swfVersion);
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_SWF_FRAME_RATE");
    histogram.add(parseInfo.frameRate);
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_SWF_AREA");
    histogram.add(parseInfo.width * parseInfo.height);
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_SWF_BANNER");
    histogram.add(getBannerType(parseInfo.width, parseInfo.height));
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_SWF_AVM2");
    histogram.add(parseInfo.isAvm2);
  },
  onError: function (errorType) {
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_ERROR");
    histogram.add(errorType);
  },
  onPageIndex: function (pageIndex) {
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_SWF_INDEX_ON_PAGE");
    histogram.add(pageIndex);
  },
  onFeature: function (featureType) {
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_FEATURE_USED");
    histogram.add(featureType);
  },
  onFallback: function (userAction) {
    var histogram = Services.telemetry.getHistogramById("SHUMWAY_FALLBACK");
    histogram.add(userAction);
  }
};
