



































function FooComponent() {
}
FooComponent.prototype =
{
  
  getInterfaces: function getInterfaces(aCount) {
    var interfaces = [Components.interfaces.nsIClassInfo];
    aCount.value = interfaces.length;
    return interfaces;
  },

  
  getHelperForLanguage: function getHelperForLanguage(aLanguage) {
    return null;
  },

  
  classDescription: "Module importer test 1",

  
  classID: Components.ID("{6b933fe6-6eba-4622-ac86-e4f654f1b474}"),

  
  contractID: "@mozilla.org/tests/module-importer;1",

  
  implementationLanguage: Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT,

  
  flags: 0,

  
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
  
  getInterfaces: function getInterfaces(aCount) {
    var interfaces = [Components.interfaces.nsIClassInfo];
    aCount.value = interfaces.length;
    return interfaces;
  },

  
  getHelperForLanguage: function getHelperForLanguage(aLanguage) {
    return null;
  },

  
  classDescription: "Module importer test 2",

  
  classID: Components.ID("{708a896a-b48d-4bff-906e-fc2fd134b296}"),

  
  contractID: "@mozilla.org/tests/module-importer;2",

  
  implementationLanguage: Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT,

  
  flags: 0
};

Components.utils.import("rel:XPCOMUtils.jsm");

var NSGetModule = XPCOMUtils.generateNSGetModule([
  {
    className:  FooComponent.prototype.classDescription,
    cid:        FooComponent.prototype.classID,
    contractID: FooComponent.prototype.contractID,
    factory: XPCOMUtils.generateFactory(FooComponent)
  },
  {
    className:  BarComponent.prototype.classDescription,
    cid:        BarComponent.prototype.classID,
    contractID: BarComponent.prototype.contractID,
    factory: XPCOMUtils.generateFactory(
      BarComponent,
      [Components.interfaces.nsIClassInfo]
    )
  }
], null, null);
