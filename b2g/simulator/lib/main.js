




const { Cc, Ci, Cu } = require("chrome");

const { SimulatorProcess } = require("./simulator-process");
const { Promise: promise } = Cu.import("resource://gre/modules/Promise.jsm", {});
const { Simulator } = Cu.import("resource://gre/modules/devtools/Simulator.jsm");
const { AddonManager } = Cu.import("resource://gre/modules/AddonManager.jsm", {});

let process;

function launch({ port }) {
  
  if (process) {
    return close().then(launch.bind(null, { port: port }));
  }

  process = SimulatorProcess();
  process.remoteDebuggerPort = port;
  process.run();

  return promise.resolve();
}

function close() {
  if (!process) {
    return promise.resolve();
  }
  let p = process;
  process = null;
  return p.kill();
}

let name;

AddonManager.getAddonByID(require("addon").id, function (addon) {
  name = addon.name.replace(" Simulator", "");

  Simulator.register(name, {
    
    
    appinfo: { label: name },
    launch: launch,
    close: close
  });
});

exports.shutdown = function () {
  Simulator.unregister(name);
  close();
}

