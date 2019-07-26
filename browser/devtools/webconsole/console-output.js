




"use strict";

const {Cc, Ci, Cu} = require("chrome");

loader.lazyImporter(this, "VariablesView", "resource:///modules/devtools/VariablesView.jsm");

const Heritage = require("sdk/core/heritage");
const XHTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";

const WebConsoleUtils = require("devtools/toolkit/webconsole/utils").Utils;
const l10n = new WebConsoleUtils.l10n(STRINGS_URI);




const COMPAT = {
  
  CATEGORIES: {
    NETWORK: 0,
    CSS: 1,
    JS: 2,
    WEBDEV: 3,
    INPUT: 4,
    OUTPUT: 5,
    SECURITY: 6,
  },

  
  SEVERITIES: {
    ERROR: 0,
    WARNING: 1,
    INFO: 2,
    LOG: 3,
  },

  
  
  
  
  
  PREFERENCE_KEYS: [
    
    [ "network",    "netwarn",    null,   "networkinfo", ],  
    [ "csserror",   "cssparser",  null,   null,          ],  
    [ "exception",  "jswarn",     null,   "jslog",       ],  
    [ "error",      "warn",       "info", "log",         ],  
    [ null,         null,         null,   null,          ],  
    [ null,         null,         null,   null,          ],  
    [ "secerror",   "secwarn",    null,   null,          ],  
  ],

  
  CATEGORY_CLASS_FRAGMENTS: [ "network", "cssparser", "exception", "console",
                              "input", "output", "security" ],

  
  SEVERITY_CLASS_FRAGMENTS: [ "error", "warn", "info", "log" ],

  
  GROUP_INDENT: 12,
};


const CONSOLE_API_LEVELS_TO_SEVERITIES = {
  error: "error",
  exception: "error",
  assert: "error",
  warn: "warning",
  info: "info",
  log: "log",
  trace: "log",
  debug: "log",
  dir: "log",
  group: "log",
  groupCollapsed: "log",
  groupEnd: "log",
  time: "log",
  timeEnd: "log"
};


const IGNORED_SOURCE_URLS = ["debugger eval code", "self-hosted"];


const MAX_LONG_STRING_LENGTH = 200000;
















function ConsoleOutput(owner)
{
  this.owner = owner;
  this._onFlushOutputMessage = this._onFlushOutputMessage.bind(this);
}

ConsoleOutput.prototype = {
  



  get element() {
    return this.owner.outputNode;
  },

  



  get document() {
    return this.owner.document;
  },

  



  get window() {
    return this.owner.window;
  },

  



  get webConsoleClient() {
    return this.owner.webConsoleClient;
  },

  






  addMessage: function(...args)
  {
    for (let msg of args) {
      msg.init(this);
      this.owner.outputMessage(msg._categoryCompat, this._onFlushOutputMessage,
                               [msg]);
    }
    return this;
  },

  













  _onFlushOutputMessage: function(message)
  {
    return message.render().element;
  },

  









  getSelectedMessages: function(limit)
  {
    let selection = this.window.getSelection();
    if (selection.isCollapsed) {
      return [];
    }

    if (selection.containsNode(this.element, true)) {
      return Array.slice(this.element.children);
    }

    let anchor = this.getMessageForElement(selection.anchorNode);
    let focus = this.getMessageForElement(selection.focusNode);
    if (!anchor || !focus) {
      return [];
    }

    let start, end;
    if (anchor.timestamp > focus.timestamp) {
      start = focus;
      end = anchor;
    } else {
      start = anchor;
      end = focus;
    }

    let result = [];
    let current = start;
    while (current) {
      result.push(current);
      if (current == end || (limit && result.length == limit)) {
        break;
      }
      current = current.nextSibling;
    }
    return result;
  },

  







  getMessageForElement: function(elem)
  {
    while (elem && elem.parentNode) {
      if (elem.classList && elem.classList.contains("message")) {
        return elem;
      }
      elem = elem.parentNode;
    }
    return null;
  },

  


  selectAllMessages: function()
  {
    let selection = this.window.getSelection();
    selection.removeAllRanges();
    let range = this.document.createRange();
    range.selectNodeContents(this.element);
    selection.addRange(range);
  },

  





  selectMessage: function(elem)
  {
    let selection = this.window.getSelection();
    selection.removeAllRanges();
    let range = this.document.createRange();
    range.selectNodeContents(elem);
    selection.addRange(range);
  },

  



  openLink: function()
  {
    this.owner.owner.openLink.apply(this.owner.owner, arguments);
  },

  



  openVariablesView: function()
  {
    this.owner.jsterm.openVariablesView.apply(this.owner.jsterm, arguments);
  },

  


  destroy: function()
  {
    this.owner = null;
  },
}; 





