



"use strict";

this.EXPORTED_SYMBOLS = ["Log"];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

const ONE_BYTE = 1;
const ONE_KILOBYTE = 1024 * ONE_BYTE;
const ONE_MEGABYTE = 1024 * ONE_KILOBYTE;

const STREAM_SEGMENT_SIZE = 4096;
const PR_UINT32_MAX = 0xffffffff;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
const INTERNAL_FIELDS = new Set(["_level", "_message", "_time", "_namespace"]);





function dumpError(text) {
  dump(text + "\n");
  Cu.reportError(text);
}

this.Log = {
  Level: {
    Fatal:  70,
    Error:  60,
    Warn:   50,
    Info:   40,
    Config: 30,
    Debug:  20,
    Trace:  10,
    All:    -1, 
    Desc: {
      70: "FATAL",
      60: "ERROR",
      50: "WARN",
      40: "INFO",
      30: "CONFIG",
      20: "DEBUG",
      10: "TRACE",
      "-1":  "ALL",
    },
    Numbers: {
      "FATAL": 70,
      "ERROR": 60,
      "WARN": 50,
      "INFO": 40,
      "CONFIG": 30,
      "DEBUG": 20,
      "TRACE": 10,
      "ALL": -1,
    }
  },

  get repository() {
    delete Log.repository;
    Log.repository = new LoggerRepository();
    return Log.repository;
  },
  set repository(value) {
    delete Log.repository;
    Log.repository = value;
  },

  LogMessage: LogMessage,
  Logger: Logger,
  LoggerRepository: LoggerRepository,

  Formatter: Formatter,
  BasicFormatter: BasicFormatter,
  MessageOnlyFormatter: MessageOnlyFormatter,
  StructuredFormatter: StructuredFormatter,

  Appender: Appender,
  DumpAppender: DumpAppender,
  ConsoleAppender: ConsoleAppender,
  StorageStreamAppender: StorageStreamAppender,

  FileAppender: FileAppender,
  BoundedFileAppender: BoundedFileAppender,

  ParameterFormatter: ParameterFormatter,
  
  
  
  enumerateInterfaces: function Log_enumerateInterfaces(aObject) {
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

  
  
  
  enumerateProperties: function (aObject, aExcludeComplexTypes) {
    let properties = [];

    for (p in aObject) {
      try {
        if (aExcludeComplexTypes &&
            (typeof(aObject[p]) == "object" || typeof(aObject[p]) == "function"))
          continue;
        properties.push(p + " = " + aObject[p]);
      }
      catch(ex) {
        properties.push(p + " = " + ex);
      }
    }

    return properties;
  },

  _formatError: function _formatError(e) {
    let result = e.toString();
    if (e.fileName) {
      result +=  " (" + e.fileName;
      if (e.lineNumber) {
        result += ":" + e.lineNumber;
      }
      if (e.columnNumber) {
        result += ":" + e.columnNumber;
      }
      result += ")";
    }
    return result + " " + Log.stackTrace(e);
  },

  
  
  exceptionStr: function exceptionStr(e) {
    if (!e) {
      return "" + e;
    }
    if (e instanceof Ci.nsIException) {
      return e.toString() + " " + Log.stackTrace(e);
    }
    else if (isError(e)) {
      return Log._formatError(e);
    }
    
    let message = e.message ? e.message : e;
    return message + " " + Log.stackTrace(e);
  },

  stackTrace: function stackTrace(e) {
    
    if (e.location) {
      let frame = e.location;
      let output = [];
      while (frame) {
        
        
        
        let str = "<file:unknown>";

        let file = frame.filename || frame.fileName;
        if (file) {
          str = file.replace(/^(?:chrome|file):.*?([^\/\.]+\.\w+)$/, "$1");
        }

        if (frame.lineNumber) {
          str += ":" + frame.lineNumber;
        }

        if (frame.name) {
          str = frame.name + "()@" + str;
        }

        if (str) {
          output.push(str);
        }
        frame = frame.caller;
      }
      return "Stack trace: " + output.join(" < ");
    }
    
    if (e.stack) {
      return "JS Stack trace: " + Task.Debugging.generateReadableStack(e.stack).trim()
        .replace(/\n/g, " < ").replace(/@[^@]*?([^\/\.]+\.\w+:)/g, "@$1");
    }

    return "No traceback available";
  }
};





function LogMessage(loggerName, level, message, params) {
  this.loggerName = loggerName;
  this.level = level;
  





  if (!params && message && (typeof(message) == "object") &&
      (typeof(message.valueOf()) != "string")) {
    this.message = null;
    this.params = message;
  } else {
    
    this.message = message;
    this.params = params;
  }

  
  
  this._structured = this.params && this.params.action;
  this.time = Date.now();
}
LogMessage.prototype = {
  get levelDesc() {
    if (this.level in Log.Level.Desc)
      return Log.Level.Desc[this.level];
    return "UNKNOWN";
  },

  toString: function LogMsg_toString() {
    let msg = "LogMessage [" + this.time + " " + this.level + " " +
      this.message;
    if (this.params) {
      msg += " " + JSON.stringify(this.params);
    }
    return msg + "]"
  }
};






function Logger(name, repository) {
  if (!repository)
    repository = Log.repository;
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
    dumpError("Log warning: root logger configuration error: no level defined");
    return Log.Level.All;
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

  












  logStructured: function (action, params) {
    if (!action) {
      throw "An action is required when logging a structured message.";
    }
    if (!params) {
      return this.log(this.level, undefined, {"action": action});
    }
    if (typeof(params) != "object") {
      throw "The params argument is required to be an object.";
    }

    let level = params._level;
    if (level) {
      let ulevel = level.toUpperCase();
      if (ulevel in Log.Level.Numbers) {
        level = Log.Level.Numbers[ulevel];
      }
    } else {
      level = this.level;
    }

    params.action = action;
    this.log(level, params._message, params);
  },

  log: function (level, string, params) {
    if (this.level > level)
      return;

    
    
    let message;
    let appenders = this.appenders;
    for (let appender of appenders) {
      if (appender.level > level) {
        continue;
      }
      if (!message) {
        message = new LogMessage(this._name, level, string, params);
      }
      appender.append(message);
    }
  },

  fatal: function (string, params) {
    this.log(Log.Level.Fatal, string, params);
  },
  error: function (string, params) {
    this.log(Log.Level.Error, string, params);
  },
  warn: function (string, params) {
    this.log(Log.Level.Warn, string, params);
  },
  info: function (string, params) {
    this.log(Log.Level.Info, string, params);
  },
  config: function (string, params) {
    this.log(Log.Level.Config, string, params);
  },
  debug: function (string, params) {
    this.log(Log.Level.Debug, string, params);
  },
  trace: function (string, params) {
    this.log(Log.Level.Trace, string, params);
  }
};






function LoggerRepository() {}
LoggerRepository.prototype = {
  _loggers: {},

  _rootLogger: null,
  get rootLogger() {
    if (!this._rootLogger) {
      this._rootLogger = new Logger("root", this);
      this._rootLogger.level = Log.Level.All;
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

  








  getLogger: function (name) {
    if (name in this._loggers)
      return this._loggers[name];
    this._loggers[name] = new Logger(name, this);
    this._updateParents(name);
    return this._loggers[name];
  },

  
















  getLoggerWithMessagePrefix: function (name, prefix) {
    let log = this.getLogger(name);

    let proxy = Object.create(log);
    proxy.log = (level, string, params) => log.log(level, prefix + string, params);
    return proxy;
  },
};








function Formatter() {}
Formatter.prototype = {
  format: function Formatter_format(message) {}
};


function BasicFormatter(dateFormat) {
  if (dateFormat) {
    this.dateFormat = dateFormat;
  }
  this.parameterFormatter = new ParameterFormatter();
}
BasicFormatter.prototype = {
  __proto__: Formatter.prototype,

  







  formatText: function (message) {
    let params = message.params;
    if (typeof(params) == "undefined") {
      return message.message || "";
    }
    
    
    let pIsObject = (typeof(params) == 'object' || typeof(params) == 'function');

    
    if (this.parameterFormatter) {
      
      
      let subDone = false;
      let regex = /\$\{(\S*)\}/g;
      let textParts = [];
      if (message.message) {
        textParts.push(message.message.replace(regex, (_, sub) => {
          
          if (sub) {
            if (pIsObject && sub in message.params) {
              subDone = true;
              return this.parameterFormatter.format(message.params[sub]);
            }
            return '${' + sub + '}';
          }
          
          subDone = true;
          return this.parameterFormatter.format(message.params);
        }));
      }
      if (!subDone) {
        
        let rest = this.parameterFormatter.format(message.params);
        if (rest !== null && rest != "{}") {
          textParts.push(rest);
        }
      }
      return textParts.join(': ');
    }
  },

  format: function BF_format(message) {
    return message.time + "\t" +
      message.loggerName + "\t" +
      message.levelDesc + "\t" +
      this.formatText(message);
  }
};




function MessageOnlyFormatter() {
}
MessageOnlyFormatter.prototype = Object.freeze({
  __proto__: Formatter.prototype,

  format: function (message) {
    return message.message;
  },
});




function StructuredFormatter() { }
StructuredFormatter.prototype = {
  __proto__: Formatter.prototype,

  format: function (logMessage) {
    let output = {
      _time: logMessage.time,
      _namespace: logMessage.loggerName,
      _level: logMessage.levelDesc
    };

    for (let key in logMessage.params) {
      output[key] = logMessage.params[key];
    }

    if (!output.action) {
      output.action = "UNKNOWN";
    }

    if (!output._message && logMessage.message) {
      output._message = logMessage.message;
    }

    return JSON.stringify(output);
  }
}




function isError(aObj) {
  return (aObj && typeof(aObj) == 'object' && "name" in aObj && "message" in aObj &&
          "fileName" in aObj && "lineNumber" in aObj && "stack" in aObj);
};







function ParameterFormatter() {
  this._name = "ParameterFormatter"
}
ParameterFormatter.prototype = {
  format: function(ob) {
    try {
      if (ob === undefined) {
        return "undefined";
      }
      if (ob === null) {
        return "null";
      }
      
      if ((typeof(ob) != "object" || typeof(ob.valueOf()) != "object") &&
          typeof(ob) != "function") {
        return ob;
      }
      if (ob instanceof Ci.nsIException) {
        return ob.toString() + " " + Log.stackTrace(ob);
      }
      else if (isError(ob)) {
        return Log._formatError(ob);
      }
      
      
      return JSON.stringify(ob, (key, val) => {
        if (INTERNAL_FIELDS.has(key)) {
          return undefined;
        }
        return val;
      });
    }
    catch (e) {
      dumpError("Exception trying to format object for log message: " + Log.exceptionStr(e));
    }
    
    try {
      return ob.toSource();
    } catch (_) { }
    try {
      return "" + ob;
    } catch (_) {
      return "[object]"
    }
  }
}







function Appender(formatter) {
  this._name = "Appender";
  this._formatter = formatter? formatter : new BasicFormatter();
}
Appender.prototype = {
  level: Log.Level.All,

  append: function App_append(message) {
    if (message) {
      this.doAppend(this._formatter.format(message));
    }
  },
  toString: function App_toString() {
    return this._name + " [level=" + this.level +
      ", formatter=" + this._formatter + "]";
  },
  doAppend: function App_doAppend(formatted) {}
};






function DumpAppender(formatter) {
  Appender.call(this, formatter);
  this._name = "DumpAppender";
}
DumpAppender.prototype = {
  __proto__: Appender.prototype,

  doAppend: function DApp_doAppend(formatted) {
    dump(formatted + "\n");
  }
};






function ConsoleAppender(formatter) {
  Appender.call(this, formatter);
  this._name = "ConsoleAppender";
}
ConsoleAppender.prototype = {
  __proto__: Appender.prototype,

  
  append: function App_append(message) {
    if (message) {
      let m = this._formatter.format(message);
      if (message.level > Log.Level.Warn) {
        Cu.reportError(m);
        return;
      }
      this.doAppend(m);
    }
  },

  doAppend: function CApp_doAppend(formatted) {
    Cc["@mozilla.org/consoleservice;1"].
      getService(Ci.nsIConsoleService).logStringMessage(formatted);
  }
};









function StorageStreamAppender(formatter) {
  Appender.call(this, formatter);
  this._name = "StorageStreamAppender";
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

  doAppend: function (formatted) {
    if (!formatted) {
      return;
    }
    try {
      this.outputStream.writeString(formatted + "\n");
    } catch(ex) {
      if (ex.result == Cr.NS_BASE_STREAM_CLOSED) {
        
        
        this._outputStream = null;
      } try {
          this.outputStream.writeString(formatted + "\n");
      } catch (ex) {
        
      }
    }
  }
};






function FileAppender(path, formatter) {
  Appender.call(this, formatter);
  this._name = "FileAppender";
  this._encoder = new TextEncoder();
  this._path = path;
  this._file = null;
  this._fileReadyPromise = null;

  
  this._lastWritePromise = null;
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
    }

    return this._fileReadyPromise;
  },

  doAppend: function (formatted) {
    let array = this._encoder.encode(formatted + "\n");
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
  FileAppender.call(this, path, formatter);
  this._name = "BoundedFileAppender";
  this._size = 0;
  this._maxSize = maxSize;
  this._closeFilePromise = null;
}

BoundedFileAppender.prototype = {
  __proto__: FileAppender.prototype,

  doAppend: function (formatted) {
    if (!this._removeFilePromise) {
      if (this._size < this._maxSize) {
        this._size += formatted.length;
        return FileAppender.prototype.doAppend.call(this, formatted);
      }
      this._removeFilePromise = this.reset();
    }
    this._removeFilePromise.then(_ => {
      this._removeFilePromise = null;
      this.doAppend(formatted);
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

