









var EXPORTED_SYMBOLS = ["NS_ASSERT"];

var gTraceOnAssert = true;


















function NS_ASSERT(condition, message) {
  if (condition)
    return;

  var releaseBuild = true;
  var defB = Components.classes["@mozilla.org/preferences-service;1"]
                       .getService(Components.interfaces.nsIPrefService)
                       .getDefaultBranch(null);
  try {
    switch (defB.getCharPref("app.update.channel")) {
      case "nightly":
      case "beta":
      case "default":
        releaseBuild = false;
    }
  } catch(ex) {}

  var caller = arguments.callee.caller;
  var assertionText = "ASSERT: " + message + "\n";

  if (releaseBuild) {
    
    Components.utils.reportError(assertionText);
    return;
  }

  
  dump(assertionText);

  var stackText = "";
  if (gTraceOnAssert) {
    stackText = "Stack Trace: \n";
    var count = 0;
    while (caller) {
      stackText += count++ + ":" + caller.name + "(";
      for (var i = 0; i < caller.arguments.length; ++i) {
        var arg = caller.arguments[i];
        stackText += arg;
        if (i < caller.arguments.length - 1)
          stackText += ",";
      }
      stackText += ")\n";
      caller = caller.arguments.callee.caller;
    }
  }

  var environment = Components.classes["@mozilla.org/process/environment;1"].
                    getService(Components.interfaces.nsIEnvironment);
  if (environment.exists("XUL_ASSERT_PROMPT") &&
      !parseInt(environment.get("XUL_ASSERT_PROMPT")))
    return;

  var source = null;
  if (this.window)
    source = this.window;
  var ps = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].
           getService(Components.interfaces.nsIPromptService);
  ps.alert(source, "Assertion Failed", assertionText + stackText);
}
