



































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function FooComponent() {
  this.wrappedJSObject = this;
}
FooComponent.prototype =
{
  
  classDescription:  "Foo Component",
  classID:           Components.ID("{6b933fe6-6eba-4622-ac86-e4f654f1b474}"),
  contractID:       "@mozilla.org/tests/module-importer;1",

  
  implementationLanguage: Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT,
  flags: 0,

  getInterfaces: function getInterfaces(aCount) {
    var interfaces = [Components.interfaces.nsIClassInfo];
    aCount.value = interfaces.length;

    
    var threw = true;
    try {
      thereIsNoSuchIdentifier;
      threw = false;
    } catch (ex) {
      do_check_true(ex.lineNumber == 60);
    }
    do_check_true(threw);
    
    return interfaces;
  },

  getHelperForLanguage: function getHelperForLanguage(aLanguage) {
    return null;
  },

  
  QueryInterface: function QueryInterface(aIID) {
    if (aIID.equals(Components.interfaces.nsIClassInfo) ||
        aIID.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
};

function BarComponent() {
}
BarComponent.prototype =
{
  
  classDescription: "Module importer test 2",
  classID: Components.ID("{708a896a-b48d-4bff-906e-fc2fd134b296}"),
  contractID: "@mozilla.org/tests/module-importer;2",

  
  implementationLanguage: Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT,
  flags: 0,

  getInterfaces: function getInterfaces(aCount) {
    var interfaces = [Components.interfaces.nsIClassInfo];
    aCount.value = interfaces.length;
    return interfaces;
  },

  getHelperForLanguage: function getHelperForLanguage(aLanguage) {
    return null;
  },

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIClassInfo])
};

function do_check_true(cond, text) {
  
  
  
  if (!cond)
    throw "Failed check: " + text;
}

var gComponentsArray = [FooComponent, BarComponent];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(gComponentsArray);
