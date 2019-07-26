


"use strict";

this.EXPORTED_SYMBOLS = ['Log4Moz'];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

const ONE_BYTE = 1;
const ONE_KILOBYTE = 1024 * ONE_BYTE;
const ONE_MEGABYTE = 1024 * ONE_KILOBYTE;

const STREAM_SEGMENT_SIZE = 4096;
const PR_UINT32_MAX = 0xffffffff;

Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Task.jsm");

this.Log4Moz = {
  Level: {
    Fatal:  70,
    Error:  60,
    Warn:   50,
    Info:   40,
    Config: 30,
    Debug:  20,
    Trace:  10,
    All:    0,
    Desc: {
      70: "FATAL",
      60: "ERROR",
      50: "WARN",
      40: "INFO",
      30: "CONFIG",
      20: "DEBUG",
      10: "TRACE",
      0:  "ALL"
    }
  },

  get repository() {
    delete Log4Moz.repository;
    Log4Moz.repository = new LoggerRepository();
    return Log4Moz.repository;
  },
  set repository(value) {
    delete Log4Moz.repository;
    Log4Moz.repository = value;
  },

  LogMessage: LogMessage,
  Logger: Logger,
  LoggerRepository: LoggerRepository,

  Formatter: Formatter,
  BasicFormatter: BasicFormatter,

  Appender: Appender,
  DumpAppender: DumpAppender,
  ConsoleAppender: ConsoleAppender,
  StorageStreamAppender: StorageStreamAppender,

  FileAppender: FileAppender,
  BoundedFileAppender: BoundedFileAppender,

  
  
  
  enumerateInterfaces: function Log4Moz_enumerateInterfaces(aObject) {
    let interfaces = [];

    for (i in Ci) {
      try {
        aObject.QueryInterface(Ci[i]);
        interfaces.push(i);
      }
      catch(ex) {}
    }

    return interfaces;
  },

  
  
  
  enumerateProperties: function Log4Moz_enumerateProps(aObject,
                                                       aExcludeComplexTypes) {
    let properties = [];

    for (p in aObject) {
      try {
        if (aExcludeComplexTypes &&
            (typeof aObject[p] == "object" || typeof aObject[p] == "function"))
          continue;
        properties.push(p + " = " + aObject[p]);
      }
      catch(ex) {
        properties.push(p + " = " + ex);
      }
    }

    return properties;
  }
};






function LogMessage(loggerName, level, message){
  this.loggerName = loggerName;
  this.level = level;
  this.message = message;
  this.time = Date.now();
}
LogMessage.prototype = {
  get levelDesc() {
    if (this.level in Log4Moz.Level.Desc)
      return Log4Moz.Level.Desc[this.level];
    return "UNKNOWN";
  },

  toString: function LogMsg_toString(){
    return "LogMessage [" + this.time + " " + this.level + " " +
      this.message + "]";
  }
};






function Logger(name, repository) {
  if (!repository)
    repository = Log4Moz.repository;
  this._name = name;
  this.children = [];
  this.ownAppenders = [];
  this.appenders = [];
  this._repository = repository;
}
Logger.prototype = {
  get name() {
    return this._name;
  },

  _level: null,
  get level() {
    if (this._level != null)
      return this._level;
    if (this.parent)
      return this.parent.level;
    dump("log4moz warning: root logger configuration error: no level defined\n");
    return Log4Moz.Level.All;
  },
  set level(level) {
    this._level = level;
  },

  _parent: null,
  get parent() this._parent,
  set parent(parent) {
    if (this._parent == parent) {
      return;
    }
    
    if (this._parent) {
      let index = this._parent.children.indexOf(this);
      if (index != -1) {
        this._parent.children.splice(index, 1);
      }
    }
    this._parent = parent;
    parent.children.push(this);
    this.updateAppenders();
  },

  updateAppenders: function updateAppenders() {
    if (this._parent) {
      let notOwnAppenders = this._parent.appenders.filter(function(appender) {
        return this.ownAppenders.indexOf(appender) == -1;
      }, this);
      this.appenders = notOwnAppenders.concat(this.ownAppenders);
    } else {
      this.appenders = this.ownAppenders.slice();
    }

    
    for (let i = 0; i < this.children.length; i++) {
      this.children[i].updateAppenders();
    }
  },

  addAppender: function Logger_addAppender(appender) {
    if (this.ownAppenders.indexOf(appender) != -1) {
      return;
    }
    this.ownAppenders.push(appender);
    this.updateAppenders();
  },

  removeAppender: function Logger_removeAppender(appender) {
    let index = this.ownAppenders.indexOf(appender);
    if (index == -1) {
      return;
    }
    this.ownAppenders.splice(index, 1);
    this.updateAppenders();
  },

  log: function Logger_log(level, string) {
    if (this.level > level)
      return;

    
    
    let message;
    let appenders = this.appenders;
    for (let appender of appenders) {
      if (appender.level > level) {
        continue;
      }
      if (!message) {
        message = new LogMessage(this._name, level, string);
      }
      appender.append(message);
    }
  },

  fatal: function Logger_fatal(string) {
    this.log(Log4Moz.Level.Fatal, string);
  },
  error: function Logger_error(string) {
    this.log(Log4Moz.Level.Error, string);
  },
  warn: function Logger_warn(string) {
    this.log(Log4Moz.Level.Warn, string);
  },
  info: function Logger_info(string) {
    this.log(Log4Moz.Level.Info, string);
  },
  config: function Logger_config(string) {
    this.log(Log4Moz.Level.Config, string);
  },
  debug: function Logger_debug(string) {
    this.log(Log4Moz.Level.Debug, string);
  },
  trace: function Logger_trace(string) {
    this.log(Log4Moz.Level.Trace, string);
  }
};






