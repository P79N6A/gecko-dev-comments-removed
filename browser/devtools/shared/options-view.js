const EventEmitter = require("devtools/toolkit/event-emitter");
const { Services } = require("resource://gre/modules/Services.jsm");

const OPTIONS_SHOWN_EVENT = "options-shown";
const OPTIONS_HIDDEN_EVENT = "options-hidden";
const PREF_CHANGE_EVENT = "pref-changed";










const OptionsView = function (options={}) {
  this.branchName = options.branchName;
  this.menupopup = options.menupopup;
  this.window = this.menupopup.ownerDocument.defaultView;
  let { document } = this.window;
  this.$ = document.querySelector.bind(document);
  this.$$ = (selector, parent=document) => parent.querySelectorAll(selector);
  
  
  this.button = this.$(`[popup=${this.menupopup.getAttribute("id")}]`);

  this.prefObserver = new PrefObserver(this.branchName);

  EventEmitter.decorate(this);
};
exports.OptionsView = OptionsView;

OptionsView.prototype = {
  


  initialize: function () {
    let { MutationObserver } = this.window;
    this._onPrefChange = this._onPrefChange.bind(this);
    this._onOptionChange = this._onOptionChange.bind(this);
    this._onPopupShown = this._onPopupShown.bind(this);
    this._onPopupHidden = this._onPopupHidden.bind(this);

    
    
    
    this.mutationObserver = new MutationObserver(this._onOptionChange);
    let observerConfig = { attributes: true, attributeFilter: ["checked"]};

    
    for (let $el of this.$$("menuitem", this.menupopup)) {
      let prefName = $el.getAttribute("data-pref");

      if (this.prefObserver.get(prefName)) {
        $el.setAttribute("checked", "true");
      } else {
        $el.removeAttribute("checked");
      }
      this.mutationObserver.observe($el, observerConfig);
    }

    
    this.prefObserver.register();
    this.prefObserver.on(PREF_CHANGE_EVENT, this._onPrefChange);

    
    this.menupopup.addEventListener("popupshown", this._onPopupShown);
    this.menupopup.addEventListener("popuphidden", this._onPopupHidden);
  },

  



  destroy: function () {
    this.mutationObserver.disconnect();
    this.prefObserver.off(PREF_CHANGE_EVENT, this._onPrefChange);
    this.menupopup.removeEventListener("popupshown", this._onPopupShown);
    this.menupopup.removeEventListener("popuphidden", this._onPopupHidden);
  },

  


  getPref: function (prefName) {
    return this.prefObserver.get(prefName);
  },

  




  _onPrefChange: function (_, prefName) {
    let $el = this.$(`menuitem[data-pref="${prefName}"]`, this.menupopup);
    let value = this.prefObserver.get(prefName);

    
    
    if (!$el) {
      this.emit(PREF_CHANGE_EVENT, prefName);
      return;
    }

    if (value) {
      $el.setAttribute("checked", value);
    } else {
      $el.removeAttribute("checked");
    }

    this.emit(PREF_CHANGE_EVENT, prefName);
  },

  



  _onOptionChange: function (mutations) {
    let { target } = mutations[0];
    let prefName = target.getAttribute("data-pref");
    let value = target.getAttribute("checked") === "true";

    this.prefObserver.set(prefName, value);
  },

  



  _onPopupShown: function () {
    this.button.setAttribute("open", true);
    this.emit(OPTIONS_SHOWN_EVENT);
  },

  



  _onPopupHidden: function () {
    this.button.removeAttribute("open");
    this.emit(OPTIONS_HIDDEN_EVENT);
  }
};








const PrefObserver = function (branchName) {
  this.branchName = branchName;
  this.branch = Services.prefs.getBranch(branchName);
  EventEmitter.decorate(this);
};

PrefObserver.prototype = {
  


  get: function (prefName) {
    let fullName = this.branchName + prefName;
    return Services.prefs.getBoolPref(fullName);
  },
  


  set: function (prefName, value) {
    let fullName = this.branchName + prefName;
    Services.prefs.setBoolPref(fullName, value);
  },
  register: function () {
    this.branch.addObserver("", this, false);
  },
  unregister: function () {
    this.branch.removeObserver("", this);
  },
  observe: function (subject, topic, prefName) {
    this.emit(PREF_CHANGE_EVENT, prefName);
  }
};
