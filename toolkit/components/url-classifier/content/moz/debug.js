# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Google Safe Browsing.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Fritz Schneider <fritz@google.com> (original author)
#   Annie Sullivan <sullivan@google.com>
#   Aaron Boodman <aa@google.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****
































































if (typeof G_GDEBUG == "undefined") {
  throw new Error("G_GDEBUG constant must be set before loading debug.js");
}









function G_Debug(who, msg) {
  if (G_GDEBUG) {
    G_GetDebugZone(who).debug(msg);
  }
}




function G_DebugL(who, msg) {
  if (G_GDEBUG) {
    var zone = G_GetDebugZone(who);

    if (zone.zoneIsEnabled()) {
      G_debugService.dump(
        G_File.LINE_END_CHAR +
        "************************************************************" +
        G_File.LINE_END_CHAR);

      G_Debug(who, msg);

      G_debugService.dump(
        "************************************************************" +
        G_File.LINE_END_CHAR +
        G_File.LINE_END_CHAR);
    }
  }
}








function G_TraceCall(who, msg) {
  if (G_GDEBUG) {
    if (G_debugService.callTracingEnabled()) {
      G_debugService.dump(msg + G_File.LINE_END_CHAR);
    }
  }
}








function G_Error(who, msg) {
  if (G_GDEBUG) {
    G_GetDebugZone(who).error(msg);
  }
}









function G_Assert(who, condition, msg) {
  if (G_GDEBUG) {
    G_GetDebugZone(who).assert(condition, msg);
  }
}




function G_AssertEqual(who, expected, actual, msg) {
  if (G_GDEBUG) {
    G_GetDebugZone(who).assert(
        expected == actual,
        msg + " Expected: {%s}, got: {%s}".subs(expected, actual));
  }
}









function G_GetDebugZone(who) {
  if (G_GDEBUG) {
    var zone = "?";

    if (who && who.debugZone) {
      zone = who.debugZone;
    } else if (isString(who)) {
      zone = who;
    }

    return G_debugService.getZone(zone);
  }
}


















function G_DebugZone(service, prefix, zone) {
  if (G_GDEBUG) {
    this.debugService_ = service;
    this.prefix_ = prefix;
    this.zone_ = zone;
    this.zoneEnabledPrefName_ = prefix + ".zone." + this.zone_;
    this.settings_ = new G_DebugSettings();
  }
}
  



G_DebugZone.prototype.zoneIsEnabled = function() {
  if (G_GDEBUG) {
    var explicit = this.settings_.getSetting(this.zoneEnabledPrefName_, null);

    if (explicit !== null) {
      return explicit;
    } else {
      return this.debugService_.allZonesEnabled();
    }
  }
}




G_DebugZone.prototype.enableZone = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.zoneEnabledPrefName_, true);
  }
}




G_DebugZone.prototype.disableZone = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.zoneEnabledPrefName_, false);
  }
}






G_DebugZone.prototype.debug = function(msg) {
  if (G_GDEBUG) {
    if (this.zoneIsEnabled()) {
      this.debugService_.dump("[%s] %s%s".subs(this.zone_,
                                               msg,
                                               G_File.LINE_END_CHAR));
    }
  }
}






G_DebugZone.prototype.error = function(msg) {
  if (G_GDEBUG) {
    this.debugService_.dump("[%s] %s%s".subs(this.zone_,
                                             msg,
                                             G_File.LINE_END_CHAR));
    throw new Error(msg);
    debugger;
  }
}







G_DebugZone.prototype.assert = function(condition, msg) {
  if (G_GDEBUG) {
    if (condition !== true) {
      G_Error(this.zone_, "ASSERT FAILED: " + msg);
    }
  }
}











function G_DebugService(opt_prefix) {
  if (G_GDEBUG) {
    this.prefix_ = opt_prefix ? opt_prefix : "safebrowsing-debug-service";
    this.consoleEnabledPrefName_ = this.prefix_ + ".alsologtoconsole";
    this.allZonesEnabledPrefName_ = this.prefix_ + ".enableallzones";
    this.callTracingEnabledPrefName_ = this.prefix_ + ".trace-function-calls";
    this.logFileEnabledPrefName_ = this.prefix_ + ".logfileenabled";
    this.logFileErrorLevelPrefName_ = this.prefix_ + ".logfile-errorlevel";
    this.zones_ = {};

    this.loggifier = new G_Loggifier();
    this.settings_ = new G_DebugSettings();

    
    
    Cc["@mozilla.org/consoleservice;1"]
      .getService(Ci.nsIConsoleService)
      .registerListener(this);
  }
}


G_DebugService.ERROR_LEVEL_INFO = "INFO";
G_DebugService.ERROR_LEVEL_WARNING = "WARNING";
G_DebugService.ERROR_LEVEL_EXCEPTION = "EXCEPTION";





