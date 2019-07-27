





"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");
Cu.import("resource://gre/modules/devtools/Console.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

devtools.lazyRequireGetter(this, "promise");
devtools.lazyRequireGetter(this, "EventEmitter",
                                 "devtools/toolkit/event-emitter");
devtools.lazyRequireGetter(this, "AnimationsFront",
                                 "devtools/server/actors/animation", true);

const require = devtools.require;

const STRINGS_URI = "chrome://browser/locale/devtools/animationinspector.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);


let gToolbox, gInspector;





let startup = Task.async(function*(inspector) {
  gInspector = inspector;
  gToolbox = inspector.toolbox;

  
  if (!typeof AnimationsPanel === "undefined") {
    throw new Error("AnimationsPanel was not loaded in the animationinspector window");
  }

  
  
  
  yield AnimationsController.initialize();
  yield AnimationsPanel.initialize();
});





let shutdown = Task.async(function*() {
  yield AnimationsController.destroy();
  
  if (typeof AnimationsPanel !== "undefined") {
    yield AnimationsPanel.destroy();
  }
  gToolbox = gInspector = null;
});


function setPanel(panel) {
  return startup(panel).catch(Cu.reportError);
}
function destroy() {
  return shutdown().catch(Cu.reportError);
}





















let AnimationsController = {
  PLAYERS_UPDATED_EVENT: "players-updated",

  initialize: Task.async(function*() {
    if (this.initialized) {
      return this.initialized.promise;
    }
    this.initialized = promise.defer();

    this.onPanelVisibilityChange = this.onPanelVisibilityChange.bind(this);
    this.onNewNodeFront = this.onNewNodeFront.bind(this);
    this.onAnimationMutations = this.onAnimationMutations.bind(this);

    let target = gToolbox.target;
    this.animationsFront = new AnimationsFront(target.client, target.form);

    
    this.hasToggleAll = yield target.actorHasMethod("animations", "toggleAll");
    this.hasSetCurrentTime = yield target.actorHasMethod("animationplayer",
                                                         "setCurrentTime");
    this.hasMutationEvents = yield target.actorHasMethod("animations",
                                                         "stopAnimationPlayerUpdates");
    this.hasSetPlaybackRate = yield target.actorHasMethod("animationplayer",
                                                          "setPlaybackRate");

    if (this.destroyed) {
      console.warn("Could not fully initialize the AnimationsController");
      return;
    }

    this.startListeners();
    yield this.onNewNodeFront();

    this.initialized.resolve();
  }),

  destroy: Task.async(function*() {
    if (!this.initialized) {
      return;
    }

    if (this.destroyed) {
      return this.destroyed.promise;
    }
    this.destroyed = promise.defer();

    this.stopListeners();
    yield this.destroyAnimationPlayers();
    this.nodeFront = null;

    if (this.animationsFront) {
      this.animationsFront.destroy();
      this.animationsFront = null;
    }

    this.destroyed.resolve();
  }),

  startListeners: function() {
    
    
    gInspector.selection.on("new-node-front", this.onNewNodeFront);
    gInspector.sidebar.on("select", this.onPanelVisibilityChange);
    gToolbox.on("select", this.onPanelVisibilityChange);
  },

  stopListeners: function() {
    gInspector.selection.off("new-node-front", this.onNewNodeFront);
    gInspector.sidebar.off("select", this.onPanelVisibilityChange);
    gToolbox.off("select", this.onPanelVisibilityChange);
    if (this.isListeningToMutations) {
      this.animationsFront.off("mutations", this.onAnimationMutations);
    }
  },

  isPanelVisible: function() {
    return gToolbox.currentToolId === "inspector" &&
           gInspector.sidebar &&
           gInspector.sidebar.getCurrentTabID() == "animationinspector";
  },

  onPanelVisibilityChange: Task.async(function*(e, id) {
    if (this.isPanelVisible()) {
      this.onNewNodeFront();
      this.startAllAutoRefresh();
    } else {
      this.stopAllAutoRefresh();
    }
  }),

  onNewNodeFront: Task.async(function*() {
    
    if (!this.isPanelVisible() || this.nodeFront === gInspector.selection.nodeFront) {
      return;
    }

    let done = gInspector.updating("animationscontroller");

    if(!gInspector.selection.isConnected() ||
       !gInspector.selection.isElementNode()) {
      yield this.destroyAnimationPlayers();
      this.emit(this.PLAYERS_UPDATED_EVENT);
      done();
      return;
    }

    this.nodeFront = gInspector.selection.nodeFront;
    yield this.refreshAnimationPlayers(this.nodeFront);
    this.emit(this.PLAYERS_UPDATED_EVENT, this.animationPlayers);

    done();
  }),

  


  toggleAll: function() {
    if (!this.hasToggleAll) {
      return promis.resolve();
    }

    return this.animationsFront.toggleAll().catch(Cu.reportError);
  },

  
  
  
  
  animationPlayers: [],

  refreshAnimationPlayers: Task.async(function*(nodeFront) {
    yield this.destroyAnimationPlayers();

    this.animationPlayers = yield this.animationsFront.getAnimationPlayersForNode(nodeFront);
    this.startAllAutoRefresh();

    
    
    if (!this.isListeningToMutations && this.hasMutationEvents) {
      this.animationsFront.on("mutations", this.onAnimationMutations);
      this.isListeningToMutations = true;
    }
  }),

  onAnimationMutations: Task.async(function*(changes) {
    
    
    for (let {type, player} of changes) {
      if (type === "added") {
        this.animationPlayers.push(player);
        player.startAutoRefresh();
      }

      if (type === "removed") {
        player.stopAutoRefresh();
        yield player.release();
        let index = this.animationPlayers.indexOf(player);
        this.animationPlayers.splice(index, 1);
      }
    }

    
    this.emit(this.PLAYERS_UPDATED_EVENT, this.animationPlayers);
  }),

  startAllAutoRefresh: function() {
    for (let front of this.animationPlayers) {
      front.startAutoRefresh();
    }
  },

  stopAllAutoRefresh: function() {
    for (let front of this.animationPlayers) {
      front.stopAutoRefresh();
    }
  },

  destroyAnimationPlayers: Task.async(function*() {
    
    
    
    if (this.hasMutationEvents) {
      yield this.animationsFront.stopAnimationPlayerUpdates();
    }
    this.stopAllAutoRefresh();
    for (let front of this.animationPlayers) {
      yield front.release();
    }
    this.animationPlayers = [];
  })
};

EventEmitter.decorate(AnimationsController);
