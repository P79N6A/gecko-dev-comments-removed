#ifdef 0



#endif

let gSearch = {

  currentEngineName: null,

  init: function () {
    for (let idSuffix of this._nodeIDSuffixes) {
      this._nodes[idSuffix] =
        document.getElementById("newtab-search-" + idSuffix);
    }

    window.addEventListener("ContentSearchService", this);
    this._send("GetState");
  },

  showPanel: function () {
    let panel = this._nodes.panel;
    let logo = this._nodes.logo;
    panel.openPopup(logo);
    logo.setAttribute("active", "true");
    panel.addEventListener("popuphidden", function onHidden() {
      panel.removeEventListener("popuphidden", onHidden);
      logo.removeAttribute("active");
    });
  },

  search: function (event) {
    if (event) {
      event.preventDefault();
    }
    let searchStr = this._nodes.text.value;
    if (this.currentEngineName && searchStr.length) {
      this._send("Search", {
        engineName: this.currentEngineName,
        searchString: searchStr,
        whence: "newtab",
      });
    }
    this._suggestionController.addInputValueToFormHistory();
  },

  manageEngines: function () {
    this._nodes.panel.hidePopup();
    this._send("ManageEngines");
  },

  handleEvent: function (event) {
    let methodName = "on" + event.detail.type;
    if (this.hasOwnProperty(methodName)) {
      this[methodName](event.detail.data);
    }
  },

  onState: function (data) {
    this._newEngines = data.engines;
    this._setCurrentEngine(data.currentEngine);
    this._initWhenInitalStateReceived();
  },

  onCurrentState: function (data) {
    if (this._initialStateReceived) {
      this._newEngines = data.engines;
      this._setCurrentEngine(data.currentEngine);
    }
  },

  onCurrentEngine: function (engineName) {
    if (this._initialStateReceived) {
      this._nodes.panel.hidePopup();
      this._setCurrentEngine(engineName);
    }
  },

  onFocusInput: function () {
    this._nodes.text.focus();
  },

  _nodeIDSuffixes: [
    "form",
    "logo",
    "manage",
    "panel",
    "text",
  ],

  _nodes: {},

  _initWhenInitalStateReceived: function () {
    this._nodes.form.addEventListener("submit", e => this.search(e));
    this._nodes.logo.addEventListener("click", e => this.showPanel());
    this._nodes.manage.addEventListener("click", e => this.manageEngines());
    this._nodes.panel.addEventListener("popupshowing", e => this._setUpPanel());
    this._initialStateReceived = true;
    this._initWhenInitalStateReceived = function () {};
  },

  _send: function (type, data=null) {
    window.dispatchEvent(new CustomEvent("ContentSearchClient", {
      detail: {
        type: type,
        data: data,
      },
    }));
  },

  _setUpPanel: function () {
    
    if (this._newEngines) {
      this._buildPanel(this._newEngines);
      delete this._newEngines;
    }

    
    let panel = this._nodes.panel;
    for (let box of panel.childNodes) {
      if (box.getAttribute("engine") == this.currentEngineName) {
        box.setAttribute("selected", "true");
      }
      else {
        box.removeAttribute("selected");
      }
    }
  },

  _buildPanel: function (engines) {
    let panel = this._nodes.panel;

    
    let i = 0;
    while (i < panel.childNodes.length) {
      let node = panel.childNodes[i];
      if (node != this._nodes.manage) {
        panel.removeChild(node);
      }
      else {
        i++;
      }
    }

    
    for (let engine of engines) {
      panel.insertBefore(this._makePanelEngine(panel, engine),
                         this._nodes.manage);
    }
  },

  _makePanelEngine: function (panel, engine) {
    let box = document.createElementNS(XUL_NAMESPACE, "hbox");
    box.className = "newtab-search-panel-engine";
    box.setAttribute("engine", engine.name);

    box.addEventListener("click", () => {
      this._send("SetCurrentEngine", engine.name);
      panel.hidePopup();
      this._nodes.text.focus();
    });

    let image = document.createElementNS(XUL_NAMESPACE, "image");
    if (engine.iconBuffer) {
      let blob = new Blob([engine.iconBuffer]);
      let size = Math.round(16 * window.devicePixelRatio);
      let sizeStr = size + "," + size;
      let uri = URL.createObjectURL(blob) + "#-moz-resolution=" + sizeStr;
      image.setAttribute("src", uri);
    }
    box.appendChild(image);

    let label = document.createElementNS(XUL_NAMESPACE, "label");
    label.setAttribute("value", engine.name);
    box.appendChild(label);

    return box;
  },

  _setCurrentEngine: function (engine) {
    this.currentEngineName = engine.name;

    
    let logoBuf = window.devicePixelRatio == 2 ? engine.logo2xBuffer :
                  engine.logoBuffer || engine.logo2xBuffer;
    if (logoBuf) {
      this._nodes.logo.hidden = false;
      let uri = URL.createObjectURL(new Blob([logoBuf]));
      this._nodes.logo.style.backgroundImage = "url(" + uri + ")";
      this._nodes.text.placeholder = "";
    }
    else {
      this._nodes.logo.hidden = true;
      this._nodes.text.placeholder = engine.name;
    }

    
    if (!this._suggestionController) {
      let parent = document.getElementById("newtab-scrollbox");
      this._suggestionController =
        new SearchSuggestionUIController(this._nodes.text, parent,
                                         () => this.search());
    }
    this._suggestionController.engineName = engine.name;
  },
};