G_DebugService.prototype.alsoDumpToConsole = function() {
  if (G_GDEBUG) {
    return this.settings_.getSetting(this.consoleEnabledPrefName_, false);
  }
}




G_DebugService.prototype.logFileIsEnabled = function() {
  if (G_GDEBUG) {
    return this.settings_.getSetting(this.logFileEnabledPrefName_, false);
  }
}





G_DebugService.prototype.enableLogFile = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.logFileEnabledPrefName_, true);
  }
}




G_DebugService.prototype.disableLogFile = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.logFileEnabledPrefName_, false);
  }
}




G_DebugService.prototype.getLogFile = function() {
  if (G_GDEBUG) {
    return this.logFile_;
  }
}




G_DebugService.prototype.setLogFile = function(file) {
  if (G_GDEBUG) {
    this.logFile_ = file;
  }
}




G_DebugService.prototype.enableDumpToConsole = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.consoleEnabledPrefName_, true);
  }
}




G_DebugService.prototype.disableDumpToConsole = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.consoleEnabledPrefName_, false);
  }
}






G_DebugService.prototype.getZone = function(zone) {
  if (G_GDEBUG) {
    if (!this.zones_[zone]) 
      this.zones_[zone] = new G_DebugZone(this, this.prefix_, zone);
    
    return this.zones_[zone];
  }
}




G_DebugService.prototype.enableZone = function(zone) {
  if (G_GDEBUG) {
    var toEnable = this.getZone(zone);
    toEnable.enableZone();
  }
}




G_DebugService.prototype.disableZone = function(zone) {
  if (G_GDEBUG) {
    var toDisable = this.getZone(zone);
    toDisable.disableZone();
  }
}




G_DebugService.prototype.allZonesEnabled = function() {
  if (G_GDEBUG) {
    return this.settings_.getSetting(this.allZonesEnabledPrefName_, false);
  }
}




G_DebugService.prototype.enableAllZones = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.allZonesEnabledPrefName_, true);
  }
}




G_DebugService.prototype.disableAllZones = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.allZonesEnabledPrefName_, false);
  }
}




G_DebugService.prototype.callTracingEnabled = function() {
  if (G_GDEBUG) {
    return this.settings_.getSetting(this.callTracingEnabledPrefName_, false);
  }
}




G_DebugService.prototype.enableCallTracing = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.callTracingEnabledPrefName_, true);
  }
}




G_DebugService.prototype.disableCallTracing = function() {
  if (G_GDEBUG) {
    this.settings_.setDefault(this.callTracingEnabledPrefName_, false);
  }
}




G_DebugService.prototype.getLogFileErrorLevel = function() {
  if (G_GDEBUG) {
    var level = this.settings_.getSetting(this.logFileErrorLevelPrefName_, 
                                          G_DebugService.ERROR_LEVEL_EXCEPTION);

    return level.toUpperCase();
  }
}




G_DebugService.prototype.setLogFileErrorLevel = function(level) {
  if (G_GDEBUG) {
    
    level = level.toUpperCase();

    if (level != G_DebugService.ERROR_LEVEL_INFO &&
        level != G_DebugService.ERROR_LEVEL_WARNING &&
        level != G_DebugService.ERROR_LEVEL_EXCEPTION) {
      throw new Error("Invalid error level specified: {" + level + "}");
    }

    this.settings_.setDefault(this.logFileErrorLevelPrefName_, level);
  }
}






G_DebugService.prototype.dump = function(msg) {
  if (G_GDEBUG) {
    dump(msg);
    
    if (this.alsoDumpToConsole()) {
      try {
        var console = Components.classes['@mozilla.org/consoleservice;1']
                      .getService(Components.interfaces.nsIConsoleService);
        console.logStringMessage(msg);
      } catch(e) {
        dump("G_DebugZone ERROR: COULD NOT DUMP TO CONSOLE" +
             G_File.LINE_END_CHAR);
      }
    }

    this.maybeDumpToFile(msg);
  }
}




G_DebugService.prototype.maybeDumpToFile = function(msg) {
  if (this.logFileIsEnabled() && this.logFile_) {
    if (!this.logWriter_) {
      this.logWriter_ = new G_FileWriter(this.logFile_, true);
    }

    this.logWriter_.write(msg);
  }
}





