



"use strict";

this.EXPORTED_SYMBOLS = [
  "getTestLogger",
  "initTestLogging",
];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");

this.initTestLogging = function initTestLogging(level) {
  function LogStats() {
    this.errorsLogged = 0;
  }
  LogStats.prototype = {
    format: function format(message) {
      if (message.level == Log.Level.Error) {
        this.errorsLogged += 1;
      }

      return message.time + "\t" + message.loggerName + "\t" + message.levelDesc + "\t" +
        this.formatText(message) + "\n";
    }
  };
  LogStats.prototype.__proto__ = new Log.BasicFormatter();

  let log = Log.repository.rootLogger;
  let logStats = new LogStats();
  let appender = new Log.DumpAppender(logStats);

  if (typeof(level) == "undefined") {
    level = "Debug";
  }
  getTestLogger().level = Log.Level[level];
  Log.repository.getLogger("Services").level = Log.Level[level];

  log.level = Log.Level.Trace;
  appender.level = Log.Level.Trace;
  
  log.ownAppenders = [appender];
  log.updateAppenders();

  return logStats;
}

this.getTestLogger = function getTestLogger(component) {
  return Log.repository.getLogger("Testing");
}

