



















































































var EXPORTED_SYMBOLS = [ "gcli" ];






























function fmt(aStr, aMaxLen, aMinLen, aOptions) {
  if (aMinLen == undefined) {
    aMinLen = aMaxLen;
  }
  if (aStr == null) {
    aStr = "";
  }
  if (aStr.length > aMaxLen) {
    if (aOptions && aOptions.truncate == "start") {
      return "_" + aStr.substring(aStr.length - aMaxLen + 1);
    }
    else {
      return aStr.substring(0, aMaxLen - 1) + "_";
    }
  }
  if (aStr.length < aMinLen) {
    return Array(aMinLen - aStr.length + 1).join(" ") + aStr;
  }
  return aStr;
}










function getCtorName(aObj) {
  return Object.prototype.toString.call(aObj).slice(8, -1);
}










function stringify(aThing) {
  if (aThing === undefined) {
    return "undefined";
  }

  if (aThing === null) {
    return "null";
  }

  if (typeof aThing == "object") {
    try {
      return getCtorName(aThing) + " " + fmt(JSON.stringify(aThing), 50, 0);
    }
    catch (ex) {
      return "[stringify error]";
    }
  }

  var str = aThing.toString().replace(/\s+/g, " ");
  return fmt(str, 60, 0);
}









function log(aThing) {
  if (aThing == null) {
    return "null";
  }

  if (aThing == undefined) {
    return "undefined";
  }

  if (typeof aThing == "object") {
    var reply = "";
    var type = getCtorName(aThing);
    if (type == "Error") {
      reply += "  " + aThing.message + "\n";
      reply += logProperty("stack", aThing.stack);
    }
    else {
      var keys = Object.getOwnPropertyNames(aThing);
      if (keys.length > 0) {
        reply += type + "\n";
        keys.forEach(function(aProp) {
          reply += logProperty(aProp, aThing[aProp]);
        }, this);
      }
      else {
        reply += type + " (enumerated with for-in)\n";
        var prop;
        for (prop in aThing) {
          reply += logProperty(prop, aThing[prop]);
        }
      }
    }

    return reply;
  }

  return "  " + aThing.toString() + "\n";
}












function logProperty(aProp, aValue) {
  var reply = "";
  if (aProp == "stack" && typeof value == "string") {
    var trace = parseStack(aValue);
    reply += formatTrace(trace);
  }
  else {
    reply += "    - " + aProp + " = " + stringify(aValue) + "\n";
  }
  return reply;
}










function parseStack(aStack) {
  var trace = [];
  aStack.split("\n").forEach(function(line) {
    if (!line) {
      return;
    }
    var at = line.lastIndexOf("@");
    var posn = line.substring(at + 1);
    trace.push({
      file: posn.split(":")[0],
      line: posn.split(":")[1],
      call: line.substring(0, at)
    });
  }, this);
  return trace;
}











function getStack(aFrame) {
  if (!aFrame) {
    aFrame = Components.stack.caller;
  }
  var trace = [];
  while (aFrame) {
    trace.push({
      file: aFrame.filename,
      line: aFrame.lineNumber,
      call: aFrame.name
    });
    aFrame = aFrame.caller;
  }
  return trace;
};









function formatTrace(aTrace) {
  var reply = "";
  aTrace.forEach(function(frame) {
    reply += fmt(frame.file, 20, 20, { truncate: "start" }) + " " +
             fmt(frame.line, 5, 5) + " " +
             fmt(frame.call, 75, 75) + "\n";
  });
  return reply;
}












function createDumper(aLevel) {
  return function() {
    var args = Array.prototype.slice.call(arguments, 0);
    var data = args.map(function(arg) {
      return stringify(arg);
    });
    dump(aLevel + ": " + data.join(", ") + "\n");
  };
}












function createMultiLineDumper(aLevel) {
  return function() {
    dump(aLevel + "\n");
    var args = Array.prototype.slice.call(arguments, 0);
    args.forEach(function(arg) {
      dump(log(arg));
    });
  };
}




var console = {
  debug: createMultiLineDumper("debug"),
  log: createDumper("log"),
  info: createDumper("info"),
  warn: createDumper("warn"),
  error: createMultiLineDumper("error"),
  trace: function Console_trace() {
    var trace = getStack(Components.stack.caller);
    dump(formatTrace(trace) + "\n");
  },
  clear: function Console_clear() {},

  dir: createMultiLineDumper("dir"),
  dirxml: createMultiLineDumper("dirxml"),
  group: createDumper("group"),
  groupEnd: createDumper("groupEnd")
};









var debugDependencies = false;







function define(moduleName, deps, payload) {
  if (typeof moduleName != "string") {
    console.error(this.depth + " Error: Module name is not a string.");
    console.trace();
    return;
  }

  if (arguments.length == 2) {
    payload = deps;
  }

  if (debugDependencies) {
    console.log("define: " + moduleName + " -> " + payload.toString()
        .slice(0, 40).replace(/\n/, '\\n').replace(/\r/, '\\r') + "...");
  }

  if (moduleName in define.modules) {
    console.error(this.depth + " Error: Redefining module: " + moduleName);
  }
  define.modules[moduleName] = payload;
};




define.modules = {};










function Domain() {
  this.modules = {};

  if (debugDependencies) {
    this.depth = "";
  }
}
















Domain.prototype.require = function(deps, callback) {
  if (Array.isArray(deps)) {
    var params = deps.map(function(dep) {
      return this.lookup(dep);
    }, this);
    if (callback) {
      callback.apply(null, params);
    }
    return undefined;
  }
  else {
    return this.lookup(deps);
  }
};







Domain.prototype.lookup = function(moduleName) {
  if (moduleName in this.modules) {
    var module = this.modules[moduleName];
    if (debugDependencies) {
      console.log(this.depth + " Using module: " + moduleName);
    }
    return module;
  }

  if (!(moduleName in define.modules)) {
    console.error(this.depth + " Missing module: " + moduleName);
    return null;
  }

  var module = define.modules[moduleName];

  if (debugDependencies) {
    console.log(this.depth + " Compiling module: " + moduleName);
  }

  if (typeof module == "function") {
    if (debugDependencies) {
      this.depth += ".";
    }

    var exports = {};
    try {
      module(this.require.bind(this), exports, { id: moduleName, uri: "" });
    }
    catch (ex) {
      console.error("Error using module: " + moduleName, ex);
      throw ex;
    }
    module = exports;

    if (debugDependencies) {
      this.depth = this.depth.slice(0, -1);
    }
  }

  
  this.modules[moduleName] = module;

  return module;
};






define.Domain = Domain;
define.globalDomain = new Domain();





var require = define.globalDomain.require.bind(define.globalDomain);










define('gcli/index', [ ], function(require, exports, module) {

  exports.addCommand = function() {  };
  exports.removeCommand = function() {  };
  exports.startup = function() {  };
  exports.shutdown = function() {  };

});







var gcli = require("gcli/index");
gcli.createView = require("gcli/ui/start/firefox");
gcli._internal = { require: require, define: define, console: console };

