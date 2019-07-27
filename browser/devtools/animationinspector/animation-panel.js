





"use strict";




let AnimationsPanel = {
  UI_UPDATED_EVENT: "ui-updated",

  initialize: Task.async(function*() {
    if (this.initialized) {
      return this.initialized.promise;
    }
    this.initialized = promise.defer();

    this.playersEl = document.querySelector("#players");
    this.errorMessageEl = document.querySelector("#error-message");
    this.pickerButtonEl = document.querySelector("#element-picker");

    let hUtils = gToolbox.highlighterUtils;
    this.togglePicker = hUtils.togglePicker.bind(hUtils);
    this.onPickerStarted = this.onPickerStarted.bind(this);
    this.onPickerStopped = this.onPickerStopped.bind(this);
    this.createPlayerWidgets = this.createPlayerWidgets.bind(this);

    this.startListeners();

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
    yield this.destroyPlayerWidgets();

    this.playersEl = this.errorMessageEl = null;

    this.destroyed.resolve();
  }),

  startListeners: function() {
    AnimationsController.on(AnimationsController.PLAYERS_UPDATED_EVENT,
      this.createPlayerWidgets);
    this.pickerButtonEl.addEventListener("click", this.togglePicker, false);
    gToolbox.on("picker-started", this.onPickerStarted);
    gToolbox.on("picker-stopped", this.onPickerStopped);
  },

  stopListeners: function() {
    AnimationsController.off(AnimationsController.PLAYERS_UPDATED_EVENT,
      this.createPlayerWidgets);
    this.pickerButtonEl.removeEventListener("click", this.togglePicker, false);
    gToolbox.off("picker-started", this.onPickerStarted);
    gToolbox.off("picker-stopped", this.onPickerStopped);
  },

  displayErrorMessage: function() {
    this.errorMessageEl.style.display = "block";
  },

  hideErrorMessage: function() {
    this.errorMessageEl.style.display = "none";
  },

  onPickerStarted: function() {
    this.pickerButtonEl.setAttribute("checked", "true");
  },

  onPickerStopped: function() {
    this.pickerButtonEl.removeAttribute("checked");
  },

  createPlayerWidgets: Task.async(function*() {
    let done = gInspector.updating("animationspanel");

    
    this.hideErrorMessage();
    yield this.destroyPlayerWidgets();

    
    if (!AnimationsController.animationPlayers.length) {
      this.displayErrorMessage();
      this.emit(this.UI_UPDATED_EVENT);
      done();
      return;
    }

    
    this.playerWidgets = [];
    let initPromises = [];

    for (let player of AnimationsController.animationPlayers) {
      let widget = new PlayerWidget(player, this.playersEl);
      initPromises.push(widget.initialize());
      this.playerWidgets.push(widget);
    }

    yield initPromises;
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

    this.el.remove();
    this.playPauseBtnEl = this.currentTimeEl = this.timeDisplayEl = null;
    this.containerEl = this.el = this.player = null;
  }),

  startListeners: function() {
    this.player.on(this.player.AUTO_REFRESH_EVENT, this.onStateChanged);
    this.playPauseBtnEl.addEventListener("click", this.onPlayPauseBtnClick);
  },

  stopListeners: function() {
    this.player.off(this.player.AUTO_REFRESH_EVENT, this.onStateChanged);
    this.playPauseBtnEl.removeEventListener("click", this.onPlayPauseBtnClick);
  },

  createMarkup: function() {
    let state = this.player.state;

    this.el = createNode({
      attributes: {
        "class": "player-widget " + state.playState
      }
    });

    
    let titleEl = createNode({
      parent: this.el,
      attributes: {
        "class": "animation-title"
      }
    });
    let titleHTML = "";

    
    if (state.name) {
      
      titleHTML += L10N.getStr("player.animationNameLabel");
      titleHTML += "<strong>" + state.name + "</strong>";
    } else {
      
      titleHTML += L10N.getStr("player.transitionNameLabel");
    }

    
    titleHTML += "<span class='meta-data'>";
    titleHTML += L10N.getStr("player.animationDurationLabel");
    titleHTML += "<strong>" + L10N.getFormatStr("player.timeLabel",
      this.getFormattedTime(state.duration)) + "</strong>";
    if (state.delay) {
      titleHTML += L10N.getStr("player.animationDelayLabel");
      titleHTML += "<strong>" + L10N.getFormatStr("player.timeLabel",
        this.getFormattedTime(state.delay)) + "</strong>";
    }
    titleHTML += L10N.getStr("player.animationIterationCountLabel");
    let count = state.iterationCount || L10N.getStr("player.infiniteIterationCount");
    titleHTML += "<strong>" + count + "</strong>";
    titleHTML += "</span>"

    titleEl.innerHTML = titleHTML;

    
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
        "value": "0",
        
        "disabled": "true"
      }
    });

    
    this.timeDisplayEl = createNode({
      parent: timelineEl,
      attributes: {
        "class": "time-display"
      }
    });

    this.containerEl.appendChild(this.el);

    
    this.displayTime(state.currentTime);
  },

  




  getFormattedTime: function(time) {
    return (time/1000).toLocaleString(undefined, {
      minimumFractionDigits: 2,
      maximumFractionDigits: 2
    });
  },

  






  onPlayPauseBtnClick: function() {
    if (this.player.state.playState === "running") {
      return this.pause();
    } else {
      return this.play();
    }
  },

  


  onStateChanged: function() {
    let state = this.player.state;
    this.updateWidgetState(state.playState);

    switch (state.playState) {
      case "finished":
        this.stopTimelineAnimation();
        this.stopListeners();
        break;
      case "running":
        this.startTimelineAnimation();
        break;
      case "paused":
        this.stopTimelineAnimation();
        this.displayTime(this.player.state.currentTime);
        break;
    }
  },

  




  pause: function() {
    
    
    this.updateWidgetState("paused");
    return this.player.pause().then(() => {
      this.stopTimelineAnimation();
    });
  },

  




  play: function() {
    
    
    this.updateWidgetState("running");
    this.startTimelineAnimation();
    return this.player.play();
  },

  updateWidgetState: function(playState) {
    this.el.className = "player-widget " + playState;
  },

  



  startTimelineAnimation: function() {
    this.stopTimelineAnimation();

    let start = performance.now();
    let loop = () => {
      this.rafID = requestAnimationFrame(loop);
      let now = this.player.state.currentTime + performance.now() - start;
      this.displayTime(now);
    };

    loop();
  },

  


  displayTime: function(time) {
    let state = this.player.state;

    
    
    if (state.delay) {
      time = Math.max(0, time - state.delay);
    }

    this.timeDisplayEl.textContent = L10N.getFormatStr("player.timeLabel",
      this.getFormattedTime(time));
    if (!state.iterationCount && time !== state.duration) {
      this.currentTimeEl.value = time % state.duration;
    } else {
      this.currentTimeEl.value = time;
    }
  },

  


  stopTimelineAnimation: function() {
    if (this.rafID) {
      cancelAnimationFrame(this.rafID);
      this.rafID = null;
    }
  }
};






function createNode(options) {
  let type = options.nodeType || "div";
  let node = document.createElement(type);

  for (let name in options.attributes || {}) {
    let value = options.attributes[name];
    node.setAttribute(name, value);
  }

  if (options.parent) {
    options.parent.appendChild(node);
  }

  return node;
}
