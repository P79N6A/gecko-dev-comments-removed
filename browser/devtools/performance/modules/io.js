


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

loader.lazyRequireGetter(this, "Services");
loader.lazyRequireGetter(this, "promise");

loader.lazyImporter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");
loader.lazyImporter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");





const PERF_TOOL_SERIALIZER_IDENTIFIER = "Recorded Performance Data";
const PERF_TOOL_SERIALIZER_LEGACY_VERSION = 1;
const PERF_TOOL_SERIALIZER_CURRENT_VERSION = 2;




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
    recordingData.version = PERF_TOOL_SERIALIZER_CURRENT_VERSION;

    let string = JSON.stringify(recordingData);
    let inputStream = this.getUnicodeConverter().convertToInputStream(string);
    let outputStream = FileUtils.openSafeFileOutputStream(file);

    NetUtil.asyncCopy(inputStream, outputStream, deferred.resolve);
    return deferred.promise;
  },

  








  loadRecordingFromFile: function(file) {
    let deferred = promise.defer();

    let channel = NetUtil.newChannel2(file,
                                      null,
                                      null,
                                      null,      
                                      Services.scriptSecurityManager.getSystemPrincipal(),
                                      null,      
                                      Ci.nsILoadInfo.SEC_NORMAL,
                                      Ci.nsIContentPolicy.TYPE_OTHER);

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
      if (!isValidSerializerVersion(recordingData.version)) {
        deferred.reject(new Error("Unsupported recording data file version."));
        return;
      }
      if (recordingData.version === PERF_TOOL_SERIALIZER_LEGACY_VERSION) {
        recordingData = convertLegacyData(recordingData);
      }
      deferred.resolve(recordingData);
    });

    return deferred.promise;
  }
};

exports.PerformanceIO = PerformanceIO;








function isValidSerializerVersion (version) {
  return !!~[
    PERF_TOOL_SERIALIZER_LEGACY_VERSION,
    PERF_TOOL_SERIALIZER_CURRENT_VERSION
  ].indexOf(version);
}









function convertLegacyData (legacyData) {
  let { profilerData, ticksData, recordingDuration } = legacyData;

  
  
  let data = {
    label: profilerData.profilerLabel,
    duration: recordingDuration,
    markers: [],
    frames: [],
    memory: [],
    ticks: ticksData,
    allocations: { sites: [], timestamps: [], frames: [], counts: [] },
    profile: profilerData.profile,
    
    
    configuration: {
      withTicks: !!ticksData.length,
      withMarkers: false,
      withMemory: false,
      withAllocations: false
    }
  };

  return data;
}
