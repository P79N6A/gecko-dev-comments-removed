




const Cu = Components.utils;
const toolId = "test-tool";

let tempScope = {};
Cu.import("resource:///modules/devtools/EventEmitter.jsm", tempScope);
let EventEmitter = tempScope.EventEmitter;
Cu.import("resource:///modules/devtools/Target.jsm", tempScope);
let TargetFactory = tempScope.TargetFactory;

function test() {
  addTab("about:blank", function(aBrowser, aTab) {
    runTests(aTab);
  });
}

function runTests(aTab) {
  let toolDefinition = {
    id: toolId,
    isTargetSupported: function() true,
    killswitch: "devtools.test-tool.enabled",
    url: "about:blank",
    label: "someLabel",
    build: function(iframeWindow, toolbox) {
      let panel = new DevToolPanel(iframeWindow, toolbox);
      return panel.open();
    },
  };

  ok(gDevTools, "gDevTools exists");
  is(gDevTools.getToolDefinitionMap().has(toolId), false,
    "The tool is not registered");

  gDevTools.registerTool(toolDefinition);
  is(gDevTools.getToolDefinitionMap().has(toolId), true,
    "The tool is registered");

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.showToolbox(target, toolId).then(function(toolbox) {
    is(toolbox.target, target, "toolbox target is correct");
    is(toolbox._host.hostTab, gBrowser.selectedTab, "toolbox host is correct");
    continueTests(toolbox);
  }).then(null, console.error);
}

function continueTests(toolbox, panel) {
  ok(toolbox.getCurrentPanel(), "panel value is correct");
  is(toolbox.currentToolId, toolId, "toolbox _currentToolId is correct");

  let toolDefinitions = gDevTools.getToolDefinitionMap();
  is(toolDefinitions.has(toolId), true, "The tool is in gDevTools");

  let toolDefinition = toolDefinitions.get(toolId);
  is(toolDefinition.id, toolId, "toolDefinition id is correct");

  gDevTools.unregisterTool(toolId);
  is(gDevTools.getToolDefinitionMap().has(toolId), false,
    "The tool is no longer registered");

  toolbox.destroy().then(function() {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    ok(gDevTools._toolboxes.get(target) == null, "gDevTools doesn't know about target");
    ok(toolbox._target == null, "toolbox doesn't know about target.");

    finishUp();
  }).then(null, console.error);
}

function finishUp() {
  tempScope = null;
  gBrowser.removeCurrentTab();
  finish();
}









function DevToolPanel(iframeWindow, toolbox) {
  EventEmitter.decorate(this);

  this._toolbox = toolbox;

  





}

DevToolPanel.prototype = {
  open: function() {
    let deferred = Promise.defer();

    executeSoon(function() {
      this._isReady = true;
      this.emit("ready");
      deferred.resolve(this);
    }.bind(this));

    return deferred.promise;
  },

  get target() this._toolbox.target,

  get toolbox() this._toolbox,

  get isReady() this._isReady,

  _isReady: false,

  destroy: function DTI_destroy() {
    return Promise.defer(null);
  },
};
