



"use strict";

















this.EXPORTED_SYMBOLS = [ "console" ];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

let gTimerRegistry = new Map();




















function fmt(aStr, aMaxLen, aMinLen, aOptions) {
  if (aMinLen == null) {
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
  if (aObj === null) {
    return "null";
  }
  if (aObj === undefined) {
    return "undefined";
  }
  if (aObj.constructor && aObj.constructor.name) {
    return aObj.constructor.name;
  }
  
  
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
    let type = getCtorName(aThing);
    if (aThing instanceof Components.interfaces.nsIDOMNode && aThing.tagName) {
      return debugElement(aThing);
    }
    type = (type == "Object" ? "" : type + " ");
    let json;
    try {
      json = JSON.stringify(aThing);
    }
    catch (ex) {
      
      json = "{" + Object.keys(aThing).join(":..,") + ":.., " + "}";
    }
    return type + fmt(json, 50, 0);
  }

  if (typeof aThing == "function") {
    return fmt(aThing.toString().replace(/\s+/g, " "), 80, 0);
  }

  let str = aThing.toString().replace(/\n/g, "|");
  return fmt(str, 80, 0);
}









function debugElement(aElement) {
  return "<" + aElement.tagName +
      (aElement.id ? "#" + aElement.id : "") +
      (aElement.className ?
          "." + aElement.className.split(" ").join(" .") :
          "") +
      ">";
}









function log(aThing) {
  if (aThing === null) {
    return "null\n";
  }

  if (aThing === undefined) {
    return "undefined\n";
  }

  if (typeof aThing == "object") {
    let reply = "";
    let type = getCtorName(aThing);
    if (type == "Map") {
      reply += "Map\n";
      for (let [key, value] of aThing) {
        reply += logProperty(key, value);
      }
    }
    else if (type == "Set") {
      let i = 0;
      reply += "Set\n";
      for (let value of aThing) {
        reply += logProperty('' + i, value);
        i++;
      }
    }
    else if (type.match("Error$")) {
      reply += "  Message: " + aThing + "\n";
      if (aThing.stack) {
        reply += "  Stack:\n";
        var frame = aThing.stack;
        while (frame) {
          reply += "    " + frame + "\n";
          frame = frame.caller;
        }
      }
    }
    else if (aThing instanceof Components.interfaces.nsIDOMNode && aThing.tagName) {
      reply += "  " + debugElement(aThing) + "\n";
    }
    else {
      let keys = Object.getOwnPropertyNames(aThing);
      if (keys.length > 0) {
        reply += type + "\n";
        keys.forEach(function(aProp) {
          reply += logProperty(aProp, aThing[aProp]);
        });
      }
      else {
        reply += type + "\n";
        let root = aThing;
        let logged = [];
        while (root != null) {
          let properties = Object.keys(root);
          properties.sort();
          properties.forEach(function(property) {
            if (!(property in logged)) {
              logged[property] = property;
              reply += logProperty(property, aThing[property]);
            }
          });

          root = Object.getPrototypeOf(root);
          if (root != null) {
            reply += '  - prototype ' + getCtorName(root) + '\n';
          }
        }
      }
    }

    return reply;
  }

  return "  " + aThing.toString() + "\n";
}












function logProperty(aProp, aValue) {
  let reply = "";
  if (aProp == "stack" && typeof value == "string") {
    let trace = parseStack(aValue);
    reply += formatTrace(trace);
  }
  else {
    reply += "    - " + aProp + " = " + stringify(aValue) + "\n";
  }
  return reply;
}










function parseStack(aStack) {
  let trace = [];
  aStack.split("\n").forEach(function(line) {
    if (!line) {
      return;
    }
    let at = line.lastIndexOf("@");
    let posn = line.substring(at + 1);
    trace.push({
      filename: posn.split(":")[0],
      lineNumber: posn.split(":")[1],
      functionName: line.substring(0, at)
    });
  });
  return trace;
}














function getStack(aFrame, aMaxDepth = 0) {
  if (!aFrame) {
    aFrame = Components.stack.caller;
  }
  let trace = [];
  while (aFrame) {
    trace.push({
      filename: aFrame.filename,
      lineNumber: aFrame.lineNumber,
      functionName: aFrame.name,
      language: aFrame.language,
    });
    if (aMaxDepth == trace.length) {
      break;
    }
    aFrame = aFrame.caller;
  }
  return trace;
}