G_DebugService.prototype.observe = function(consoleMessage) {
  if (G_GDEBUG) {
    var errorLevel = this.getLogFileErrorLevel();

    
    
    
    if (!(consoleMessage instanceof Ci.nsIScriptError)) {
      
      if (errorLevel == G_DebugService.ERROR_LEVEL_INFO) {
        this.maybeDumpToFile(G_DebugService.ERROR_LEVEL_INFO + ": " + 
                             consoleMessage.message + G_File.LINE_END_CHAR);
      }

      return;
    }

    
    
    var flags = consoleMessage.flags;
    var sourceName = consoleMessage.sourceName;
    var lineNumber = consoleMessage.lineNumber;

    
    
    if (!flags) {
      flags = Ci.nsIScriptError.exceptionFlag;
    }

    
    if (!sourceName) {
      sourceName = "<unknown>";
    }

    if (!lineNumber) {
      lineNumber = "<unknown>";
    }

    
    if (flags & Ci.nsIScriptError.warningFlag) {
      
      if (errorLevel == G_DebugService.ERROR_LEVEL_WARNING ||
          errorLevel == G_DebugService.ERROR_LEVEL_INFO) {
        this.reportScriptError_(consoleMessage.message,
                                sourceName,
                                lineNumber,
                                G_DebugService.ERROR_LEVEL_WARNING);
      }
    } else if (flags & Ci.nsIScriptError.exceptionFlag) {
      
      this.reportScriptError_(consoleMessage.message,
                              sourceName,
                              lineNumber,
                              G_DebugService.ERROR_LEVEL_EXCEPTION);
    }
  }
}




G_DebugService.prototype.reportScriptError_ = function(message, sourceName, 
                                                       lineNumber, label) {
  var message = ["",
                 "------------------------------------------------------------",
                 label + ": " + message,
                 "location: " + sourceName + ", " + "line: " + lineNumber,
                 "------------------------------------------------------------",
                 "",
                 ""].join(G_File.LINE_END_CHAR);

  dump(message);
  this.maybeDumpToFile(message);
}














function G_Loggifier() {
  if (G_GDEBUG) {
    
    this.mark_(this);  
  }
}







G_Loggifier.prototype.mark_ = function(obj) {
  if (G_GDEBUG) {
    obj.__loggified_ = true;
  }
}





G_Loggifier.prototype.isLoggified = function(obj) {
  if (G_GDEBUG) {
    return !!obj.__loggified_;
  }
}









G_Loggifier.prototype.getFunctionName_ = function(constructor) {
  if (G_GDEBUG) {
    return constructor.name || "???";
  }
}


















G_Loggifier.prototype.loggify = function(obj) {
  if (G_GDEBUG) {
    if (!G_debugService.callTracingEnabled()) {
      return;
    }

    if (typeof window != "undefined" && obj == window || 
        this.isLoggified(obj))   
      return;

    var zone = G_GetDebugZone(obj);
    if (!zone || !zone.zoneIsEnabled()) {
      return;
    }

    this.mark_(obj);

    
    
    
    
    

    function wrap(meth, objName, methName) {
      return function() {
        
        
        var args = new Array(arguments.length);
        var argsString = "";
        for (var i = 0; i < args.length; i++) {
          args[i] = arguments[i];
          argsString += (i == 0 ? "" : ", ");
          
          if (isFunction(args[i])) {
            argsString += "[function]";
          } else {
            argsString += args[i];
          }
        }

        G_TraceCall(this, "> " + objName + "." + methName + "(" + 
                    argsString + ")");
        
        
        try {
          var retVal = meth.apply(this, arguments);
          var reportedRetVal = retVal;

          if (typeof reportedRetVal == "undefined")
            reportedRetVal = "void";
          else if (reportedRetVal === "")
            reportedRetVal = "\"\" (empty string)";
        } catch (e) {
          if (e && !e.__logged) {
            G_TraceCall(this, "Error: " + e.message + ". " + 
                        e.fileName + ": " + e.lineNumber);
            try {
              e.__logged = true;
            } catch (e2) {
              
              
              throw e;
            }
          }
          
          throw e;      
        }

        
        G_TraceCall(
          this, 
          "< " + objName + "." + methName + ": " + reportedRetVal);

        return retVal;
      };
    };

    var ignoreLookup = {};

    if (arguments.length > 1) {
      for (var i = 1; i < arguments.length; i++) {
        ignoreLookup[arguments[i]] = true;
      }
    }
    
    
    for (var p in obj) {
      
      
      if (typeof obj[p] == "function" && obj[p].call && !ignoreLookup[p]) {
        var objName = this.getFunctionName_(obj.constructor);
        obj[p] = wrap(obj[p], objName, p);
      }
    }
  }
}











function G_DebugSettings() {
  this.defaults_ = {};
  this.prefs_ = new G_Preferences();
}





G_DebugSettings.prototype.getSetting = function(name, opt_default) {
  var override = this.prefs_.getPref(name, null);

  if (override !== null) {
    return override;
  } else if (typeof this.defaults_[name] != "undefined") {
    return this.defaults_[name];
  } else {
    return opt_default;
  }
}





G_DebugSettings.prototype.setDefault = function(name, val) {
  this.defaults_[name] = val;
}

var G_debugService = new G_DebugService(); 

if (G_GDEBUG) {
  G_debugService.enableAllZones();
}
