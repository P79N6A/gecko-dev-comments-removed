


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "promise");

loader.lazyImporter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");
loader.lazyImporter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");





const PERF_TOOL_SERIALIZER_IDENTIFIER = "Recorded Performance Data";
const PERF_TOOL_SERIALIZER_VERSION = 1;




let PerformanceIO = {
  



  getUnicodeConverter: function() {
    let className = "@mozilla.org/intl/scriptableunicodeconverter";
    let converter = Cc[className].createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    return converter;
  },

  











  saveRecordingToFile: function(recordingData, file) {
    let deferred = promise.defer();

    recordingData.fileType = PERF_TOOL_SERIALIZER_IDENTIFIER;
    recordingData.version = PERF_TOOL_SERIALIZER_VERSION;

    let string = JSON.stringify(recordingData);
    let inputStream = this.getUnicodeConverter().convertToInputStream(string);
    let outputStream = FileUtils.openSafeFileOutputStream(file);

    NetUtil.asyncCopy(inputStream, outputStream, deferred.resolve);
    return deferred.promise;
  },

  








  loadRecordingFromFile: function(file) {
    let deferred = promise.defer();

    let channel = NetUtil.newChannel(file);
    channel.contentType = "text/plain";

    NetUtil.asyncFetch(channel, (inputStream, status) => {
      try {
        let string = NetUtil.readInputStreamToString(inputStream, inputStream.available());
        var recordingData = JSON.parse(string);
      } catch (e) {
        deferred.reject(new Error("Could not read recording data file."));
        return;
      }
      if (recordingData.fileType != PERF_TOOL_SERIALIZER_IDENTIFIER) {
        deferred.reject(new Error("Unrecognized recording data file."));
        return;
      }
      if (recordingData.version != PERF_TOOL_SERIALIZER_VERSION) {
        deferred.reject(new Error("Unsupported recording data file version."));
        return;
      }
      deferred.resolve(recordingData);
    });

    return deferred.promise;
  }
};

exports.PerformanceIO = PerformanceIO;
