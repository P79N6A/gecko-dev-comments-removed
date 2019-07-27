






"use strict";














const {Cu} = require("chrome");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
const {
  createNode,
  drawGraphElementBackground,
  findOptimalTimeInterval
} = require("devtools/animationinspector/utils");

const STRINGS_URI = "chrome://browser/locale/devtools/animationinspector.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);
const MILLIS_TIME_FORMAT_MAX_DURATION = 4000;

const TIME_GRADUATION_MIN_SPACING = 40;







function PlayerMetaDataHeader() {
  
  
  this.state = {};
}

exports.PlayerMetaDataHeader = PlayerMetaDataHeader;

PlayerMetaDataHeader.prototype = {
  init: function(containerEl) {
    
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
      nodeType: "span",
      textContent: L10N.getStr("player.animationDurationLabel")
    });

    this.durationValue = createNode({
      parent: metaData,
      nodeType: "strong"
    });

    
    this.delayLabel = createNode({
      parent: metaData,
      nodeType: "span",
      attributes: {
        "style": "display:none;"
      },
      textContent: L10N.getStr("player.animationDelayLabel")
    });

    this.delayValue = createNode({
      parent: metaData,
      nodeType: "strong"
    });

    
    
    this.iterationLabel = createNode({
      parent: metaData,
      nodeType: "span",
      attributes: {
        "style": "display:none;"
      },
      textContent: L10N.getStr("player.animationIterationCountLabel")
    });

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








function PlaybackRateSelector() {
  this.currentRate = null;
  this.onSelectionChanged = this.onSelectionChanged.bind(this);
  EventEmitter.decorate(this);
}

exports.PlaybackRateSelector = PlaybackRateSelector;

PlaybackRateSelector.prototype = {
  PRESETS: [.1, .5, 1, 2, 5, 10],

  init: function(containerEl) {
    
    
    this.parentEl = containerEl;
  },

  destroy: function() {
    this.removeSelect();
    this.parentEl = this.el = null;
  },

  removeSelect: function() {
    if (this.el) {
      this.el.removeEventListener("change", this.onSelectionChanged);
      this.el.remove();
    }
  },

  



  getCurrentPresets: function({playbackRate}) {
    return [...new Set([...this.PRESETS, playbackRate])].sort((a, b) => a > b);
  },

  render: function(state) {
    if (state.playbackRate === this.currentRate) {
      return;
    }

    this.removeSelect();

    this.el = createNode({
      parent: this.parentEl,
      nodeType: "select",
      attributes: {
        "class": "rate devtools-button"
      }
    });

    for (let preset of this.getCurrentPresets(state)) {
      let option = createNode({
        parent: this.el,
        nodeType: "option",
        attributes: {
          value: preset,
        },
        textContent: L10N.getFormatStr("player.playbackRateLabel", preset)
      });
      if (preset === state.playbackRate) {
        option.setAttribute("selected", "");
      }
    }

    this.el.addEventListener("change", this.onSelectionChanged);

    this.currentRate = state.playbackRate;
  },

  onSelectionChanged: function() {
    this.emit("rate-changed", parseFloat(this.el.value));
  }
};











function AnimationTargetNode(inspector, options={}) {
  this.inspector = inspector;
  this.options = options;

  this.onPreviewMouseOver = this.onPreviewMouseOver.bind(this);
  this.onPreviewMouseOut = this.onPreviewMouseOut.bind(this);
  this.onSelectNodeClick = this.onSelectNodeClick.bind(this);
  this.onMarkupMutations = this.onMarkupMutations.bind(this);

  EventEmitter.decorate(this);
}

exports.AnimationTargetNode = AnimationTargetNode;

