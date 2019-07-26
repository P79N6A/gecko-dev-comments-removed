const Cu = Components.utils;
Cu.import("resource:///modules/devtools/ProfilerHelpers.jsm");




var gStrings = {
  
  
  "Complete Profile": L10N.getStr("profiler.completeProfile"),
  "Sample Range": L10N.getStr("profiler.sampleRange"),
  "Running Time": L10N.getStr("profiler.runningTime"),
  "Self": L10N.getStr("profiler.self"),
  "Symbol Name": L10N.getStr("profiler.symbolName"),

  getStr: function (name) {
    return L10N.getStr(name);
  },

  getFormatStr: function (name, params) {
    return L10N.getFormatStr(name, params);
  }
};