







































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const CATEGORY_NAME = "test-cat";



function CatRegisteredComponent() {}
CatRegisteredComponent.prototype = {
  classDescription: "CatRegisteredComponent",
  classID:          Components.ID("{163cd427-1f08-4416-a291-83ea71127b0e}"),
  contractID:       "@unit.test.com/cat-registered-component;1",
  QueryInterface: XPCOMUtils.generateQI([]),
  _xpcom_categories: [
    { category: CATEGORY_NAME }
  ]
};



function CatAppRegisteredComponent() {}
CatAppRegisteredComponent.prototype = {
  classDescription: "CatAppRegisteredComponent",
  classID:          Components.ID("{b686dc84-f42e-4c94-94fe-89d0ac899578}"),
  contractID:       "@unit.test.com/cat-app-registered-component;1",
  QueryInterface: XPCOMUtils.generateQI([]),
  _xpcom_categories: [
    { category: CATEGORY_NAME,
      apps: [  "{adb42a9a-0d19-4849-bf4d-627614ca19be}" ]
    }
  ]
};




function CatUnregisteredComponent() {}
CatUnregisteredComponent.prototype = {
  classDescription: "CatUnregisteredComponent",
  classID:          Components.ID("{c31a552b-0228-4a1a-8cdf-d8aab7d4eff8}"),
  contractID:       "@unit.test.com/cat-unregistered-component;1",
  QueryInterface: XPCOMUtils.generateQI([]),
  _xpcom_categories: [
    { category: CATEGORY_NAME,
      apps: [  "{e84fce36-6ef6-435c-bf63-979a8811dcd4}" ]
    }
  ]
};


let components = [
  CatRegisteredComponent,
  CatAppRegisteredComponent,
  CatUnregisteredComponent,
];
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule(components);
}
