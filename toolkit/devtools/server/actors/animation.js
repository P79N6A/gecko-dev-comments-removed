



"use strict";





















const {Cu} = require("chrome");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
const {setInterval, clearInterval} = require("sdk/timers");
const protocol = require("devtools/server/protocol");
const {ActorClass, Actor, FrontClass, Front, Arg, method, RetVal, types} = protocol;
const {NodeActor} = require("devtools/server/actors/inspector");
const events = require("sdk/event/core");

const PLAYER_DEFAULT_AUTO_REFRESH_TIMEOUT = 500; 










let AnimationPlayerActor = ActorClass({
  typeName: "animationplayer",

  







  initialize: function(animationsActor, player, playerIndex) {
    Actor.prototype.initialize.call(this, animationsActor.conn);

    this.player = player;
    this.node = player.effect.target;
    this.playerIndex = playerIndex;
    this.styles = this.node.ownerDocument.defaultView.getComputedStyle(this.node);
  },

  destroy: function() {
    this.player = this.node = this.styles = null;
    Actor.prototype.destroy.call(this);
  },

  



  release: method(function() {}, {release: true}),

  form: function(detail) {
    if (detail === "actorid") {
      return this.actorID;
    }

    let data = this.getCurrentState();
    data.actor = this.actorID;

    return data;
  },

  






  getPlayerIndex: function() {
    let names = this.styles.animationName;

    
    
    
    
    
    
    if (!names) {
      return this.playerIndex;
    }

    
    if (names.contains(",") === -1) {
      return 0;
    }

    
    
    names = names.split(",").map(n => n.trim());
    for (let i = 0; i < names.length; i ++) {
      if (names[i] === this.player.effect.name) {
        return i;
      }
    }
  },

  






  getDuration: function() {
    let durationText;
    if (this.styles.animationDuration !== "0s") {
      durationText = this.styles.animationDuration;
    } else if (this.styles.transitionDuration !== "0s") {
      durationText = this.styles.transitionDuration;
    } else {
      return null;
    }

    
    
    if (durationText.indexOf(",") !== -1) {
      durationText = durationText.split(",")[this.getPlayerIndex()];
    }

    return parseFloat(durationText) * 1000;
  },

  






  getDelay: function() {
    let delayText;
    if (this.styles.animationDelay !== "0s") {
      delayText = this.styles.animationDelay;
    } else if (this.styles.transitionDelay !== "0s") {
      delayText = this.styles.transitionDelay;
    } else {
      return 0;
    }

    if (delayText.indexOf(",") !== -1) {
      delayText = delayText.split(",")[this.getPlayerIndex()];
    }

    return parseFloat(delayText) * 1000;
  },

  








  getIterationCount: function() {
    let iterationText = this.styles.animationIterationCount;
    if (iterationText.indexOf(",") !== -1) {
      iterationText = iterationText.split(",")[this.getPlayerIndex()];
    }

    return iterationText === "infinite"
           ? null
           : parseInt(iterationText, 10);
  },

  





  getCurrentState: method(function() {
    
    
    
    let newState = {
      
      startTime: this.player.startTime,
      currentTime: this.player.currentTime,
      playState: this.player.playState,
      playbackRate: this.player.playbackRate,
      name: this.player.effect.name,
      duration: this.getDuration(),
      delay: this.getDelay(),
      iterationCount: this.getIterationCount(),
      
      
      
      
      
      isRunningOnCompositor: this.player.isRunningOnCompositor
    };

    
    
    
    
    let sentState = {};
    if (this.currentState) {
      for (let key in newState) {
        if (typeof this.currentState[key] === "undefined" ||
            this.currentState[key] !== newState[key]) {
          sentState[key] = newState[key];
        }
      }
    } else {
      sentState = newState;
    }
    this.currentState = newState;

    return sentState;
  }, {
    request: {},
    response: {
      data: RetVal("json")
    }
  }),

  


  pause: method(function() {
    this.player.pause();
    return this.player.ready;
  }, {
    request: {},
    response: {}
  }),

  



  play: method(function() {
    this.player.play();
    return this.player.ready;
  }, {
    request: {},
    response: {}
  }),

  










  ready: method(function() {
    return this.player.ready;
  }, {
    request: {},
    response: {}
  }),

  


  setCurrentTime: method(function(currentTime) {
    this.player.currentTime = currentTime;
  }, {
    request: {
      currentTime: Arg(0, "number")
    },
    response: {}
  }),

  


  setPlaybackRate: method(function(playbackRate) {
    this.player.playbackRate = playbackRate;
  }, {
    request: {
      currentTime: Arg(0, "number")
    },
    response: {}
  })
});

