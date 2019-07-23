







































const nsISupports = Components.interfaces.nsISupports;
const nsIEcho = Components.interfaces.nsIEcho;
const nsIXPCTestIn = Components.interfaces.nsIXPCTestIn;

const CLASS_ID = Components.ID("{9e45a36d-7cf7-4f2a-a415-d0b07e54945b}");
const CLASS_NAME = "XPConnect Tests in Javascript";
const CONTRACT_ID = "@mozilla.org/javaxpcom/tests/xpc;1";






function XPCTests() {
};


XPCTests.prototype = {

  
  
  
  
  QueryInterface: function(aIID) {
    if (!aIID.equals(nsIEcho) &&
        !aIID.equals(nsIXPCTestIn) &&
        !aIID.equals(nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  },

  
  
  

  In2OutOneString: function(input) {
    return input;
  },

  In2OutOneDOMString: function(input) {
    return input;
  },

  In2OutOneAString: function(input) {
    return input;
  },

  In2OutOneUTF8String: function(input) {
    return input;
  },

  In2OutOneCString: function(input) {
    return input;
  },

  _string: null,

  SetAString: function(str) {
    _string = str;
  },

  GetAString: function() {
    return _string;
  },

  
  
  

  EchoLong: function(l) {
    return l;
  },

  EchoShort: function(a) {
    return a;
  },

  EchoChar: function(c) {
    return c;
  },

  EchoBoolean: function(b) {
    return b;
  },

  EchoOctet: function(o) {
    return o;
  },

  EchoLongLong: function(ll) {
    return ll;
  },

  EchoUnsignedShort: function(us) {
    return us;
  },

  EchoUnsignedLong: function(ul) {
    return ul;
  },

  EchoFloat: function(f) {
    return f;
  },

  EchoDouble: function(d) {
    return d;
  },

  EchoWchar: function(wc) {
    return wc;
  },

  EchoString: function(ws) {
    return ws;
  },

  EchoPRBool: function(b) {
    return b;
  },

  EchoPRInt32: function(l) {
    return l;
  },

  EchoPRInt16: function(l) {
    return l;
  },

  EchoPRInt64: function(i) {
    return i;
  },

  EchoPRUint8: function(i) {
    return i;
  },

  EchoPRUint16: function(i) {
    return i;
  },

  EchoPRUint32: function(i) {
    return i;
  },

  EchoPRUint64: function(i) {
    return i;
  },

  EchoVoid: function() {}

};




var XPCTestsFactory = {
  createInstance: function (aOuter, aIID)
  {
    if (aOuter != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return (new XPCTests()).QueryInterface(aIID);
  }
};




var XPCTestsModule = {
  _firstTime: true,
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType)
  {
    aCompMgr = aCompMgr.
        QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME, 
        CONTRACT_ID, aFileSpec, aLocation, aType);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType)
  {
    aCompMgr = aCompMgr.
        QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CLASS_ID, aLocation);        
  },
  
  getClassObject: function(aCompMgr, aCID, aIID)
  {
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    if (aCID.equals(CLASS_ID))
      return XPCTestsFactory;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(aCompMgr)
  {
    return true;
  }
};




function NSGetModule(aCompMgr, aFileSpec)
{
  return XPCTestsModule;
}
