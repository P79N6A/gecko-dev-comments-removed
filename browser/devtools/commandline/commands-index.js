



"use strict";

const { createSystem, connectFront, disconnectFront } = require("gcli/system");
const { GcliFront } = require("devtools/server/actors/gcli");





exports.baseModules = [
  "gcli/types/delegate",
  "gcli/types/selection",
  "gcli/types/array",

  "gcli/types/boolean",
  "gcli/types/command",
  "gcli/types/date",
  "gcli/types/file",
  "gcli/types/javascript",
  "gcli/types/node",
  "gcli/types/number",
  "gcli/types/resource",
  "gcli/types/setting",
  "gcli/types/string",
  "gcli/types/union",
  "gcli/types/url",

  "gcli/fields/fields",
  "gcli/fields/delegate",
  "gcli/fields/selection",

  "gcli/ui/focus",
  "gcli/ui/intro",

  "gcli/converters/converters",
  "gcli/converters/basic",
  "gcli/converters/terminal",

  "gcli/languages/command",
  "gcli/languages/javascript",

  "gcli/commands/clear",
  "gcli/commands/context",
  "gcli/commands/help",
  "gcli/commands/pref",
];





exports.devtoolsModules = [
  "devtools/tilt/tilt-commands",
  "gcli/commands/addon",
  "gcli/commands/appcache",
  "gcli/commands/calllog",
  "gcli/commands/cmd",
  "gcli/commands/cookie",
  "gcli/commands/csscoverage",
  "gcli/commands/folder",
  "gcli/commands/highlight",
  "gcli/commands/inject",
  "gcli/commands/jsb",
  "gcli/commands/listen",
  "gcli/commands/media",
  "gcli/commands/pagemod",
  "gcli/commands/paintflashing",
  "gcli/commands/restart",
  "gcli/commands/rulers",
  "gcli/commands/screenshot",
  "gcli/commands/tools",
];





const defaultTools = require("definitions").defaultTools;
exports.devtoolsToolModules = defaultTools.map(def => def.commands || [])
                                 .reduce((prev, curr) => prev.concat(curr), []);




exports.addAllItemsByModule = function(system) {
  system.addItemsByModule(exports.baseModules, { delayedLoad: true });
  system.addItemsByModule(exports.devtoolsModules, { delayedLoad: true });
  system.addItemsByModule(exports.devtoolsToolModules, { delayedLoad: true });

  const { mozDirLoader } = require("gcli/commands/cmd");
  system.addItemsByModule("mozcmd", { delayedLoad: true, loader: mozDirLoader });
};





var linksForTarget = new WeakMap();





var customProperties = [ "buttonId", "buttonClass", "tooltipText" ];





exports.getSystem = function(target) {
  const existingLinks = linksForTarget.get(target);
  if (existingLinks != null) {
    existingLinks.refs++;
    return existingLinks.promise;
  }

  const system = createSystem({ location: "client" });

  exports.addAllItemsByModule(system);

  
  const links = {
    refs: 1,
    system,
    promise: system.load().then(() => {
      return GcliFront.create(target).then(front => {
        links.front = front;
        return connectFront(system, front, customProperties).then(() => system);
      });
    })
  };

  linksForTarget.set(target, links);
  return links.promise;
};





exports.releaseSystem = function(target) {
  const links = linksForTarget.get(target);
  if (links == null) {
    throw new Error("releaseSystem called for unknown target");
  }

  links.refs--;
  if (links.refs === 0) {
    disconnectFront(links.system, links.front);
    links.system.destroy();
    linksForTarget.delete(target);
  }
};