let Messages = {};







Messages.BaseMessage = function()
{
  this.widgets = new Set();
  this._onClickAnchor = this._onClickAnchor.bind(this);
  this._repeatID = { uid: gSequenceId() };
  this.textContent = "";
};

Messages.BaseMessage.prototype = {
  





  output: null,

  





  parent: null,

  





  element: null,

  



  get visible() {
    return this.element && this.element.parentNode;
  },

  



  get document() {
    return this.output.document;
  },

  



  textContent: null,

  



  widgets: null,

  
  
  _categoryCompat: null,
  _severityCompat: null,
  _categoryNameCompat: null,
  _severityNameCompat: null,
  _filterKeyCompat: null,

  





  _repeatID: null,

  








  init: function(output, parent=null)
  {
    this.output = output;
    this.parent = parent;
    return this;
  },

  





  getRepeatID: function()
  {
    return JSON.stringify(this._repeatID);
  },

  




  render: function()
  {
    if (!this.element) {
      this.element = this._renderCompat();
    }
    return this;
  },

  








  _renderCompat: function()
  {
    let doc = this.output.document;
    let container = doc.createElementNS(XHTML_NS, "div");
    container.id = "console-msg-" + gSequenceId();
    container.className = "message";
    container.category = this._categoryCompat;
    container.severity = this._severityCompat;
    container.setAttribute("category", this._categoryNameCompat);
    container.setAttribute("severity", this._severityNameCompat);
    container.setAttribute("filter", this._filterKeyCompat);
    container.clipboardText = this.textContent;
    container.timestamp = this.timestamp;
    container._messageObject = this;

    return container;
  },

  









  _addLinkCallback: function(element, callback = this._onClickAnchor)
  {
    
    
    
    
    
    this.output.owner._addMessageLinkCallback(element, callback);
  },

  







  _onClickAnchor: function(event)
  {
    this.output.openLink(event.target.href);
  },
}; 













Messages.NavigationMarker = function(url, timestamp)
{
  Messages.BaseMessage.call(this);
  this._url = url;
  this.textContent = "------ " + url;
  this.timestamp = timestamp;
};

Messages.NavigationMarker.prototype = Heritage.extend(Messages.BaseMessage.prototype,
{
  




  _url: null,

  





  timestamp: 0,

  _categoryCompat: COMPAT.CATEGORIES.NETWORK,
  _severityCompat: COMPAT.SEVERITIES.LOG,
  _categoryNameCompat: "network",
  _severityNameCompat: "info",
  _filterKeyCompat: "networkinfo",

  



  render: function()
  {
    if (this.element) {
      return this;
    }

    let url = this._url;
    let pos = url.indexOf("?");
    if (pos > -1) {
      url = url.substr(0, pos);
    }

    let doc = this.output.document;
    let urlnode = doc.createElementNS(XHTML_NS, "a");
    urlnode.className = "url";
    urlnode.textContent = url;
    urlnode.title = this._url;
    urlnode.href = this._url;
    urlnode.draggable = false;
    this._addLinkCallback(urlnode);

    let render = Messages.BaseMessage.prototype.render.bind(this);
    render().element.appendChild(urlnode);
    this.element.classList.add("navigation-marker");
    this.element.url = this._url;
    this.element.appendChild(doc.createTextNode("\n"));

    return this;
  },
}); 





























Messages.Simple = function(message, options = {})
{
  Messages.BaseMessage.call(this);

  this.category = options.category;
  this.severity = options.severity;
  this.location = options.location;
  this.timestamp = options.timestamp || Date.now();
  this.private = !!options.private;

  this._message = message;
  this._className = options.className;
  this._link = options.link;
  this._linkCallback = options.linkCallback;
  this._filterDuplicates = options.filterDuplicates;
};

