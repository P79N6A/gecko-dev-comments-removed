

















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
  onForm: function (isAcroform) {
    let histogram = Services.telemetry.getHistogramById("PDF_VIEWER_FORM");
    histogram.add(isAcroform);
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
