



















































var DiskSearch =
{
  findFiles: function(aRootDir, aExtList, aRecurse)
  {
    
    
    var extHash = {};
    for (var i = 0; i < aExtList.length; i++) {
      extHash[aExtList[i]] = true;
    }

    var ios = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
    this.fileHandler = ios.getProtocolHandler("file").QueryInterface(Components.interfaces.nsIFileProtocolHandler);

    
    var results = [];
    this.recurseDir(aRootDir, extHash, aRecurse, results);
    return results;
  },

  recurseDir: function(aDir, aExtHash, aRecurse, aResults)
  {
    debug("("+aResults.length+") entering " + aDir.path + "\n");
    var entries = aDir.directoryEntries;
    var entry, ext;
    while (entries.hasMoreElements()) {
      entry = XPCU.QI(entries.getNext(), "nsIFile");
      if (aRecurse && entry.isDirectory())
        this.recurseDir(entry, aExtHash, aRecurse, aResults);
      ext = this.getExtension(entry.leafName);
      if (ext) {
        if (aExtHash[ext])
          aResults.push(this.fileHandler.getURLSpecFromFile(entry));
      }
    }
  },

  getExtension: function(aFileName)
  {
    var dotpt = aFileName.lastIndexOf(".");
    if (dotpt)
      return aFileName.substr(dotpt+1).toLowerCase();

    return null;
  }
};

