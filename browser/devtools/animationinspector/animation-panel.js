





"use strict";




let AnimationsPanel = {
  UI_UPDATED_EVENT: "ui-updated",
  PANEL_INITIALIZED: "panel-initialized",

  initialize: Task.async(function*() {
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
    this.createPlayerWidgets = this.createPlayerWidgets.bind(this);
    this.toggleAll = this.toggleAll.bind(this);
    this.onTabNavigated = this.onTabNavigated.bind(this);

    this.startListeners();

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
    yield this.destroyPlayerWidgets();

    this.playersEl = this.errorMessageEl = null;
    this.toggleAllButtonEl = this.pickerButtonEl = null;

    this.destroyed.resolve();
  }),

  startListeners: function() {
    AnimationsController.on(AnimationsController.PLAYERS_UPDATED_EVENT,
      this.createPlayerWidgets);

    this.pickerButtonEl.addEventListener("click", this.togglePicker, false);
    gToolbox.on("picker-started", this.onPickerStarted);
    gToolbox.on("picker-stopped", this.onPickerStopped);

    this.toggleAllButtonEl.addEventListener("click", this.toggleAll, false);
    gToolbox.target.on("navigate", this.onTabNavigated);
  },

  stopListeners: function() {
    AnimationsController.off(AnimationsController.PLAYERS_UPDATED_EVENT,
      this.createPlayerWidgets);

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

    
    
    
    if (this.playerWidgets) {
      let currentWidgetStateChange = [];
      for (let widget of this.playerWidgets) {
        currentWidgetStateChange.push(btnClass.contains("paused")
          ? widget.play() : widget.pause());
      }
      yield promise.all(currentWidgetStateChange).catch(Cu.reportError);
    }

    btnClass.toggle("paused");
    yield AnimationsController.toggleAll();
  }),

  onTabNavigated: function() {
    this.toggleAllButtonEl.classList.remove("paused");
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
  this.onRewindBtnClick = this.onRewindBtnClick.bind(this);
  this.onFastForwardBtnClick = this.onFastForwardBtnClick.bind(this);
  this.onCurrentTimeChanged = this.onCurrentTimeChanged.bind(this);

  this.metaDataComponent = new PlayerMetaDataHeader();
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
  },

  stopListeners: function() {
    this.player.off(this.player.AUTO_REFRESH_EVENT, this.onStateChanged);
    this.playPauseBtnEl.removeEventListener("click", this.onPlayPauseBtnClick);
    if (AnimationsController.hasSetCurrentTime) {
      this.rewindBtnEl.removeEventListener("click", this.onRewindBtnClick);
      this.fastForwardBtnEl.removeEventListener("click", this.onFastForwardBtnClick);
      this.currentTimeEl.removeEventListener("input", this.onCurrentTimeChanged);
    }
  },

  createMarkup: function() {
    let state = this.player.state;

    this.el = createNode({
      attributes: {
        "class": "player-widget " + state.playState
      }
    });

    this.metaDataComponent.createMarkup(this.el);
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

    this.containerEl.appendChild(this.el);

    
    this.displayTime(state.currentTime);
  },

  






  onPlayPauseBtnClick: function() {
    if (this.player.state.playState === "running") {
      return this.pause();
    } else {
      return this.play();
    }
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

  


  onStateChanged: function() {
    let state = this.player.state;
    this.updateWidgetState(state);
    this.metaDataComponent.render(state);

    switch (state.playState) {
      case "finished":
        this.stopTimelineAnimation();
        this.displayTime(this.player.state.duration);
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

  





  setCurrentTime: Task.async(function*(time, shouldPause) {
    if (!AnimationsController.hasSetCurrentTime) {
      throw new Error("This server version doesn't support setting animations' currentTime");
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

  




  pause: function() {
    if (this.player.state.playState === "finished") {
      return;
    }

    
    
    this.updateWidgetState({playState: "paused"});
    this.stopTimelineAnimation();
    return this.player.pause();
  },

  




  play: function() {
    if (this.player.state.playState === "finished") {
      return;
    }

    
    
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
      let now = state.currentTime + performance.now() - start;
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







function PlayerMetaDataHeader() {
  
  
  this.state = {};
}

PlayerMetaDataHeader.prototype = {
  createMarkup: function(containerEl) {
    
    this.el = createNode({
      parent: containerEl,
      attributes: {
        "class": "animation-title"
      }
    });

    
    this.nameLabel = createNode({
      parent: this.el,
      nodeType: "span"
    });

    this.nameValue = createNode({
      parent: this.el,
      nodeType: "strong",
      attributes: {
        "style": "display:none;"
      }
    });

    
    let metaData = createNode({
      parent: this.el,
      nodeType: "span",
      attributes: {
        "class": "meta-data"
      }
    });

    
    this.durationLabel = createNode({
      parent: metaData,
      nodeType: "span"
    });
    this.durationLabel.textContent = L10N.getStr("player.animationDurationLabel");

    this.durationValue = createNode({
      parent: metaData,
      nodeType: "strong"
    });

    
    this.delayLabel = createNode({
      parent: metaData,
      nodeType: "span",
      attributes: {
        "style": "display:none;"
      }
    });
    this.delayLabel.textContent = L10N.getStr("player.animationDelayLabel");

    this.delayValue = createNode({
      parent: metaData,
      nodeType: "strong"
    });

    
    
    this.iterationLabel = createNode({
      parent: metaData,
      nodeType: "span",
      attributes: {
        "style": "display:none;"
      }
    });
    this.iterationLabel.textContent = L10N.getStr("player.animationIterationCountLabel");

    this.iterationValue = createNode({
      parent: metaData,
      nodeType: "strong",
      attributes: {
        "style": "display:none;"
      }
    });
  },

  destroy: function() {
    this.state = null;
    this.el.remove();
    this.el = null;
    this.nameLabel = this.nameValue = null;
    this.durationLabel = this.durationValue = null;
    this.delayLabel = this.delayValue = null;
    this.iterationLabel = this.iterationValue = null;
  },

  render: function(state) {
    
    if (state.name !== this.state.name) {
      if (state.name) {
        
        this.nameLabel.textContent = L10N.getStr("player.animationNameLabel");
        this.nameValue.style.display = "inline";
        this.nameValue.textContent = state.name;
      } else {
        
        this.nameLabel.textContent = L10N.getStr("player.transitionNameLabel");
        this.nameValue.style.display = "none";
      }
    }

    
    if (state.duration !== this.state.duration) {
      this.durationValue.textContent = L10N.getFormatStr("player.timeLabel",
        L10N.numberWithDecimals(state.duration / 1000, 2));
    }

    
    if (state.delay !== this.state.delay) {
      if (state.delay) {
        this.delayLabel.style.display = "inline";
        this.delayValue.style.display = "inline";
        this.delayValue.textContent = L10N.getFormatStr("player.timeLabel",
          L10N.numberWithDecimals(state.delay / 1000, 2));
      } else {
        
        this.delayLabel.style.display = "none";
        this.delayValue.style.display = "none";
      }
    }

    
    if (state.iterationCount !== this.state.iterationCount) {
      if (state.iterationCount !== 1) {
        this.iterationLabel.style.display = "inline";
        this.iterationValue.style.display = "inline";
        let count = state.iterationCount ||
                    L10N.getStr("player.infiniteIterationCount");
        this.iterationValue.innerHTML = count;
      } else {
        
        this.iterationLabel.style.display = "none";
        this.iterationValue.style.display = "none";
      }
    }

    this.state = state;
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
