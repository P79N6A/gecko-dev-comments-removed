





"use strict";

let { classes: Cc, interfaces: Ci, utils: Cu } = Components;

let { XPCOMUtils } = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {});

XPCOMUtils.defineLazyModuleGetter(this, "console",
                                  "resource://gre/modules/devtools/Console.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CommandUtils",
                                  "resource:///modules/devtools/DeveloperToolbar.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "require", function() {
  let { require } = Cu.import("resource://gre/modules/devtools/Require.jsm", {});
  Cu.import("resource://gre/modules/devtools/gcli.jsm", {});
  return require;
});

XPCOMUtils.defineLazyGetter(this, "canon", () => require("gcli/canon"));
XPCOMUtils.defineLazyGetter(this, "Requisition", () => require("gcli/cli").Requisition);
XPCOMUtils.defineLazyGetter(this, "util", () => require("util/util"));








function GcliActor(connection, parentActor) {
  this.connection = connection;
}

GcliActor.prototype.actorPrefix = "gcli";

GcliActor.prototype.disconnect = function() {
};

GcliActor.prototype.getCommandSpecs = function(request) {
  return { commandSpecs: canon.getCommandSpecs() };
};

GcliActor.prototype.execute = function(request) {
  let chromeWindow = Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType);
  let contentWindow = chromeWindow.gBrowser.selectedTab.linkedBrowser.contentWindow;

  let environment = CommandUtils.createEnvironment(chromeWindow.document,
                                                   contentWindow.document);

  let requisition = new Requisition(environment);
  requisition.updateExec(request.typed).then(output => {
    return output.promise.then(() => {
      this.connection.send({
        from: this.actorID,
        requestId: request.requestId,
        data: output.data,
        type: output.type,
        error: output.error
      });
    });
  }).then(null, console.error);
};

GcliActor.prototype.requestTypes = {
  getCommandSpecs: GcliActor.prototype.getCommandSpecs,
  execute: GcliActor.prototype.execute,
};

addTabActor(GcliActor, "gcliActor");
addGlobalActor(GcliActor, "gcliActor");
