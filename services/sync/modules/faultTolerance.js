




































const Cu = Components.utils;
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");

const EXPORTED_SYMBOLS = ["FaultTolerance"];

FaultTolerance = {
  get Service() {
    if (!this._Service)
      this._Service = new FTService();
    return this._Service;
  }
};

function FTService() {
  this._log = Log4Moz.repository.getLogger("FaultTolerance");
  this._log.level = Log4Moz.Level["Debug"];
  this._appender = new FTAppender(this);
  Log4Moz.repository.rootLogger.addAppender(this._appender);
}
FTService.prototype = {
  get lastException() this._lastException,
  onMessage: function FTS_onMessage(message) {
    
    
  },

  onException: function FTS_onException(exception) {
    this._lastException = exception;
    this._log.debug(Utils.stackTrace(exception));
    return true; 
  }
};

function FTFormatter() {}
FTFormatter.prototype = {
  __proto__: new Log4Moz.Formatter(),
  format: function FTF_format(message) message
};

function FTAppender(ftService) {
  this._ftService = ftService;
  this._name = "FTAppender";
  this._formatter = new FTFormatter();
}
FTAppender.prototype = {
  __proto__: new Log4Moz.Appender(),
  doAppend: function FTA_append(message) {
    this._ftService.onMessage(message);
  }
};
