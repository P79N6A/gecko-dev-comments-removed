



"use strict";

module.metadata = {
  "stability": "experimental"
};

const { Cu, Cc, Ci } = require("chrome");
const { Class } = require("../sdk/core/heritage");
const { Disposable, setup } = require("../sdk/core/disposable");
const { contract, validate } = require("../sdk/util/contract");
const { each, pairs, values } = require("../sdk/util/sequence");

const { gDevTools } = Cu.import("resource:///modules/devtools/gDevTools.jsm", {});




const registerSDKURI = () => {
  const ioService = Cc['@mozilla.org/network/io-service;1']
                      .getService(Ci.nsIIOService);
  const resourceHandler = ioService.getProtocolHandler("resource")
                                   .QueryInterface(Ci.nsIResProtocolHandler);

  const uri = module.uri.replace("dev/toolbox.js", "");
  resourceHandler.setSubstitution("sdk", ioService.newURI(uri, null, null));
};

registerSDKURI();


const Tool = Class({
  extends: Disposable,
  setup: function(params={}) {
    const { panels } = validate(this, params);

    this.panels = panels;

    each(([key, Panel]) => {
      const { url, label, tooltip, icon } = validate(Panel.prototype);
      const { id } = Panel.prototype;

      gDevTools.registerTool({
        id: id,
        url: "about:blank",
        label: label,
        tooltip: tooltip,
        icon: icon,
        isTargetSupported: target => target.isLocalTab,
        build: (window, toolbox) => {
          const panel = new Panel();
          setup(panel, { window: window,
                         toolbox: toolbox,
                         url: url });

          return panel.ready();
        }
      });
    }, pairs(panels));
  },
  dispose: function() {
    each(Panel => gDevTools.unregisterTool(Panel.prototype.id),
         values(this.panels));
  }
});

validate.define(Tool, contract({
  panels: {
    is: ["object", "undefined"]
  }
}));
exports.Tool = Tool;
