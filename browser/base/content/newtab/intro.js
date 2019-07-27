#ifdef 0



#endif

const PREF_INTRO_SHOWN = "browser.newtabpage.introShown";

let gIntro = {
  _introShown: Services.prefs.getBoolPref(PREF_INTRO_SHOWN),

  _nodeIDSuffixes: [
    "panel",
  ],

  _nodes: {},

  init: function() {
    for (let idSuffix of this._nodeIDSuffixes) {
      this._nodes[idSuffix] = document.getElementById("newtab-intro-" + idSuffix);
    }

    this._nodes.panel.addEventListener("popupshowing", e => this._setUpPanel());
  },

  showIfNecessary: function() {
    if (!this._introShown) {
      Services.prefs.setBoolPref(PREF_INTRO_SHOWN, true);
      this.showPanel();
    }
  },

  showPanel: function() {
    
    gCustomize.showPanel().then(nodes => {
      
      this._nodes.panel.openPopup(nodes.what);
    });
  },

  _setUpPanel: function() {
    
    if (this._nodes.panel.childNodes.length == 1) {
      ['<a href="' + TILES_EXPLAIN_LINK + '">' + newTabString("learn.link") + "</a>",
       '<a href="' + TILES_PRIVACY_LINK + '">' + newTabString("privacy.link") + "</a>",
       '<input type="button" class="newtab-customize"/>',
      ].forEach((arg, index) => {
        let paragraph = document.createElementNS(HTML_NAMESPACE, "p");
        this._nodes.panel.appendChild(paragraph);
        paragraph.innerHTML = newTabString("intro.paragraph" + (index + 1), [arg]);
      });
    }
  },
};