AnimationTargetNode.prototype = {
  init: function(containerEl) {
    let document = containerEl.ownerDocument;

    
    this.el = createNode({
      parent: containerEl,
      attributes: {
        "class": "animation-target"
      }
    });

    
    this.selectNodeEl = createNode({
      parent: this.el,
      nodeType: "span",
      attributes: {
        "class": "node-selector"
      }
    });

    
    this.previewEl = createNode({
      parent: this.el,
      nodeType: "span"
    });

    if (!this.options.compact) {
      this.previewEl.appendChild(document.createTextNode("<"));
    }

    
    this.tagNameEl = createNode({
      parent: this.previewEl,
      nodeType: "span",
      attributes: {
        "class": "tag-name theme-fg-color3"
      }
    });

    
    this.idEl = createNode({
      parent: this.previewEl,
      nodeType: "span"
    });

    if (!this.options.compact) {
      createNode({
        parent: this.idEl,
        nodeType: "span",
        attributes: {
          "class": "attribute-name theme-fg-color2"
        },
        textContent: "id"
      });
      this.idEl.appendChild(document.createTextNode("=\""));
    } else {
      createNode({
        parent: this.idEl,
        nodeType: "span",
        attributes: {
          "class": "theme-fg-color2"
        },
        textContent: "#"
      });
    }

    createNode({
      parent: this.idEl,
      nodeType: "span",
      attributes: {
        "class": "attribute-value theme-fg-color6"
      }
    });

    if (!this.options.compact) {
      this.idEl.appendChild(document.createTextNode("\""));
    }

    
    this.classEl = createNode({
      parent: this.previewEl,
      nodeType: "span"
    });

    if (!this.options.compact) {
      createNode({
        parent: this.classEl,
        nodeType: "span",
        attributes: {
          "class": "attribute-name theme-fg-color2"
        },
        textContent: "class"
      });
      this.classEl.appendChild(document.createTextNode("=\""));
    } else {
      createNode({
        parent: this.classEl,
        nodeType: "span",
        attributes: {
          "class": "theme-fg-color6"
        },
        textContent: "."
      });
    }

    createNode({
      parent: this.classEl,
      nodeType: "span",
      attributes: {
        "class": "attribute-value theme-fg-color6"
      }
    });

    if (!this.options.compact) {
      this.classEl.appendChild(document.createTextNode("\""));
      this.previewEl.appendChild(document.createTextNode(">"));
    }

    
    this.previewEl.addEventListener("mouseover", this.onPreviewMouseOver);
    this.previewEl.addEventListener("mouseout", this.onPreviewMouseOut);
    this.selectNodeEl.addEventListener("click", this.onSelectNodeClick);

    
    this.inspector.on("markupmutation", this.onMarkupMutations);
  },

  destroy: function() {
    this.inspector.off("markupmutation", this.onMarkupMutations);
    this.previewEl.removeEventListener("mouseover", this.onPreviewMouseOver);
    this.previewEl.removeEventListener("mouseout", this.onPreviewMouseOut);
    this.selectNodeEl.removeEventListener("click", this.onSelectNodeClick);
    this.el.remove();
    this.el = this.tagNameEl = this.idEl = this.classEl = null;
    this.selectNodeEl = this.previewEl = null;
    this.nodeFront = this.inspector = this.playerFront = null;
  },

  onPreviewMouseOver: function() {
    if (!this.nodeFront) {
      return;
    }
    this.inspector.toolbox.highlighterUtils.highlightNodeFront(this.nodeFront);
  },

  onPreviewMouseOut: function() {
    this.inspector.toolbox.highlighterUtils.unhighlight();
  },

  onSelectNodeClick: function() {
    if (!this.nodeFront) {
      return;
    }
    this.inspector.selection.setNodeFront(this.nodeFront, "animationinspector");
  },

  onMarkupMutations: function(e, mutations) {
    if (!this.nodeFront || !this.playerFront) {
      return;
    }

    for (let {target} of mutations) {
      if (target === this.nodeFront) {
        
        this.render(this.playerFront);
        break;
      }
    }
  },

  render: Task.async(function*(playerFront) {
    this.playerFront = playerFront;
    this.nodeFront = undefined;

    try {
      this.nodeFront = yield this.inspector.walker.getNodeFromActor(
                             playerFront.actorID, ["node"]);
    } catch (e) {
      
      
      if (!this.el) {
        console.warn("Cound't retrieve the animation target node, widget " +
                     "destroyed");
      }
      console.error(e);
      return;
    }

    if (!this.nodeFront || !this.el) {
      return;
    }

    let {tagName, attributes} = this.nodeFront;

    this.tagNameEl.textContent = tagName.toLowerCase();

    let idIndex = attributes.findIndex(({name}) => name === "id");
    if (idIndex > -1 && attributes[idIndex].value) {
      this.idEl.querySelector(".attribute-value").textContent =
        attributes[idIndex].value;
      this.idEl.style.display = "inline";
    } else {
      this.idEl.style.display = "none";
    }

    let classIndex = attributes.findIndex(({name}) => name === "class");
    if (classIndex > -1 && attributes[classIndex].value) {
      let value = attributes[classIndex].value;
      if (this.options.compact) {
        value = value.split(" ").join(".");
      }

      this.classEl.querySelector(".attribute-value").textContent = value;
      this.classEl.style.display = "inline";
    } else {
      this.classEl.style.display = "none";
    }

    this.emit("target-retrieved");
  })
};













let TimeScale = {
  minStartTime: Infinity,
  maxEndTime: 0,

  



  addAnimation: function({startTime, delay, duration, iterationCount}) {
    this.minStartTime = Math.min(this.minStartTime, startTime);
    let length = delay + (duration * (!iterationCount ? 1 : iterationCount));
    this.maxEndTime = Math.max(this.maxEndTime, startTime + length);
  },

  


  reset: function() {
    this.minStartTime = Infinity;
    this.maxEndTime = 0;
  },

  





  startTimeToDistance: function(time, containerWidth) {
    time -= this.minStartTime;
    return this.durationToDistance(time, containerWidth);
  },

  





  durationToDistance: function(duration, containerWidth) {
    return containerWidth * duration / (this.maxEndTime - this.minStartTime);
  },

  





  distanceToTime: function(distance, containerWidth) {
    return this.minStartTime +
      ((this.maxEndTime - this.minStartTime) * distance / containerWidth);
  },

  






  distanceToRelativeTime: function(distance, containerWidth) {
    let time = this.distanceToTime(distance, containerWidth);
    return time - this.minStartTime;
  },

  





  formatTime: function(time) {
    let duration = this.maxEndTime - this.minStartTime;

    
    if (duration <= MILLIS_TIME_FORMAT_MAX_DURATION) {
      return L10N.getFormatStr("timeline.timeGraduationLabel", time.toFixed(0));
    }

    
    return L10N.getFormatStr("player.timeLabel", (time / 1000).toFixed(1));
  }
};

