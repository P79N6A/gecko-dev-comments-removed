



const {Cu} = require("chrome");
const ObservableObject = require("devtools/shared/observable-object");
const {Simulator} = Cu.import("resource://gre/modules/devtools/Simulator.jsm");

let store = new ObservableObject({versions:[]});

function feedStore() {
  store.object.versions = Simulator.availableNames().map(name => {
    let simulator = Simulator.getByName(name);
    return {
      version: name,
      label: simulator ? name : "Unknown"
    }
  });
}

Simulator.on("register", feedStore);
Simulator.on("unregister", feedStore);
feedStore();

module.exports = store;
