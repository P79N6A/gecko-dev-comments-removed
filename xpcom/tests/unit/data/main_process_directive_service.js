


Components.utils.import("resource:///modules/XPCOMUtils.jsm");

function TestProcessDirective() {}
TestProcessDirective.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsISupportsString]),
  contractID: "@mozilla.org/xpcom/tests/MainProcessDirectiveTest;1",
  classID: Components.ID("{9b6f4160-45be-11e4-916c-0800200c9a66}"),

  type: Components.interfaces.nsISupportsString.TYPE_STRING,
  data: "main process",
  toString: function() {
    return this.data;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TestProcessDirective]);
