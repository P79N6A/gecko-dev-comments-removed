



"use strict";





















const {ActorClass, Actor,
       FrontClass, Front,
       Arg, method, RetVal} = require("devtools/server/protocol");
const {NodeActor} = require("devtools/server/actors/inspector");










let AnimationPlayerActor = ActorClass({
  typeName: "animationplayer",

  








  initialize: function(animationsActor, player, node, playerIndex) {
    this.player = player;
    this.node = node;
    this.playerIndex = playerIndex;
    this.styles = node.ownerDocument.defaultView.getComputedStyle(node);
    Actor.prototype.initialize.call(this, animationsActor.conn);
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
      durationText = durationText.split(",")[this.playerIndex];
    }

    return parseFloat(durationText) * 1000;
  },

  








  getIterationCount: function() {
    let iterationText = this.styles.animationIterationCount;
    if (iterationText.indexOf(",") !== -1) {
      iterationText = iterationText.split(",")[this.playerIndex];
    }

    return parseInt(iterationText, 10);
  },

  





  getCurrentState: method(function() {
    return {
      



      startTime: this.player.startTime,
      currentTime: this.player.currentTime,
      playState: this.player.playState,
      name: this.player.source.effect.name,
      duration: this.getDuration(),
      iterationCount: this.getIterationCount(),
      







      isRunningOnCompositor: this.player.isRunningOnCompositor
    };
  }, {
    request: {},
    response: {
      data: RetVal("json")
    }
  }),

  


  pause: method(function() {
    this.player.pause();
  }, {
    request: {},
    response: {}
  }),

  


  play: method(function() {
    this.player.play();
  }, {
    request: {},
    response: {}
  })
});

let AnimationPlayerFront = FrontClass(AnimationPlayerActor, {
  initialize: function(conn, form, detail, ctx) {
    Front.prototype.initialize.call(this, conn, form, detail, ctx);
  },

  form: function(form, detail) {
    if (detail === "actorid") {
      this.actorID = form;
      return;
    }
    this._form = form;
  },

  destroy: function() {
    Front.prototype.destroy.call(this);
  },

  



  get initialState() {
    return {
      startTime: this._form.startTime,
      currentTime: this._form.currentTime,
      playState: this._form.playState,
      name: this._form.name,
      duration: this._form.duration,
      iterationCount: this._form.iterationCount,
      isRunningOnCompositor: this._form.isRunningOnCompositor
    }
  }
});




let AnimationsActor = exports.AnimationsActor = ActorClass({
  typeName: "animations",

  initialize: function(conn, tabActor) {
    Actor.prototype.initialize.call(this, conn);
  },

  destroy: function() {
    Actor.prototype.destroy.call(this);
  },

  



  disconnect: function() {
    this.destroy();
  },

  





  getAnimationPlayersForNode: method(function(nodeActor) {
    let players = nodeActor.rawNode.getAnimationPlayers();

    let actors = [];
    for (let i = 0; i < players.length; i ++) {
      
      
      actors.push(AnimationPlayerActor(this, players[i], nodeActor.rawNode, i));
    }

    return actors;
  }, {
    request: {
      actorID: Arg(0, "domnode")
    },
    response: {
      players: RetVal("array:animationplayer")
    }
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
