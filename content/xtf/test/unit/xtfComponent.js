



































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");


function NOT_IMPLEMENTED() {
  throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
}

const C_i = Components.interfaces;

const nsIXTFElementFactory        = C_i.nsIXTFElementFactory;
const nsIXTFElement               = C_i.nsIXTFElement;
const nsIXTFPrivate               = C_i.nsIXTFPrivate;
const nsIXTFAttributeHandler      = C_i.nsIXTFAttributeHandler;
const mozIJSSubScriptLoader       = C_i.mozIJSSubScriptLoader;
const nsIProgrammingLanguage      = C_i.nsIProgrammingLanguage;
const nsIClassInfo                = C_i.nsIClassInfo;
const nsIComponentRegistrar       = C_i.nsIComponentRegistrar;
const nsIFactory                  = C_i.nsIFactory;
const nsIModule                   = C_i.nsIModule;
const nsISupports                 = C_i.nsISupports;




function ObjectWrapper(object) {
  this.wrappedJSObject = object;
}


const FooInner = {
  bar: {
    testpassed: true
  },

  handle_default: {
    testpassed: false
  }
}

function FooElement(aLocalName) {
  this._wrapper = null;

  
  this.inner = new ObjectWrapper(FooInner[aLocalName]);
}
FooElement.prototype =
{
  
  onCreated: function onCreated(aWrapper) {
    this._wrapper = aWrapper;
    aWrapper.notificationMask = 0;
  },

  
  onDestroyed: function onDestroyed() {
  },

  
  isAttributeHandler: false,

  
  getScriptingInterfaces: function getScriptingInterfaces(aCount) {
    var interfaces = [];
    aCount.value = interfaces.length;
    return interfaces;
  },

  
  willChangeDocument: NOT_IMPLEMENTED,
  documentChanged: NOT_IMPLEMENTED,
  willChangeParent: NOT_IMPLEMENTED,
  parentChanged: NOT_IMPLEMENTED,
  willInsertChild: NOT_IMPLEMENTED,
  childInserted: NOT_IMPLEMENTED,
  willAppendChild: NOT_IMPLEMENTED,
  childAppended: NOT_IMPLEMENTED,
  willRemoveChild: NOT_IMPLEMENTED,
  childRemoved: NOT_IMPLEMENTED,
  willSetAttribute: NOT_IMPLEMENTED,
  attributeSet: NOT_IMPLEMENTED,
  willRemoveAttribute: NOT_IMPLEMENTED,
  attributeRemoved: NOT_IMPLEMENTED,

  beginAddingChildren: NOT_IMPLEMENTED,
  doneAddingChildren: NOT_IMPLEMENTED,

  handleDefault: NOT_IMPLEMENTED,
  cloneState: NOT_IMPLEMENTED,

  get accesskeyNode() {
    return null;
  },
  performAccesskey: NOT_IMPLEMENTED,

  
  QueryInterface: function QueryInterface(aIID) {
    if (aIID.equals(nsIXTFPrivate) ||
        aIID.equals(nsIXTFElement) ||
        aIID.equals(nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
    return null;
  }
};

function FooElementFactory() {}
FooElementFactory.prototype =
{
  classID: Components.ID("{f367b65d-6b7f-4a7f-9a4b-8bde0ff4ef10}"),

  
  createElement: function createElement(aLocalName) {
    var rv = null;
    switch (aLocalName) {
      case "bar":
        rv = new FooElement(aLocalName);
        rv.handleDefault = function handleDefault(aEvent) {
          this.inner.wrappedJSObject.testpassed = false;
        }
        break;

      case "handle_default":
        var rv = new FooElement(aLocalName);
        rv.onCreated = function onCreated(aWrapper) {
          this._wrapper = aWrapper;
          aWrapper.notificationMask = nsIXTFElement.NOTIFY_HANDLE_DEFAULT;
        }
        rv.handleDefault = function handleDefault(aEvent) {
          this.inner.wrappedJSObject.testpassed = true;
        }
        break;
    }

    return rv ? rv.QueryInterface(nsIXTFElement) : null;
  },

  
  QueryInterface: function QueryInterface(aIID) {
    if (aIID.equals(nsIXTFElementFactory) ||
        aIID.equals(nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
    return null;
  }
};



const NSGetFactory = XPCOMUtils.generateNSGetFactory([FooElementFactory]);