Messages.Simple.prototype = Heritage.extend(Messages.BaseMessage.prototype,
{
  



  category: null,

  



  severity: null,

  



  location: null,

  



  private: false,

  




  _className: null,

  




  _link: null,

  




  _linkCallback: null,

  



  _filterDuplicates: false,

  






  _message: null,

  _afterMessage: null,
  _objectActors: null,
  _groupDepthCompat: 0,

  





  timestamp: 0,

  get _categoryCompat() {
    return this.category ?
           COMPAT.CATEGORIES[this.category.toUpperCase()] : null;
  },
  get _severityCompat() {
    return this.severity ?
           COMPAT.SEVERITIES[this.severity.toUpperCase()] : null;
  },
  get _categoryNameCompat() {
    return this.category ?
           COMPAT.CATEGORY_CLASS_FRAGMENTS[this._categoryCompat] : null;
  },
  get _severityNameCompat() {
    return this.severity ?
           COMPAT.SEVERITY_CLASS_FRAGMENTS[this._severityCompat] : null;
  },

  get _filterKeyCompat() {
    return this._categoryCompat !== null && this._severityCompat !== null ?
           COMPAT.PREFERENCE_KEYS[this._categoryCompat][this._severityCompat] :
           null;
  },

  init: function()
  {
    Messages.BaseMessage.prototype.init.apply(this, arguments);
    this._groupDepthCompat = this.output.owner.groupDepth;
    this._initRepeatID();
    return this;
  },

  _initRepeatID: function()
  {
    if (!this._filterDuplicates) {
      return;
    }

    
    let rid = this._repeatID;
    delete rid.uid;

    rid.category = this.category;
    rid.severity = this.severity;
    rid.private = this.private;
    rid.location = this.location;
    rid.link = this._link;
    rid.linkCallback = this._linkCallback + "";
    rid.className = this._className;
    rid.groupDepth = this._groupDepthCompat;
    rid.textContent = "";
  },

  getRepeatID: function()
  {
    
    
    if (this._repeatID.uid) {
      return JSON.stringify({ uid: this._repeatID.uid });
    }

    return JSON.stringify(this._repeatID);
  },

  render: function()
  {
    if (this.element) {
      return this;
    }

    let timestamp = new Widgets.MessageTimestamp(this, this.timestamp).render();

    let icon = this.document.createElementNS(XHTML_NS, "span");
    icon.className = "icon";

    let body = this._renderBody();
    this._repeatID.textContent += "|" + body.textContent;

    let repeatNode = this._renderRepeatNode();
    let location = this._renderLocation();

    Messages.BaseMessage.prototype.render.call(this);
    if (this._className) {
      this.element.className += " " + this._className;
    }

    this.element.appendChild(timestamp.element);
    this.element.appendChild(icon);
    this.element.appendChild(body);
    if (repeatNode) {
      this.element.appendChild(repeatNode);
    }
    if (location) {
      this.element.appendChild(location);
    }
    this.element.appendChild(this.document.createTextNode("\n"));

    this.element.clipboardText = this.element.textContent;

    if (this.private) {
      this.element.setAttribute("private", true);
    }

    if (this._afterMessage) {
      this.element._outputAfterNode = this._afterMessage.element;
      this._afterMessage = null;
    }

    
    
    this.element._objectActors = this._objectActors;
    this._objectActors = null;

    return this;
  },

  




  _renderBody: function()
  {
    let body = this.document.createElementNS(XHTML_NS, "span");
    body.className = "body devtools-monospace";

    let anchor, container = body;
    if (this._link || this._linkCallback) {
      container = anchor = this.document.createElementNS(XHTML_NS, "a");
      anchor.href = this._link || "#";
      anchor.draggable = false;
      this._addLinkCallback(anchor, this._linkCallback);
      body.appendChild(anchor);
    }

    if (typeof this._message == "function") {
      container.appendChild(this._message(this));
    } else if (this._message instanceof Ci.nsIDOMNode) {
      container.appendChild(this._message);
    } else {
      container.textContent = this._message;
    }

    return body;
  },

  




  _renderRepeatNode: function()
  {
    if (!this._filterDuplicates) {
      return null;
    }

    let repeatNode = this.document.createElementNS(XHTML_NS, "span");
    repeatNode.setAttribute("value", "1");
    repeatNode.className = "repeats";
    repeatNode.textContent = 1;
    repeatNode._uid = this.getRepeatID();
    return repeatNode;
  },

  




  _renderLocation: function()
  {
    if (!this.location) {
      return null;
    }

    let {url, line} = this.location;
    if (IGNORED_SOURCE_URLS.indexOf(url) != -1) {
      return null;
    }

    
    
    return this.output.owner.createLocationNode(url, line);
  },
}); 
















