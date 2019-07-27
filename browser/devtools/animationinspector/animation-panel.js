







"use strict";

const {createNode} = require("devtools/animationinspector/utils");
const {
  PlayerMetaDataHeader,
  PlaybackRateSelector,
  AnimationTargetNode,
  AnimationsTimeline
} = require("devtools/animationinspector/components");




let AnimationsPanel = {
  UI_UPDATED_EVENT: "ui-updated",
  PANEL_INITIALIZED: "panel-initialized",

  initialize: Task.async(function*() {
    if (AnimationsController.destroyed) {
      console.warn("Could not initialize the animation-panel, controller " +
                   "was destroyed");
      return;
    }
    if (this.initialized) {
      return this.initialized.promise;
    }
    this.initialized = promise.defer();

    this.playersEl = document.querySelector("#players");
    this.errorMessageEl = document.querySelector("#error-message");
    this.pickerButtonEl = document.querySelector("#element-picker");
    this.toggleAllButtonEl = document.querySelector("#toggle-all");

    
    
    if (!AnimationsController.hasToggleAll) {
      document.querySelector("#toolbar").style.display = "none";
    }

    let hUtils = gToolbox.highlighterUtils;
    this.togglePicker = hUtils.togglePicker.bind(hUtils);
    this.onPickerStarted = this.onPickerStarted.bind(this);
    this.onPickerStopped = this.onPickerStopped.bind(this);
    this.refreshAnimations = this.refreshAnimations.bind(this);
    this.toggleAll = this.toggleAll.bind(this);
    this.onTabNavigated = this.onTabNavigated.bind(this);

    this.startListeners();

    if (AnimationsController.isNewUI) {
      this.animationsTimelineComponent = new AnimationsTimeline(gInspector);
      this.animationsTimelineComponent.init(this.playersEl);
    }

    yield this.refreshAnimations();

    this.initialized.resolve();

    this.emit(this.PANEL_INITIALIZED);
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

    if (this.animationsTimelineComponent) {
      this.animationsTimelineComponent.destroy();
      this.animationsTimelineComponent = null;
    }
    yield this.destroyPlayerWidgets();

    this.playersEl = this.errorMessageEl = null;
    this.toggleAllButtonEl = this.pickerButtonEl = null;

    this.destroyed.resolve();
  }),

  startListeners: function() {
    AnimationsController.on(AnimationsController.PLAYERS_UPDATED_EVENT,
      this.refreshAnimations);

    this.pickerButtonEl.addEventListener("click", this.togglePicker, false);
    gToolbox.on("picker-started", this.onPickerStarted);
    gToolbox.on("picker-stopped", this.onPickerStopped);

    this.toggleAllButtonEl.addEventListener("click", this.toggleAll, false);
    gToolbox.target.on("navigate", this.onTabNavigated);
  },

  stopListeners: function() {
    AnimationsController.off(AnimationsController.PLAYERS_UPDATED_EVENT,
      this.refreshAnimations);

    this.pickerButtonEl.removeEventListener("click", this.togglePicker, false);
    gToolbox.off("picker-started", this.onPickerStarted);
    gToolbox.off("picker-stopped", this.onPickerStopped);

    this.toggleAllButtonEl.removeEventListener("click", this.toggleAll, false);
    gToolbox.target.off("navigate", this.onTabNavigated);
  },

  displayErrorMessage: function() {
    this.errorMessageEl.style.display = "block";
    this.playersEl.style.display = "none";
  },

  hideErrorMessage: function() {
    this.errorMessageEl.style.display = "none";
    this.playersEl.style.display = "block";
  },

  onPickerStarted: function() {
    this.pickerButtonEl.setAttribute("checked", "true");
  },

  onPickerStopped: function() {
    this.pickerButtonEl.removeAttribute("checked");
  },

  toggleAll: Task.async(function*() {
    let btnClass = this.toggleAllButtonEl.classList;

    if (!AnimationsController.isNewUI) {
      
      
      
      if (this.playerWidgets) {
        let currentWidgetStateChange = [];
        for (let widget of this.playerWidgets) {
          currentWidgetStateChange.push(btnClass.contains("paused")
            ? widget.play() : widget.pause());
        }
        yield promise.all(currentWidgetStateChange).catch(Cu.reportError);
      }
    }

    btnClass.toggle("paused");
    yield AnimationsController.toggleAll();
  }),

  onTabNavigated: function() {
    this.toggleAllButtonEl.classList.remove("paused");
  },

  refreshAnimations: Task.async(function*() {
    let done = gInspector.updating("animationspanel");

    
    this.hideErrorMessage();
    yield this.destroyPlayerWidgets();

    
    if (this.animationsTimelineComponent) {
      this.animationsTimelineComponent.render(
        AnimationsController.animationPlayers);
    }

    
    
    if (!AnimationsController.animationPlayers.length) {
      this.displayErrorMessage();
      this.emit(this.UI_UPDATED_EVENT);
      done();
      return;
    }

    
    
    if (!AnimationsController.isNewUI) {
      this.playerWidgets = [];
      let initPromises = [];

      for (let player of AnimationsController.animationPlayers) {
        let widget = new PlayerWidget(player, this.playersEl);
        initPromises.push(widget.initialize());
        this.playerWidgets.push(widget);
      }

      yield initPromises;
    }

    this.emit(this.UI_UPDATED_EVENT);
    done();
  }),

  destroyPlayerWidgets: Task.async(function*() {
    if (!this.playerWidgets) {
      return;
    }

    let destroyers = this.playerWidgets.map(widget => widget.destroy());
    yield promise.all(destroyers);
    this.playerWidgets = null;
    this.playersEl.innerHTML = "";
  })
};