exports.TimeScale = TimeScale;










function AnimationsTimeline(inspector) {
  this.animations = [];
  this.targetNodes = [];
  this.inspector = inspector;

  this.onAnimationStateChanged = this.onAnimationStateChanged.bind(this);
}

exports.AnimationsTimeline = AnimationsTimeline;

AnimationsTimeline.prototype = {
  init: function(containerEl) {
    this.win = containerEl.ownerDocument.defaultView;

    this.rootWrapperEl = createNode({
      parent: containerEl,
      attributes: {
        "class": "animation-timeline"
      }
    });

    this.timeHeaderEl = createNode({
      parent: this.rootWrapperEl,
      attributes: {
        "class": "time-header"
      }
    });

    this.animationsEl = createNode({
      parent: this.rootWrapperEl,
      nodeType: "ul",
      attributes: {
        "class": "animations"
      }
    });
  },

  destroy: function() {
    this.unrender();

    this.rootWrapperEl.remove();
    this.animations = [];

    this.rootWrapperEl = null;
    this.timeHeaderEl = null;
    this.animationsEl = null;
    this.win = null;
    this.inspector = null;
  },

  destroyTargetNodes: function() {
    for (let targetNode of this.targetNodes) {
      targetNode.destroy();
    }
    this.targetNodes = [];
  },

  unrender: function() {
    for (let animation of this.animations) {
      animation.off("changed", this.onAnimationStateChanged);
    }

    TimeScale.reset();
    this.destroyTargetNodes();
    this.animationsEl.innerHTML = "";
  },

  render: function(animations) {
    this.unrender();

    this.animations = animations;
    if (!this.animations.length) {
      return;
    }

    
    for (let {state} of animations) {
      TimeScale.addAnimation(state);
    }

    this.drawHeaderAndBackground();

    for (let animation of this.animations) {
      animation.on("changed", this.onAnimationStateChanged);

      
      
      let animationEl = createNode({
        parent: this.animationsEl,
        nodeType: "li",
        attributes: {
          "class": "animation"
        }
      });

      
      let animatedNodeEl = createNode({
        parent: animationEl,
        attributes: {
          "class": "target"
        }
      });

      let timeBlockEl = createNode({
        parent: animationEl,
        attributes: {
          "class": "time-block"
        }
      });

      this.drawTimeBlock(animation, timeBlockEl);

      
      let targetNode = new AnimationTargetNode(this.inspector, {compact: true});
      targetNode.init(animatedNodeEl);
      targetNode.render(animation);

      
      this.targetNodes.push(targetNode);
    }
  },

  onAnimationStateChanged: function() {
    
    
    this.render(this.animations);
  },

  drawHeaderAndBackground: function() {
    let width = this.timeHeaderEl.offsetWidth;
    let scale = width / (TimeScale.maxEndTime - TimeScale.minStartTime);
    drawGraphElementBackground(this.win.document, "time-graduations", width, scale);

    
    this.timeHeaderEl.innerHTML = "";
    let interval = findOptimalTimeInterval(scale, TIME_GRADUATION_MIN_SPACING);
    for (let i = 0; i < width; i += interval) {
      createNode({
        parent: this.timeHeaderEl,
        nodeType: "span",
        attributes: {
          "class": "time-tick",
          "style": `left:${i}px`
        },
        textContent: TimeScale.formatTime(
          TimeScale.distanceToRelativeTime(i, width))
      });
    }
  },

  drawTimeBlock: function({state}, el) {
    let width = el.offsetWidth;

    
    
    let x = TimeScale.startTimeToDistance(state.startTime + (state.delay || 0),
                                          width);
    
    let count = state.iterationCount || 1;
    let w = TimeScale.durationToDistance(state.duration, width);

    let iterations = createNode({
      parent: el,
      attributes: {
        "class": "iterations" + (state.iterationCount ? "" : " infinite"),
        
        
        "style": `left:${x}px;
                  width:${w * count}px;
                  background-size:${Math.max(w, 2)}px 100%;`
      }
    });

    
    createNode({
      parent: iterations,
      attributes: {
        "class": "name"
      },
      textContent: state.name
    });

    
    if (state.delay) {
      let delay = TimeScale.durationToDistance(state.delay, width);
      createNode({
        parent: iterations,
        attributes: {
          "class": "delay",
          "style": `left:-${delay}px;
                    width:${delay}px;`
        }
      });
    }
  }
};