function formatTrace(aTrace) {
  let reply = "";
  aTrace.forEach(function(frame) {
    reply += fmt(frame.filename, 20, 20, { truncate: "start" }) + " " +
             fmt(frame.lineNumber, 5, 5) + " " +
             fmt(frame.functionName, 75, 75) + "\n";
  });
  return reply;
}














function startTimer(aName, aTimestamp) {
  let key = aName.toString();
  if (!gTimerRegistry.has(key)) {
    gTimerRegistry.set(key, aTimestamp || Date.now());
  }
  return { name: aName, started: gTimerRegistry.get(key) };
}












function stopTimer(aName, aTimestamp) {
  let key = aName.toString();
  let duration = (aTimestamp || Date.now()) - gTimerRegistry.get(key);
  gTimerRegistry.delete(key);
  return { name: aName, duration: duration };
}












function createDumper(aLevel) {
  return function() {
    let args = Array.prototype.slice.call(arguments, 0);
    let frame = getStack(Components.stack.caller, 1)[0];
    sendConsoleAPIMessage(aLevel, frame, args);
    let data = args.map(function(arg) {
      return stringify(arg);
    });
    dump("console." + aLevel + ": " + data.join(", ") + "\n");
  };
}












function createMultiLineDumper(aLevel) {
  return function() {
    dump("console." + aLevel + ": \n");
    let args = Array.prototype.slice.call(arguments, 0);
    let frame = getStack(Components.stack.caller, 1)[0];
    sendConsoleAPIMessage(aLevel, frame, args);
    args.forEach(function(arg) {
      dump(log(arg));
    });
  };
}




















function sendConsoleAPIMessage(aLevel, aFrame, aArgs, aOptions = {})
{
  let consoleEvent = {
    ID: aFrame.filename,
    level: aLevel,
    filename: aFrame.filename,
    lineNumber: aFrame.lineNumber,
    functionName: aFrame.functionName,
    timeStamp: Date.now(),
    arguments: aArgs,
  };

  consoleEvent.wrappedJSObject = consoleEvent;

  switch (aLevel) {
    case "trace":
      consoleEvent.stacktrace = aOptions.stacktrace;
      break;
    case "time":
    case "timeEnd":
      consoleEvent.timer = aOptions.timer;
      break;
    case "group":
    case "groupCollapsed":
    case "groupEnd":
      try {
        consoleEvent.groupName = Array.prototype.join.call(aArgs, " ");
      }
      catch (ex) {
        Cu.reportError(ex);
        Cu.reportError(ex.stack);
        return;
      }
      break;
  }

  Services.obs.notifyObservers(consoleEvent, "console-api-log-event", null);
}






this.console = {
  debug: createMultiLineDumper("debug"),
  log: createDumper("log"),
  info: createDumper("info"),
  warn: createDumper("warn"),
  error: createMultiLineDumper("error"),

  trace: function Console_trace() {
    let args = Array.prototype.slice.call(arguments, 0);
    let trace = getStack(Components.stack.caller);
    sendConsoleAPIMessage("trace", trace[0], args,
                          { stacktrace: trace });
    dump("console.trace:\n" + formatTrace(trace) + "\n");
  },
  clear: function Console_clear() {},

  dir: createMultiLineDumper("dir"),
  dirxml: createMultiLineDumper("dirxml"),
  group: createDumper("group"),
  groupEnd: createDumper("groupEnd"),

  time: function Console_time() {
    let args = Array.prototype.slice.call(arguments, 0);
    let frame = getStack(Components.stack.caller, 1)[0];
    let timer = startTimer(args[0]);
    sendConsoleAPIMessage("time", frame, args, { timer: timer });
    dump("console.time: '" + timer.name + "' @ " + (new Date()) + "\n");
  },

  timeEnd: function Console_timeEnd() {
    let args = Array.prototype.slice.call(arguments, 0);
    let frame = getStack(Components.stack.caller, 1)[0];
    let timer = stopTimer(args[0]);
    sendConsoleAPIMessage("timeEnd", frame, args, { timer: timer });
    dump("console.timeEnd: '" + timer.name + "' " + timer.duration + "ms\n");
  },
};
