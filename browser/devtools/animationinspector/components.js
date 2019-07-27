





"use strict";














const {Cu} = require('chrome');
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

const STRINGS_URI = "chrome://browser/locale/devtools/animationinspector.properties";
const L10N = new ViewHelpers.L10N(STRINGS_URI);







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
    return [...new Set([...this.PRESETS, playbackRate])].sort((a,b) => a > b);
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
        }
      });
      option.textContent = L10N.getFormatStr("player.playbackRateLabel", preset);
      if (preset === state.playbackRate) {
        option.setAttribute("selected", "");
      }
    }

    this.el.addEventListener("change", this.onSelectionChanged);

    this.currentRate = state.playbackRate;
  },

  onSelectionChanged: function(e) {
    this.emit("rate-changed", parseFloat(this.el.value));
  }
};








function AnimationTargetNode(inspector) {
  this.inspector = inspector;

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

    this.previewEl.appendChild(document.createTextNode("<"));

    
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

    createNode({
      parent: this.idEl,
      nodeType: "span",
      attributes: {
        "class": "attribute-name theme-fg-color2"
      }
    }).textContent = "id";

    this.idEl.appendChild(document.createTextNode("=\""));

    createNode({
      parent: this.idEl,
      nodeType: "span",
      attributes: {
        "class": "attribute-value theme-fg-color6"
      }
    });

    this.idEl.appendChild(document.createTextNode("\""));

    
    this.classEl = createNode({
      parent: this.previewEl,
      nodeType: "span"
    });

    createNode({
      parent: this.classEl,
      nodeType: "span",
      attributes: {
        "class": "attribute-name theme-fg-color2"
      }
    }).textContent = "class";

    this.classEl.appendChild(document.createTextNode("=\""));

    createNode({
      parent: this.classEl,
      nodeType: "span",
      attributes: {
        "class": "attribute-value theme-fg-color6"
      }
    });

    this.classEl.appendChild(document.createTextNode("\""));

    this.previewEl.appendChild(document.createTextNode(">"));

    
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

  render: function(playerFront) {
    this.playerFront = playerFront;
    this.inspector.walker.getNodeFromActor(playerFront.actorID, ["node"]).then(nodeFront => {
      
      if (!this.el || !nodeFront) {
        return;
      }

      this.nodeFront = nodeFront;
      let {tagName, attributes} = nodeFront;

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
        this.classEl.querySelector(".attribute-value").textContent =
          attributes[classIndex].value;
        this.classEl.style.display = "inline";
      } else {
        this.classEl.style.display = "none";
      }

      this.emit("target-retrieved");
    }, e => {
      this.nodeFront = null;
      if (!this.el) {
        console.warn("Cound't retrieve the animation target node, widget destroyed");
      } else {
        console.error(e);
      }
    });
  }
};










function createNode(options) {
  if (!options.parent) {
    throw new Error("Missing parent DOMNode to create new node");
  }

  let type = options.nodeType || "div";
  let node = options.parent.ownerDocument.createElement(type);

  for (let name in options.attributes || {}) {
    let value = options.attributes[name];
    node.setAttribute(name, value);
  }

  options.parent.appendChild(node);
  return node;
}

exports.createNode = createNode;
