

















'use strict';

this.EXPORTED_SYMBOLS = ['PdfJsTelemetry'];

const Cu = Components.utils;
Cu.import('resource://gre/modules/Services.jsm');

this.PdfJsTelemetry = {
  onViewerIsUsed: function () {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_USED");
    histogram.add(true);
  },
  onFallback: function () {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_FALLBACK_SHOWN");
    histogram.add(true);
  },
  onDocumentSize: function (size) {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_DOCUMENT_SIZE_KB");
    histogram.add(size / 1024);
  },
  onDocumentVersion: function (versionId) {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_DOCUMENT_VERSION");
    histogram.add(versionId);
  },
  onDocumentGenerator: function (generatorId) {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_DOCUMENT_GENERATOR");
    histogram.add(generatorId);
  },
  onEmbed: function (isObject) {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_EMBED");
    histogram.add(isObject);
  },
  onFontType: function (fontTypeId) {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_FONT_TYPES");
    histogram.add(fontTypeId);
  },
  onForm: function (isAcroform) {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_FORM");
    histogram.add(isAcroform);
  },
  onPrint: function () {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_PRINT");
    histogram.add(true);
  },
  onStreamType: function (streamTypeId) {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_STREAM_TYPES");
    histogram.add(streamTypeId);
  },
  onTimeToView: function (ms) {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_TIME_TO_VIEW_MS");
    histogram.add(ms);
  }
};
