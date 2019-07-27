#ifdef 0



#endif

let gCustomize = {
  _nodeIDSuffixes: [
    "blank",
    "button",
    "classic",
    "enhanced",
    "panel",
    "what",
  ],

  _nodes: {},

  init: function() {
    for (let idSuffix of this._nodeIDSuffixes) {
      this._nodes[idSuffix] = document.getElementById("newtab-customize-" + idSuffix);
    }

    this._nodes.button.addEventListener("click", e => this.showPanel());
    this._nodes.blank.addEventListener("click", e => {
      gAllPages.enabled = false;
    });
    this._nodes.classic.addEventListener("click", e => {
      gAllPages.enabled = true;
      gAllPages.enhanced = false;
    });
    this._nodes.enhanced.addEventListener("click", e => {
      gAllPages.enabled = true;
      gAllPages.enhanced = true;
    });
    this._nodes.what.addEventListener("click", e => {
      gIntro.showPanel();
    });

    this.updateSelected();
  },

  showPanel: function() {
    let nodes = this._nodes;
    let {button, panel} = nodes;
    if (button.hasAttribute("active")) {
      return Promise.resolve(nodes);
    }

    panel.openPopup(button);
    button.setAttribute("active", true);
    panel.addEventListener("popuphidden", function onHidden() {
      panel.removeEventListener("popuphidden", onHidden);
      button.removeAttribute("active");
    });

    return new Promise(resolve => {
      panel.addEventListener("popupshown", function onShown() {
        panel.removeEventListener("popupshown", onShown);
        resolve(nodes);
      });
    });
  },

  updateSelected: function() {
    let {enabled, enhanced} = gAllPages;
    let selected = enabled ? enhanced ? "enhanced" : "classic" : "blank";
    ["enhanced", "classic", "blank"].forEach(id => {
      let node = this._nodes[id];
      if (id == selected) {
        node.setAttribute("selected", true);
      }
      else {
        node.removeAttribute("selected");
      }
    });
  },
};