Messages.Extended = function(messagePieces, options = {})
{
  Messages.Simple.call(this, null, options);

  this._messagePieces = messagePieces;

  if ("quoteStrings" in options) {
    this._quoteStrings = options.quoteStrings;
  }

  this._repeatID.quoteStrings = this._quoteStrings;
  this._repeatID.messagePieces = messagePieces + "";
  this._repeatID.actors = new Set(); 
};

Messages.Extended.prototype = Heritage.extend(Messages.Simple.prototype,
{
  




  _messagePieces: null,

  




  _quoteStrings: true,

  getRepeatID: function()
  {
    if (this._repeatID.uid) {
      return JSON.stringify({ uid: this._repeatID.uid });
    }

    
    let actors = this._repeatID.actors;
    this._repeatID.actors = [...actors];
    let result = JSON.stringify(this._repeatID);
    this._repeatID.actors = actors;
    return result;
  },

  render: function()
  {
    let result = this.document.createDocumentFragment();

    for (let i = 0; i < this._messagePieces.length; i++) {
      let separator = i > 0 ? this._renderBodyPieceSeparator() : null;
      if (separator) {
        result.appendChild(separator);
      }

      let piece = this._messagePieces[i];
      result.appendChild(this._renderBodyPiece(piece));
    }

    this._message = result;
    this._messagePieces = null;
    return Messages.Simple.prototype.render.call(this);
  },

  





  _renderBodyPieceSeparator: function() { return null; },

  








  _renderBodyPiece: function(piece)
  {
    if (piece instanceof Ci.nsIDOMNode) {
      return piece;
    }
    if (typeof piece == "function") {
      return piece(this);
    }

    let isPrimitive = VariablesView.isPrimitive({ value: piece });
    let isActorGrip = WebConsoleUtils.isActorGrip(piece);

    if (isActorGrip) {
      this._repeatID.actors.add(piece.actor);

      if (!isPrimitive) {
        let widget = new Widgets.JSObject(this, piece).render();
        return widget.element;
      }
      if (piece.type == "longString") {
        let widget = new Widgets.LongString(this, piece).render();
        return widget.element;
      }
    }

    let result = this.document.createDocumentFragment();
    if (!isPrimitive || (!this._quoteStrings && typeof piece == "string")) {
      result.textContent = piece;
    } else {
      result.textContent = VariablesView.getString(piece);
    }

    return result;
  },
}); 













Messages.JavaScriptEvalOutput = function(evalResponse, errorMessage)
{
  let severity = "log", msg, quoteStrings = true;

  if (errorMessage) {
    severity = "error";
    msg = errorMessage;
    quoteStrings = false;
  } else {
    msg = evalResponse.result;
  }

  let options = {
    timestamp: evalResponse.timestamp,
    category: "output",
    severity: severity,
    quoteStrings: quoteStrings,
  };
  Messages.Extended.call(this, [msg], options);
};

Messages.JavaScriptEvalOutput.prototype = Messages.Extended.prototype;









Messages.ConsoleGeneric = function(packet)
{
  let options = {
    timestamp: packet.timeStamp,
    category: "webdev",
    severity: CONSOLE_API_LEVELS_TO_SEVERITIES[packet.level],
    private: packet.private,
    filterDuplicates: true,
    location: {
      url: packet.filename,
      line: packet.lineNumber,
    },
  };
  Messages.Extended.call(this, packet.arguments, options);
  this._repeatID.consoleApiLevel = packet.level;
};

Messages.ConsoleGeneric.prototype = Heritage.extend(Messages.Extended.prototype,
{
  _renderBodyPieceSeparator: function()
  {
    return this.document.createTextNode(" ");
  },
}); 


let Widgets = {};








Widgets.BaseWidget = function(message)
{
  this.message = message;
};

