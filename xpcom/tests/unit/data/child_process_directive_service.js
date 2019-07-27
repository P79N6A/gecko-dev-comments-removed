


Components.utils.import("resource:///modules/XPCOMUtils.jsm");

function TestProcessDirective() {}
TestProcessDirective.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsISupportsString]),
  contractID: "@mozilla.org/xpcom/tests/ChildProcessDirectiveTest;1",
  classID: Components.ID("{4bd1ba60-45c4-11e4-916c-0800200c9a66}"),

  type: Components.interfaces.nsISupportsString.TYPE_STRING,
  data: "child process",
  toString: function() {
    return this.data;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TestProcessDirective]);
