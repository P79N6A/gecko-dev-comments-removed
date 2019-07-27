


"use strict";




function EventListenersView(DebuggerController) {
  dumpn("EventListenersView was instantiated");

  this.Breakpoints = DebuggerController.Breakpoints;

  this._onCheck = this._onCheck.bind(this);
  this._onClick = this._onClick.bind(this);
}

EventListenersView.prototype = Heritage.extend(WidgetMethods, {
  


  initialize: function() {
    dumpn("Initializing the EventListenersView");

    this.widget = new SideMenuWidget(document.getElementById("event-listeners"), {
      showItemCheckboxes: true,
      showGroupCheckboxes: true
    });

    this.emptyText = L10N.getStr("noEventListenersText");
    this._eventCheckboxTooltip = L10N.getStr("eventCheckboxTooltip");
    this._onSelectorString = " " + L10N.getStr("eventOnSelector") + " ";
    this._inSourceString = " " + L10N.getStr("eventInSource") + " ";
    this._inNativeCodeString = L10N.getStr("eventNative");

    this.widget.addEventListener("check", this._onCheck, false);
    this.widget.addEventListener("click", this._onClick, false);
  },

  


  destroy: function() {
    dumpn("Destroying the EventListenersView");

    this.widget.removeEventListener("check", this._onCheck, false);
    this.widget.removeEventListener("click", this._onClick, false);
  },

  








  addListener: function(aListener, aOptions = {}) {
    let { node: { selector }, function: { url }, type } = aListener;
    if (!type) return;

    
    
    if (!url) {
      url = this._inNativeCodeString;
    }

    
    
    let eventItem = this.getItemForPredicate(aItem =>
      aItem.attachment.url == url &&
      aItem.attachment.type == type);

    if (eventItem) {
      let { selectors, view: { targets } } = eventItem.attachment;
      if (selectors.indexOf(selector) == -1) {
        selectors.push(selector);
        targets.setAttribute("value", L10N.getFormatStr("eventNodes", selectors.length));
      }
      return;
    }

    
    
    let is = (...args) => args.indexOf(type) != -1;
    let has = str => type.contains(str);
    let starts = str => type.startsWith(str);
    let group;

    if (starts("animation")) {
      group = L10N.getStr("animationEvents");
    } else if (starts("audio")) {
      group = L10N.getStr("audioEvents");
    } else if (is("levelchange")) {
      group = L10N.getStr("batteryEvents");
    } else if (is("cut", "copy", "paste")) {
      group = L10N.getStr("clipboardEvents");
    } else if (starts("composition")) {
      group = L10N.getStr("compositionEvents");
    } else if (starts("device")) {
      group = L10N.getStr("deviceEvents");
    } else if (is("fullscreenchange", "fullscreenerror", "orientationchange",
      "overflow", "resize", "scroll", "underflow", "zoom")) {
      group = L10N.getStr("displayEvents");
    } else if (starts("drag") || starts("drop")) {
      group = L10N.getStr("dragAndDropEvents");
    } else if (starts("gamepad")) {
      group = L10N.getStr("gamepadEvents");
    } else if (is("canplay", "canplaythrough", "durationchange", "emptied",
      "ended", "loadeddata", "loadedmetadata", "pause", "play", "playing",
      "ratechange", "seeked", "seeking", "stalled", "suspend", "timeupdate",
      "volumechange", "waiting")) {
      group = L10N.getStr("mediaEvents");
    } else if (is("blocked", "complete", "success", "upgradeneeded", "versionchange")) {
      group = L10N.getStr("indexedDBEvents");
    } else if (is("blur", "change", "focus", "focusin", "focusout", "invalid",
      "reset", "select", "submit")) {
      group = L10N.getStr("interactionEvents");
    } else if (starts("key") || is("input")) {
      group = L10N.getStr("keyboardEvents");
    } else if (starts("mouse") || has("click") || is("contextmenu", "show", "wheel")) {
      group = L10N.getStr("mouseEvents");
    } else if (starts("DOM")) {
      group = L10N.getStr("mutationEvents");
    } else if (is("abort", "error", "hashchange", "load", "loadend", "loadstart",
      "pagehide", "pageshow", "progress", "timeout", "unload", "uploadprogress",
      "visibilitychange")) {
      group = L10N.getStr("navigationEvents");
    } else if (is("pointerlockchange", "pointerlockerror")) {
      group = L10N.getStr("pointerLockEvents");
    } else if (is("compassneedscalibration", "userproximity")) {
      group = L10N.getStr("sensorEvents");
    } else if (starts("storage")) {
      group = L10N.getStr("storageEvents");
    } else if (is("beginEvent", "endEvent", "repeatEvent")) {
      group = L10N.getStr("timeEvents");
    } else if (starts("touch")) {
      group = L10N.getStr("touchEvents");
    } else {
      group = L10N.getStr("otherEvents");
    }

    
    let itemView = this._createItemView(type, selector, url);

    
    
    let checkboxState =
      this.Breakpoints.DOM.activeEventNames.indexOf(type) != -1;

    
    this.push([itemView.container], {
      staged: aOptions.staged, 
      attachment: {
        url: url,
        type: type,
        view: itemView,
        selectors: [selector],
        group: group,
        checkboxState: checkboxState,
        checkboxTooltip: this._eventCheckboxTooltip
      }
    });
  },

  





  getAllEvents: function() {
    return this.attachments.map(e => e.type);
  },

  





  getCheckedEvents: function() {
    return this.attachments.filter(e => e.checkboxState).map(e => e.type);
  },

  











  _createItemView: function(aType, aSelector, aUrl) {
    let container = document.createElement("hbox");
    container.className = "dbg-event-listener";

    let eventType = document.createElement("label");
    eventType.className = "plain dbg-event-listener-type";
    eventType.setAttribute("value", aType);
    container.appendChild(eventType);

    let typeSeparator = document.createElement("label");
    typeSeparator.className = "plain dbg-event-listener-separator";
    typeSeparator.setAttribute("value", this._onSelectorString);
    container.appendChild(typeSeparator);

    let eventTargets = document.createElement("label");
    eventTargets.className = "plain dbg-event-listener-targets";
    eventTargets.setAttribute("value", aSelector);
    container.appendChild(eventTargets);

    let selectorSeparator = document.createElement("label");
    selectorSeparator.className = "plain dbg-event-listener-separator";
    selectorSeparator.setAttribute("value", this._inSourceString);
    container.appendChild(selectorSeparator);

    let eventLocation = document.createElement("label");
    eventLocation.className = "plain dbg-event-listener-location";
    eventLocation.setAttribute("value", SourceUtils.getSourceLabel(aUrl));
    eventLocation.setAttribute("flex", "1");
    eventLocation.setAttribute("crop", "center");
    container.appendChild(eventLocation);

    return {
      container: container,
      type: eventType,
      targets: eventTargets,
      location: eventLocation
    };
  },

  


  _onCheck: function({ detail: { description, checked }, target }) {
    if (description == "item") {
      this.getItemForElement(target).attachment.checkboxState = checked;
      this.Breakpoints.DOM.scheduleEventBreakpointsUpdate();
      return;
    }

    
    this.items
      .filter(e => e.attachment.group == description)
      .forEach(e => this.callMethod("checkItem", e.target, checked));
  },

  


  _onClick: function({ target }) {
    
    
    
    let eventItem = this.getItemForElement(target, { noSiblings: true });
    if (eventItem) {
      let newState = eventItem.attachment.checkboxState ^= 1;
      this.callMethod("checkItem", eventItem.target, newState);
    }
  },

  _eventCheckboxTooltip: "",
  _onSelectorString: "",
  _inSourceString: "",
  _inNativeCodeString: ""
});

DebuggerView.EventListeners = new EventListenersView(DebuggerController);
