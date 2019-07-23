







































function testprivatecl() {
}

testprivatecl.prototype = {
  _arguments: ["private", "silent"],
  get length getLength() {
    return this._arguments.length;
  },
  getArgument: function getArgument(aIndex) {
    return this._arguments[aIndex];
  },
  findFlag: function findFlag(aFlag, aCaseSensitive) {
    for (let i = 0; i < this._arguments.length; ++i)
      if (aCaseSensitive ?
          (this._arguments[i] == aFlag) :
          (this._arguments[i].toLowerCase() == aFlag.toLowerCase()))
        return i;
    return -1;
  },
  removeArguments: function removeArguments(aStart, aEnd) {
    this._arguments.splice(aStart, aEnd - aStart + 1);
  },
  handleFlag: function handleFlag (aFlag, aCaseSensitive) {
    let res = this.findFlag(aFlag, aCaseSensitive);
    if (res > -1) {
      this.removeArguments(res, res);
      return true;
    }
    return false;
  },
  handleFlagWithParam: function handleFlagWithParam(aFlag, aCaseSensitive) {
    return null;
  },
  STATE_INITIAL_LAUNCH: 0,
  STATE_REMOTE_AUTO: 1,
  STATE_REMOTE_EXPLICIT: 2,
  get state getState() {
    return this.STATE_INITIAL_LAUNCH;
  },
  preventDefault: false,
  workingDirectory: null,
  windowContext: null,
  resolveFile: function resolveFile (aArgument) {
    return null;
  },
  resolveURI: function resolveURI (aArgument) {
    return null;
  },
  QueryInterface: function(aIID) {
    if (!aIID.equals(Ci.nsICommandLine)
        && !aIID.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
}

function do_test() {
  
  let pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService);

  let testcl = new testprivatecl();

  let catMan = Cc["@mozilla.org/categorymanager;1"].
               getService(Ci.nsICategoryManager);
  let categories = catMan.enumerateCategory("command-line-handler");
  while (categories.hasMoreElements()) {
    let category = categories.getNext().QueryInterface(Ci.nsISupportsCString).data;
    let contractID = catMan.getCategoryEntry("command-line-handler", category);
    let handler = Cc[contractID].getService(Ci.nsICommandLineHandler);
    handler.handle(testcl);
  }

  
  do_check_true(pb.privateBrowsingEnabled);
  
  do_check_true(pb.autoStarted);
}