EventEmitter.decorate(AnimationsPanel);




function PlayerWidget(player, containerEl) {
  EventEmitter.decorate(this);

  this.player = player;
  this.containerEl = containerEl;

  this.onStateChanged = this.onStateChanged.bind(this);
  this.onPlayPauseBtnClick = this.onPlayPauseBtnClick.bind(this);
  this.onRewindBtnClick = this.onRewindBtnClick.bind(this);
  this.onFastForwardBtnClick = this.onFastForwardBtnClick.bind(this);
  this.onCurrentTimeChanged = this.onCurrentTimeChanged.bind(this);
  this.onPlaybackRateChanged = this.onPlaybackRateChanged.bind(this);

  this.metaDataComponent = new PlayerMetaDataHeader();
  if (AnimationsController.hasSetPlaybackRate) {
    this.rateComponent = new PlaybackRateSelector();
  }
  if (AnimationsController.hasTargetNode) {
    this.targetNodeComponent = new AnimationTargetNode(gInspector);
  }
}

PlayerWidget.prototype = {
  initialize: Task.async(function*() {
    if (this.initialized) {
      return;
    }
    this.initialized = true;

    this.createMarkup();
    this.startListeners();
  }),

  destroy: Task.async(function*() {
    if (this.destroyed) {
      return;
    }
    this.destroyed = true;

    this.stopTimelineAnimation();
    this.stopListeners();
    this.metaDataComponent.destroy();
    if (this.rateComponent) {
      this.rateComponent.destroy();
    }
    if (this.targetNodeComponent) {
      this.targetNodeComponent.destroy();
    }

    this.el.remove();
    this.playPauseBtnEl = this.rewindBtnEl = this.fastForwardBtnEl = null;
    this.currentTimeEl = this.timeDisplayEl = null;
    this.containerEl = this.el = this.player = null;
  }),

  startListeners: function() {
    this.player.on(this.player.AUTO_REFRESH_EVENT, this.onStateChanged);
    this.playPauseBtnEl.addEventListener("click", this.onPlayPauseBtnClick);
    if (AnimationsController.hasSetCurrentTime) {
      this.rewindBtnEl.addEventListener("click", this.onRewindBtnClick);
      this.fastForwardBtnEl.addEventListener("click", this.onFastForwardBtnClick);
      this.currentTimeEl.addEventListener("input", this.onCurrentTimeChanged);
    }
    if (this.rateComponent) {
      this.rateComponent.on("rate-changed", this.onPlaybackRateChanged);
    }
  },

  stopListeners: function() {
    this.player.off(this.player.AUTO_REFRESH_EVENT, this.onStateChanged);
    this.playPauseBtnEl.removeEventListener("click", this.onPlayPauseBtnClick);
    if (AnimationsController.hasSetCurrentTime) {
      this.rewindBtnEl.removeEventListener("click", this.onRewindBtnClick);
      this.fastForwardBtnEl.removeEventListener("click", this.onFastForwardBtnClick);
      this.currentTimeEl.removeEventListener("input", this.onCurrentTimeChanged);
    }
    if (this.rateComponent) {
      this.rateComponent.off("rate-changed", this.onPlaybackRateChanged);
    }
  },

  createMarkup: function() {
    let state = this.player.state;

    this.el = createNode({
      parent: this.containerEl,
      attributes: {
        "class": "player-widget " + state.playState
      }
    });

    if (this.targetNodeComponent) {
      this.targetNodeComponent.init(this.el);
      this.targetNodeComponent.render(this.player);
    }

    this.metaDataComponent.init(this.el);
    this.metaDataComponent.render(state);

    
    let timelineEl = createNode({
      parent: this.el,
      attributes: {
        "class": "timeline"
      }
    });

    
    let playbackControlsEl = createNode({
      parent: timelineEl,
      attributes: {
        "class": "playback-controls"
      }
    });

    
    this.playPauseBtnEl = createNode({
      parent: playbackControlsEl,
      nodeType: "button",
      attributes: {
        "class": "toggle devtools-button"
      }
    });

    if (AnimationsController.hasSetCurrentTime) {
      this.rewindBtnEl = createNode({
        parent: playbackControlsEl,
        nodeType: "button",
        attributes: {
          "class": "rw devtools-button"
        }
      });

      this.fastForwardBtnEl = createNode({
        parent: playbackControlsEl,
        nodeType: "button",
        attributes: {
          "class": "ff devtools-button"
        }
      });
    }

    if (this.rateComponent) {
      this.rateComponent.init(playbackControlsEl);
      this.rateComponent.render(state);
    }

    
    let slidersContainerEl = createNode({
      parent: timelineEl,
      attributes: {
        "class": "sliders-container",
      }
    });

    let max = state.duration;
    if (state.iterationCount) {
      
      max = state.iterationCount * state.duration;
    }

    
    
    
    this.currentTimeEl = createNode({
      nodeType: "input",
      parent: slidersContainerEl,
      attributes: {
        "type": "range",
        "class": "current-time",
        "min": "0",
        "max": max,
        "step": "10",
        "value": "0"
      }
    });

    if (!AnimationsController.hasSetCurrentTime) {
      this.currentTimeEl.setAttribute("disabled", "true");
    }

    
    this.timeDisplayEl = createNode({
      parent: timelineEl,
      attributes: {
        "class": "time-display"
      }
    });

    
    this.displayTime(state.currentTime);
  },

  






  onPlayPauseBtnClick: function() {
    if (this.player.state.playState === "running") {
      return this.pause();
    }
    return this.play();
  },

  onRewindBtnClick: function() {
    this.setCurrentTime(0, true);
  },

  onFastForwardBtnClick: function() {
    let state = this.player.state;

    let time = state.duration;
    if (state.iterationCount) {
      time = state.iterationCount * state.duration;
    }
    this.setCurrentTime(time, true);
  },

  


  onCurrentTimeChanged: function(e) {
    let time = e.target.value;
    this.setCurrentTime(parseFloat(time), true);
  },

  



  onPlaybackRateChanged: function(e, rate) {
    this.setPlaybackRate(rate);
  },

  


  onStateChanged: function() {
    let state = this.player.state;

    this.updateWidgetState(state);
    this.metaDataComponent.render(state);
    if (this.rateComponent) {
      this.rateComponent.render(state);
    }

    switch (state.playState) {
      case "finished":
        this.stopTimelineAnimation();
        this.displayTime(this.player.state.currentTime);
        break;
      case "running":
        this.startTimelineAnimation();
        break;
      case "paused":
        this.stopTimelineAnimation();
        this.displayTime(this.player.state.currentTime);
        break;
      case "idle":
        this.stopTimelineAnimation();
        this.displayTime(0);
        break;
    }
  },

  





  setCurrentTime: Task.async(function*(time, shouldPause) {
    if (!AnimationsController.hasSetCurrentTime) {
      throw new Error("This server version doesn't support setting " +
                      "animations' currentTime");
    }

    if (shouldPause) {
      this.stopTimelineAnimation();
      yield this.pause();
    }

    if (this.player.state.delay) {
      time += this.player.state.delay;
    }

    
    
    this.displayTime(time);

    yield this.player.setCurrentTime(time);
  }),

  




  setPlaybackRate: function(rate) {
    if (!AnimationsController.hasSetPlaybackRate) {
      throw new Error("This server version doesn't support setting " +
                      "animations' playbackRate");
    }

    return this.player.setPlaybackRate(rate);
  },

  




  pause: function() {
    
    
    this.updateWidgetState({playState: "paused"});
    this.stopTimelineAnimation();
    return this.player.pause();
  },

  




  play: function() {
    
    
    this.updateWidgetState({playState: "running"});
    this.startTimelineAnimation();
    return this.player.play();
  },

  updateWidgetState: function({playState}) {
    this.el.className = "player-widget " + playState;
  },

  



  startTimelineAnimation: function() {
    this.stopTimelineAnimation();

    let state = this.player.state;

    let start = performance.now();
    let loop = () => {
      this.rafID = requestAnimationFrame(loop);
      let delta = (performance.now() - start) * state.playbackRate;
      let now = state.currentTime + delta;
      this.displayTime(now);
    };

    loop();
  },

  


  displayTime: function(time) {
    let state = this.player.state;

    
    
    if (state.delay) {
      time = Math.max(0, time - state.delay);
    }

    
    
    
    if (state.iterationCount) {
      time = Math.min(time, state.iterationCount * state.duration);
    }

    
    this.timeDisplayEl.textContent = L10N.getFormatStr("player.timeLabel",
      L10N.numberWithDecimals(time / 1000, 2));

    
    if (!state.iterationCount && time !== state.duration) {
      time = time % state.duration;
    }
    this.currentTimeEl.value = time;
  },

  


  stopTimelineAnimation: function() {
    if (this.rafID) {
      cancelAnimationFrame(this.rafID);
      this.rafID = null;
    }
  }
};