Widgets.BaseWidget.prototype = {
  



  message: null,

  



  element: null,

  



  get document() {
    return this.message.document;
  },

  


  get output() {
    return this.message.output;
  },

  



  render: function() { },

  


  destroy: function() { },
};










Widgets.MessageTimestamp = function(message, timestamp)
{
  Widgets.BaseWidget.call(this, message);
  this.timestamp = timestamp;
};

Widgets.MessageTimestamp.prototype = Heritage.extend(Widgets.BaseWidget.prototype,
{
  



  timestamp: 0,

  render: function()
  {
    if (this.element) {
      return this;
    }

    this.element = this.document.createElementNS(XHTML_NS, "span");
    this.element.className = "timestamp devtools-monospace";
    this.element.textContent = l10n.timestampString(this.timestamp) + " ";

    
    
    this.element.style.marginRight = this.message._groupDepthCompat *
                                     COMPAT.GROUP_INDENT + "px";

    return this;
  },
}); 











Widgets.JSObject = function(message, objectActor)
{
  Widgets.BaseWidget.call(this, message);
  this.objectActor = objectActor;
  this._onClick = this._onClick.bind(this);
};

Widgets.JSObject.prototype = Heritage.extend(Widgets.BaseWidget.prototype,
{
  



  objectActor: null,

  render: function()
  {
    if (this.element) {
      return this;
    }

    let anchor = this.element = this.document.createElementNS(XHTML_NS, "a");
    anchor.href = "#";
    anchor.draggable = false;
    anchor.textContent = VariablesView.getString(this.objectActor);
    this.message._addLinkCallback(anchor, this._onClick);

    return this;
  },

  



  _onClick: function()
  {
    this.output.openVariablesView({
      label: this.element.textContent,
      objectActor: this.objectActor,
      autofocus: true,
    });
  },
}); 










Widgets.LongString = function(message, longStringActor)
{
  Widgets.BaseWidget.call(this, message);
  this.longStringActor = longStringActor;
  this._onClick = this._onClick.bind(this);
  this._onSubstring = this._onSubstring.bind(this);
};

Widgets.LongString.prototype = Heritage.extend(Widgets.BaseWidget.prototype,
{
  



  longStringActor: null,

  render: function()
  {
    if (this.element) {
      return this;
    }

    let result = this.element = this.document.createElementNS(XHTML_NS, "span");
    result.className = "longString";
    this._renderString(this.longStringActor.initial);
    result.appendChild(this._renderEllipsis());

    return this;
  },

  





  _renderString: function(str)
  {
    if (this.message._quoteStrings) {
      this.element.textContent = VariablesView.getString(str);
    } else {
      this.element.textContent = str;
    }
  },

  





  _renderEllipsis: function()
  {
    let ellipsis = this.document.createElementNS(XHTML_NS, "a");
    ellipsis.className = "longStringEllipsis";
    ellipsis.textContent = l10n.getStr("longStringEllipsis");
    ellipsis.href = "#";
    ellipsis.draggable = false;
    this.message._addLinkCallback(ellipsis, this._onClick);

    return ellipsis;
  },

  




  _onClick: function()
  {
    let longString = this.output.webConsoleClient.longString(this.longStringActor);
    let toIndex = Math.min(longString.length, MAX_LONG_STRING_LENGTH);

    longString.substring(longString.initial.length, toIndex, this._onSubstring);
  },

  






  _onSubstring: function(response)
  {
    if (response.error) {
      Cu.reportError("LongString substring failure: " + response.error);
      return;
    }

    this.element.lastChild.remove();
    this.element.classList.remove("longString");

    this._renderString(this.longStringActor.initial + response.substring);

    this.output.owner.emit("messages-updated", new Set([this.message.element]));

    let toIndex = Math.min(this.longStringActor.length, MAX_LONG_STRING_LENGTH);
    if (toIndex != this.longStringActor.length) {
      this._logWarningAboutStringTooLong();
    }
  },

  



  _logWarningAboutStringTooLong: function()
  {
    let msg = new Messages.Simple(l10n.getStr("longStringTooLong"), {
      category: "output",
      severity: "warning",
    });
    this.output.addMessage(msg);
  },
}); 


function gSequenceId()
{
  return gSequenceId.n++;
}
gSequenceId.n = 0;

exports.ConsoleOutput = ConsoleOutput;
exports.Messages = Messages;
exports.Widgets = Widgets;