function LoggerRepository() {}
LoggerRepository.prototype = {
  _loggers: {},

  _rootLogger: null,
  get rootLogger() {
    if (!this._rootLogger) {
      this._rootLogger = new Logger("root", this);
      this._rootLogger.level = Log4Moz.Level.All;
    }
    return this._rootLogger;
  },
  set rootLogger(logger) {
    throw "Cannot change the root logger";
  },

  _updateParents: function LogRep__updateParents(name) {
    let pieces = name.split('.');
    let cur, parent;

    
    
    
    for (let i = 0; i < pieces.length - 1; i++) {
      if (cur)
        cur += '.' + pieces[i];
      else
        cur = pieces[i];
      if (cur in this._loggers)
        parent = cur;
    }

    
    if (!parent)
      this._loggers[name].parent = this.rootLogger;
    else
      this._loggers[name].parent = this._loggers[parent];

    
    for (let logger in this._loggers) {
      if (logger != name && logger.indexOf(name) == 0)
        this._updateParents(logger);
    }
  },

  getLogger: function LogRep_getLogger(name) {
    if (name in this._loggers)
      return this._loggers[name];
    this._loggers[name] = new Logger(name, this);
    this._updateParents(name);
    return this._loggers[name];
  }
};








function Formatter() {}
Formatter.prototype = {
  format: function Formatter_format(message) {}
};


function BasicFormatter(dateFormat) {
  if (dateFormat)
    this.dateFormat = dateFormat;
}
BasicFormatter.prototype = {
  __proto__: Formatter.prototype,

  format: function BF_format(message) {
    return message.time + "\t" +
      message.loggerName + "\t" +
      message.levelDesc + "\t" +
      message.message + "\n";
  }
};







function Appender(formatter) {
  this._name = "Appender";
  this._formatter = formatter? formatter : new BasicFormatter();
}
Appender.prototype = {
  level: Log4Moz.Level.All,

  append: function App_append(message) {
    if (message) {
      this.doAppend(this._formatter.format(message));
    }
  },
  toString: function App_toString() {
    return this._name + " [level=" + this._level +
      ", formatter=" + this._formatter + "]";
  },
  doAppend: function App_doAppend(message) {}
};






function DumpAppender(formatter) {
  this._name = "DumpAppender";
  Appender.call(this, formatter);
}
DumpAppender.prototype = {
  __proto__: Appender.prototype,

  doAppend: function DApp_doAppend(message) {
    dump(message);
  }
};






function ConsoleAppender(formatter) {
  this._name = "ConsoleAppender";
  Appender.call(this, formatter);
}
ConsoleAppender.prototype = {
  __proto__: Appender.prototype,

  doAppend: function CApp_doAppend(message) {
    if (message.level > Log4Moz.Level.Warn) {
      Cu.reportError(message);
      return;
    }
    Cc["@mozilla.org/consoleservice;1"].
      getService(Ci.nsIConsoleService).logStringMessage(message);
  }
};









