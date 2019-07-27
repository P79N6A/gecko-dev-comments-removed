



"use strict";

this.EXPORTED_SYMBOLS = [
  "MockRegistrar",
];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr, manager: Cm} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");
let logger = Log.repository.getLogger("MockRegistrar");

this.MockRegistrar = Object.freeze({
  _registeredComponents: new Map(),
  _originalCIDs: new Map(),
  get registrar() {
    return Cm.QueryInterface(Ci.nsIComponentRegistrar);
  },

  














  register(contractID, mock, args) {
    let originalCID = this._originalCIDs.get(contractID);
    if (!originalCID) {
      originalCID = this.registrar.contractIDToCID(contractID);
      this._originalCIDs.set(contractID, originalCID);
    }

    let originalFactory = Cm.getClassObject(originalCID, Ci.nsIFactory);

    let factory = {
      createInstance(outer, iid) {
        if (outer) {
          throw Cr.NS_ERROR_NO_AGGREGATION;
        }

        let wrappedMock;
        if (mock.prototype && mock.prototype.constructor) {
          wrappedMock = Object.create(mock.prototype);
          mock.apply(wrappedMock, args);
        } else {
          wrappedMock = mock;
        }

        try {
          let genuine = originalFactory.createInstance(outer, iid);
          wrappedMock._genuine = genuine;
        } catch(ex) {
          logger.info("Creating original instance failed", ex);
        }

        return wrappedMock.QueryInterface(iid);
      },
      lockFactory(lock) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIFactory])
    };

    this.registrar.unregisterFactory(originalCID, originalFactory);
    this.registrar.registerFactory(originalCID,
                                   "A Mock for " + contractID,
                                   contractID,
                                   factory);

    this._registeredComponents.set(originalCID, {
      contractID: contractID,
      factory: factory,
      originalFactory: originalFactory
    });

    return originalCID;
  },

  




  unregister(cid) {
    let component = this._registeredComponents.get(cid);
    if (!component) {
      return;
    }

    this.registrar.unregisterFactory(cid, component.factory);
    if (component.originalFactory) {
      this.registrar.registerFactory(cid,
                                     "",
                                     component.contractID,
                                     component.originalFactory);
    }

    this._registeredComponents.delete(cid);
  },

  


  unregisterAll() {
    for (let cid of this._registeredComponents.keys()) {
      this.unregister(cid);
    }
  }

});
