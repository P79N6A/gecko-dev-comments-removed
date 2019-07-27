



const { Cu } = require("chrome");
const { AddonManager } = Cu.import("resource://gre/modules/AddonManager.jsm");
const { EventEmitter } = Cu.import("resource://gre/modules/devtools/event-emitter.js");
loader.lazyRequireGetter(this, "ConnectionManager", "devtools/client/connection-manager", true);
loader.lazyRequireGetter(this, "AddonSimulatorProcess", "devtools/webide/simulator-process", true);
loader.lazyRequireGetter(this, "OldAddonSimulatorProcess", "devtools/webide/simulator-process", true);
const promise = require("promise");

const SimulatorRegExp = new RegExp(Services.prefs.getCharPref("devtools.webide.simulatorAddonRegExp"));

let Simulators = {
  
  
  
  getAll() {
    let deferred = promise.defer();
    AddonManager.getAllAddons(addons => {
      let simulators = [];
      for (let addon of addons) {
        if (SimulatorRegExp.exec(addon.id)) {
          simulators.push(new Simulator(addon));
        }
      }
      
      simulators.sort((a, b) => {
        return a.name.toLowerCase().localeCompare(b.name.toLowerCase())
      });
      deferred.resolve(simulators);
    });
    return deferred.promise;
  },
}
EventEmitter.decorate(Simulators);
exports.Simulators = Simulators;

function update() {
  Simulators.emit("updated");
}
AddonManager.addAddonListener({
  onEnabled: update,
  onDisabled: update,
  onInstalled: update,
  onUninstalled: update
});


function Simulator(addon) {
  this.addon = addon;
}

Simulator.prototype = {
  launch() {
    
    if (this.process) {
      return this.kill().then(this.launch.bind(this));
    }

    let options = {
      port: ConnectionManager.getFreeTCPPort()
    };

    if (this.version <= "1.3") {
      
      this.process = new OldAddonSimulatorProcess(this.addon, options);
    } else {
      this.process = new AddonSimulatorProcess(this.addon, options);
    }
    this.process.run();

    return promise.resolve(options.port);
  },

  kill() {
    let process = this.process;
    if (!process) {
      return promise.resolve();
    }
    this.process = null;
    return process.kill();
  },

  get id() {
    return this.addon.id;
  },

  get name() {
    return this.addon.name.replace(" Simulator", "");
  },

  get version() {
    return this.name.match(/\d+\.\d+/)[0];
  },
};
