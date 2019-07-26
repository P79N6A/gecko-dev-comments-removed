



const {Cu} = require("chrome");
const ObservableObject = require("devtools/shared/observable-object");
const {Simulator} = Cu.import("resource://gre/modules/devtools/Simulator.jsm");

let store = new ObservableObject({versions:[]});

function feedStore() {
  store.object.versions = Simulator.availableVersions().map(v => {
    return {version:v}
  });
}

Simulator.on("register", feedStore);
Simulator.on("unregister", feedStore);
feedStore();

module.exports = store;
