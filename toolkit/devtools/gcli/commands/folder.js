



"use strict";

const { Cc, Ci, Cu, CC } = require("chrome");
const { Services } = require("resource://gre/modules/Services.jsm");
const l10n = require("gcli/l10n");
const dirService = Cc["@mozilla.org/file/directory_service;1"]
                      .getService(Ci.nsIProperties);

function showFolder(aPath) {
  let nsLocalFile = CC("@mozilla.org/file/local;1", "nsILocalFile",
                        "initWithPath");

  try {
    let file = new nsLocalFile(aPath);

    if (file.exists()) {
      file.reveal();
      return l10n.lookupFormat("folderOpenDirResult", [aPath]);
    } else {
      return l10n.lookup("folderInvalidPath");
    }
  } catch (e) {
    return l10n.lookup("folderInvalidPath");
  }
}

exports.items = [
  {
    name: "folder",
    description: l10n.lookup("folderDesc")
  },
  {
    item: "command",
    runAt: "client",
    name: "folder open",
    description: l10n.lookup("folderOpenDesc"),
    params: [
      {
        name: "path",
        type: { name: "string", allowBlank: true },
        defaultValue: "~",
        description: l10n.lookup("folderOpenDir")
      }
    ],
    returnType: "string",
    exec: function(args, context) {
      let dirName = args.path;

      
      if (dirName.indexOf("~") == 0) {
        let homeDirFile = dirService.get("Home", Ci.nsIFile);
        let homeDir = homeDirFile.path;
        dirName = dirName.substr(1);
        dirName = homeDir + dirName;
      }

      return showFolder(dirName);
    }
  },
  {
    item: "command",
    runAt: "client",
    name: "folder openprofile",
    description: l10n.lookup("folderOpenProfileDesc"),
    returnType: "string",
    exec: function(args, context) {
      
      let currProfD = Services.dirsvc.get("ProfD", Ci.nsIFile);
      let profileDir = currProfD.path;
      return showFolder(profileDir);
    }
  }
];
