


const {interfaces: Ci, classes: Cc, utils: Cu, results: Cr} = Components;
Cu.import("resource:///modules/XPCOMUtils.jsm");

function TestReturnCodeChild() {}
TestReturnCodeChild.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Ci["nsIXPCTestReturnCodeChild"]]),
  contractID: "@mozilla.org/js/xpc/test/js/ReturnCodeChild;1",
  classID: Components.ID("{38dd78aa-467f-4fad-8dcf-4383a743e235}"),

  doIt(behaviour) {
    switch (behaviour) {
      case Ci.nsIXPCTestReturnCodeChild.CHILD_SHOULD_THROW:
        throw(new Error("a requested error"));
      case Ci.nsIXPCTestReturnCodeChild.CHILD_SHOULD_RETURN_SUCCESS:
        return;
      case Ci.nsIXPCTestReturnCodeChild.CHILD_SHOULD_RETURN_RESULTCODE:
        Components.returnCode = Cr.NS_ERROR_FAILURE;
        return;
      case Ci.nsIXPCTestReturnCodeChild.CHILD_SHOULD_NEST_RESULTCODES:
        
        

        
        
        
        Components.returnCode = Cr.NS_ERROR_UNEXPECTED;
        
        let sub = Cc[this.contractID].createInstance(Ci.nsIXPCTestReturnCodeChild);
        let childResult = Cr.NS_OK;
        try {
          sub.doIt(Ci.nsIXPCTestReturnCodeChild.CHILD_SHOULD_RETURN_RESULTCODE);
        } catch (ex) {
          childResult = ex.result;
        }
        
        let consoleService = Cc["@mozilla.org/consoleservice;1"]
                             .getService(Ci.nsIConsoleService);
        consoleService.logStringMessage("nested child returned " + childResult);
        return;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TestReturnCodeChild]);
