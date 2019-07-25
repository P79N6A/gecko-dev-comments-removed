



let EXPORTED_SYMBOLS = [ "CmdCommands" ];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource:///modules/devtools/gcli.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let prefSvc = "@mozilla.org/preferences-service;1";
XPCOMUtils.defineLazyGetter(this, "prefBranch", function() {
  let prefService = Cc[prefSvc].getService(Ci.nsIPrefService);
  return prefService.getBranch(null).QueryInterface(Ci.nsIPrefBranch2);
});

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource:///modules/devtools/Console.jsm");






let commands = [];




let CmdCommands = {
  





  refreshAutoCommands: function GC_refreshAutoCommands(aSandboxPrincipal) {
    
    commands.forEach(function(name) {
      gcli.removeCommand(name);
    });

    let dirName = prefBranch.getComplexValue("devtools.commands.dir",
                                             Ci.nsISupportsString).data;
    if (dirName == "") {
      return;
    }

    let dir = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    dir.initWithPath(dirName);
    if (!dir.exists() || !dir.isDirectory()) {
      throw new Error('\'' + dirName + '\' is not a directory.');
    }

    let en = dir.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);

    while (true) {
      let file = en.nextFile;
      if (!file) {
        break;
      }
      if (file.leafName.match(/.*\.mozcmd$/) && file.isFile() && file.isReadable()) {
        loadCommandFile(file, aSandboxPrincipal);
      }
    }
  },
};







function loadCommandFile(aFile, aSandboxPrincipal) {
  NetUtil.asyncFetch(aFile, function refresh_fetch(aStream, aStatus) {
    if (!Components.isSuccessCode(aStatus)) {
      console.error("NetUtil.asyncFetch(" + aFile.path + ",..) failed. Status=" + aStatus);
      return;
    }

    let source = NetUtil.readInputStreamToString(aStream, aStream.available());
    aStream.close();

    let sandbox = new Cu.Sandbox(aSandboxPrincipal, {
      sandboxPrototype: aSandboxPrincipal,
      wantXrays: false,
      sandboxName: aFile.path
    });
    let data = Cu.evalInSandbox(source, sandbox, "1.8", aFile.leafName, 1);

    if (!Array.isArray(data)) {
      console.error("Command file '" + aFile.leafName + "' does not have top level array.");
      return;
    }

    data.forEach(function(commandSpec) {
      gcli.addCommand(commandSpec);
      commands.push(commandSpec.name);
    });
  }.bind(this));
}




gcli.addCommand({
  name: "cmd",
  description: gcli.lookup("cmdDesc"),
  hidden: true
});




gcli.addCommand({
  name: "cmd refresh",
  description: gcli.lookup("cmdRefreshDesc"),
  hidden: true,
  exec: function Command_cmdRefresh(args, context) {
    GcliCmdCommands.refreshAutoCommands(context.environment.chromeDocument.defaultView);
  }
});