let AnimationPlayerFront = FrontClass(AnimationPlayerActor, {
  AUTO_REFRESH_EVENT: "updated-state",

  initialize: function(conn, form, detail, ctx) {
    Front.prototype.initialize.call(this, conn, form, detail, ctx);

    this.state = {};
  },

  form: function(form, detail) {
    if (detail === "actorid") {
      this.actorID = form;
      return;
    }
    this._form = form;
    this.state = this.initialState;
  },

  destroy: function() {
    this.stopAutoRefresh();
    Front.prototype.destroy.call(this);
  },

  



  get initialState() {
    return {
      startTime: this._form.startTime,
      currentTime: this._form.currentTime,
      playState: this._form.playState,
      playbackRate: this._form.playbackRate,
      name: this._form.name,
      duration: this._form.duration,
      delay: this._form.delay,
      iterationCount: this._form.iterationCount,
      isRunningOnCompositor: this._form.isRunningOnCompositor
    }
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  




  startAutoRefresh: function(interval=PLAYER_DEFAULT_AUTO_REFRESH_TIMEOUT) {
    if (this.autoRefreshTimer) {
      return;
    }

    this.autoRefreshTimer = setInterval(this.refreshState.bind(this), interval);
  },

  


  stopAutoRefresh: function() {
    if (!this.autoRefreshTimer) {
      return;
    }

    clearInterval(this.autoRefreshTimer);
    this.autoRefreshTimer = null;
  },

  



  refreshState: Task.async(function*() {
    let data = yield this.getCurrentState();

    
    if (!this.autoRefreshTimer) {
      return;
    }

    if (this.currentStateHasChanged) {
      this.state = data;
      events.emit(this, this.AUTO_REFRESH_EVENT, this.state);
    }
  }),

  



  getCurrentState: protocol.custom(function() {
    this.currentStateHasChanged = false;
    return this._getCurrentState().then(data => {
      for (let key in this.state) {
        if (typeof data[key] === "undefined") {
          data[key] = this.state[key];
        } else if (data[key] !== this.state[key]) {
          this.currentStateHasChanged = true;
        }
      }
      return data;
    });
  }, {
    impl: "_getCurrentState"
  }),
});





types.addDictType("animationMutationChange", {
  
  type: "string",
  
  player: "animationplayer"
});




let AnimationsActor = exports.AnimationsActor = ActorClass({
  typeName: "animations",

  events: {
    "mutations" : {
      type: "mutations",
      changes: Arg(0, "array:animationMutationChange")
    }
  },

  initialize: function(conn, tabActor) {
    Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;

    this.onWillNavigate = this.onWillNavigate.bind(this);
    this.onNavigate = this.onNavigate.bind(this);
    this.onAnimationMutation = this.onAnimationMutation.bind(this);

    this.allAnimationsPaused = false;
    events.on(this.tabActor, "will-navigate", this.onWillNavigate);
    events.on(this.tabActor, "navigate", this.onNavigate);
  },

  destroy: function() {
    Actor.prototype.destroy.call(this);
    events.off(this.tabActor, "will-navigate", this.onWillNavigate);
    events.off(this.tabActor, "navigate", this.onNavigate);

    this.stopAnimationPlayerUpdates();
    this.tabActor = this.observer = this.actors = null;
  },

  



  disconnect: function() {
    this.destroy();
  },

  





  getAnimationPlayersForNode: method(function(nodeActor) {
    let animations = nodeActor.rawNode.getAnimations();

    
    
    this.actors = [];
    for (let i = 0; i < animations.length; i ++) {
      
      
      let actor = AnimationPlayerActor(this, animations[i], i);
      this.actors.push(actor);
    }

    
    
    
    
    this.stopAnimationPlayerUpdates();
    let win = nodeActor.rawNode.ownerDocument.defaultView;
    this.observer = new win.MutationObserver(this.onAnimationMutation);
    this.observer.observe(nodeActor.rawNode, {animations: true});

    return this.actors;
  }, {
    request: {
      actorID: Arg(0, "domnode")
    },
    response: {
      players: RetVal("array:animationplayer")
    }
  }),

  onAnimationMutation: function(mutations) {
    let eventData = [];

    for (let {addedAnimations, changedAnimations, removedAnimations} of mutations) {
      for (let player of removedAnimations) {
        
        
        
        
        
        
        if (player.playState !== "idle") {
          continue;
        }
        let index = this.actors.findIndex(a => a.player === player);
        eventData.push({
          type: "removed",
          player: this.actors[index]
        });
        this.actors.splice(index, 1);
      }

      for (let player of addedAnimations) {
        
        
        if (this.actors.find(a => a.player === player)) {
          continue;
        }
        let actor = AnimationPlayerActor(
          this, player, player.effect.target.getAnimations().indexOf(player));
        this.actors.push(actor);
        eventData.push({
          type: "added",
          player: actor
        });
      }
    }

    if (eventData.length) {
      events.emit(this, "mutations", eventData);
    }
  },

  




  stopAnimationPlayerUpdates: method(function() {
    if (this.observer && !Cu.isDeadWrapper(this.observer)) {
      this.observer.disconnect();
    }
  }, {
    request: {},
    response: {}
  }),

  







  getAllAnimationPlayers: function() {
    let animations = [];

    
    
    
    for (let window of this.tabActor.windows) {
      let root = window.document.body || window.document;
      for (let element of root.getElementsByTagNameNS("*", "*")) {
        animations = [...animations, ...element.getAnimations()];
      }
    }

    return animations;
  },

  onWillNavigate: function({isTopLevel}) {
    if (isTopLevel) {
      this.stopAnimationPlayerUpdates();
    }
  },

  onNavigate: function({isTopLevel}) {
    if (isTopLevel) {
      this.allAnimationsPaused = false;
    }
  },

  


  pauseAll: method(function() {
    let readyPromises = [];
    for (let player of this.getAllAnimationPlayers()) {
      player.pause();
      readyPromises.push(player.ready);
    }
    this.allAnimationsPaused = true;
    return promise.all(readyPromises);
  }, {
    request: {},
    response: {}
  }),

  



  playAll: method(function() {
    let readyPromises = [];
    for (let player of this.getAllAnimationPlayers()) {
      player.play();
      readyPromises.push(player.ready);
    }
    this.allAnimationsPaused = false;
    return promise.all(readyPromises);
  }, {
    request: {},
    response: {}
  }),

  toggleAll: method(function() {
    if (this.allAnimationsPaused) {
      return this.playAll();
    } else {
      return this.pauseAll();
    }
  }, {
    request: {},
    response: {}
  })
});

let AnimationsFront = exports.AnimationsFront = FrontClass(AnimationsActor, {
  initialize: function(client, {animationsActor}) {
    Front.prototype.initialize.call(this, client, {actor: animationsActor});
    this.manage(this);
  },

  destroy: function() {
    Front.prototype.destroy.call(this);
  }
});