function StorageStreamAppender(formatter) {
  this._name = "StorageStreamAppender";
  Appender.call(this, formatter);
}

StorageStreamAppender.prototype = {
  __proto__: Appender.prototype,

  _converterStream: null, 
  _outputStream: null,    

  _ss: null,

  get outputStream() {
    if (!this._outputStream) {
      
      this._outputStream = this.newOutputStream();
      if (!this._outputStream) {
        return null;
      }

      
      
      if (!this._converterStream) {
        this._converterStream = Cc["@mozilla.org/intl/converter-output-stream;1"]
                                  .createInstance(Ci.nsIConverterOutputStream);
      }
      this._converterStream.init(
        this._outputStream, "UTF-8", STREAM_SEGMENT_SIZE,
        Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);
    }
    return this._converterStream;
  },

  newOutputStream: function newOutputStream() {
    let ss = this._ss = Cc["@mozilla.org/storagestream;1"]
                          .createInstance(Ci.nsIStorageStream);
    ss.init(STREAM_SEGMENT_SIZE, PR_UINT32_MAX, null);
    return ss.getOutputStream(0);
  },

  getInputStream: function getInputStream() {
    if (!this._ss) {
      return null;
    }
    return this._ss.newInputStream(0);
  },

  reset: function reset() {
    if (!this._outputStream) {
      return;
    }
    this.outputStream.close();
    this._outputStream = null;
    this._ss = null;
  },

  doAppend: function (message) {
    if (!message) {
      return;
    }
    try {
      this.outputStream.writeString(message);
    } catch(ex) {
      if (ex.result == Cr.NS_BASE_STREAM_CLOSED) {
        
        
        this._outputStream = null;
      } try {
          this.outputStream.writeString(message);
      } catch (ex) {
        
      }
    }
  }
};






function FileAppender(path, formatter) {
  this._name = "FileAppender";
  this._encoder = new TextEncoder();
  this._path = path;
  this._file = null;
  this._fileReadyPromise = null;

  
  this._lastWritePromise = null;
  Appender.call(this, formatter);
}

FileAppender.prototype = {
  __proto__: Appender.prototype,

  _openFile: function () {
    return Task.spawn(function _openFile() {
      try {
        this._file = yield OS.File.open(this._path,
                                        {truncate: true});
      } catch (err) {
        if (err instanceof OS.File.Error) {
          this._file = null;
        } else {
          throw err;
        }
      }
    }.bind(this));
  },

  _getFile: function() {
    if (!this._fileReadyPromise) {
      this._fileReadyPromise = this._openFile();
      return this._fileReadyPromise;
    }

    return this._fileReadyPromise.then(_ => {
      if (!this._file) {
        return this._openFile();
      }
    });
  },

  doAppend: function (message) {
    let array = this._encoder.encode(message);
    if (this._file) {
      this._lastWritePromise = this._file.write(array);
    } else {
      this._lastWritePromise = this._getFile().then(_ => {
        this._fileReadyPromise = null;
        if (this._file) {
          return this._file.write(array);
        }
      });
    }
  },

  reset: function () {
    let fileClosePromise = this._file.close();
    return fileClosePromise.then(_ => {
      this._file = null;
      return OS.File.remove(this._path);
    });
  }
};








function BoundedFileAppender(path, formatter, maxSize=2*ONE_MEGABYTE) {
  this._name = "BoundedFileAppender";
  this._size = 0;
  this._maxSize = maxSize;
  this._closeFilePromise = null;
  FileAppender.call(this, path, formatter);
}

BoundedFileAppender.prototype = {
  __proto__: FileAppender.prototype,

  doAppend: function (message) {
    if (!this._removeFilePromise) {
      if (this._size < this._maxSize) {
        this._size += message.length;
        return FileAppender.prototype.doAppend.call(this, message);
      }
      this._removeFilePromise = this.reset();
    }
    this._removeFilePromise.then(_ => {
      this._removeFilePromise = null;
      this.doAppend(message);
    });
  },

  reset: function () {
    let fileClosePromise;
    if (this._fileReadyPromise) {
      
      fileClosePromise = this._fileReadyPromise.then(_ => {
        return this._file.close();
      });
    } else {
      fileClosePromise = this._file.close();
    }

    return fileClosePromise.then(_ => {
      this._size = 0;
      this._file = null;
      return OS.File.remove(this._path);
    });
  }
};

