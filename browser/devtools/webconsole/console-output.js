




"use strict";

const {Cc, Ci, Cu} = require("chrome");

const { Services } = require("resource://gre/modules/Services.jsm");

loader.lazyImporter(this, "VariablesView", "resource:///modules/devtools/VariablesView.jsm");
loader.lazyImporter(this, "escapeHTML", "resource:///modules/devtools/VariablesView.jsm");
loader.lazyImporter(this, "gDevTools", "resource:///modules/devtools/gDevTools.jsm");
loader.lazyImporter(this, "Task", "resource://gre/modules/Task.jsm");
loader.lazyImporter(this, "PluralForm", "resource://gre/modules/PluralForm.jsm");
loader.lazyImporter(this, "ObjectClient", "resource://gre/modules/devtools/dbg-client.jsm");

loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "TableWidget", "devtools/shared/widgets/TableWidget", true);

const Heritage = require("sdk/core/heritage");
const URI = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
const XHTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";

const WebConsoleUtils = require("devtools/toolkit/webconsole/utils").Utils;
const l10n = new WebConsoleUtils.l10n(STRINGS_URI);

const MAX_STRING_GRIP_LENGTH = 36;
const ELLIPSIS = Services.prefs.getComplexValue("intl.ellipsis", Ci.nsIPrefLocalizedString).data;




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
  table: "log",
  debug: "log",
  dir: "log",
  group: "log",
  groupCollapsed: "log",
  groupEnd: "log",
  time: "log",
  timeEnd: "log",
  count: "log"
};


const IGNORED_SOURCE_URLS = ["debugger eval code"];


const MAX_LONG_STRING_LENGTH = 200000;



const RE_ALLOWED_STYLES = /^(?:-moz-)?(?:background|border|box|clear|color|cursor|display|float|font|line|margin|padding|text|transition|outline|white-space|word|writing|(?:min-|max-)?width|(?:min-|max-)?height)/;



const RE_CLEANUP_STYLES = [
  
  /\b(?:url|(?:-moz-)?element)[\s('"]+/gi,

  
  /['"(]*(?:chrome|resource|about|app|data|https?|ftp|file):+\/*/gi,
];


const TABLE_ROW_MAX_ITEMS = 1000;


const TABLE_COLUMN_MAX_ITEMS = 10;















function ConsoleOutput(owner)
{
  this.owner = owner;
  this._onFlushOutputMessage = this._onFlushOutputMessage.bind(this);
}

ConsoleOutput.prototype = {
  _dummyElement: null,

  



  get element() {
    return this.owner.outputNode;
  },

  



  get document() {
    return this.owner ? this.owner.document : null;
  },

  



  get window() {
    return this.owner.window;
  },

  



  get webConsoleClient() {
    return this.owner.webConsoleClient;
  },

  



  get toolboxTarget() {
    return this.owner.owner.target;
  },

  






  _releaseObject: function(actorId)
  {
    this.owner._releaseObject(actorId);
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
    this._dummyElement = null;
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
    if (this.category == "input") {
      
      
      container.setAttribute("aria-live", "off");
    }
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

  destroy: function()
  {
    
    for (let widget of this.widgets) {
      widget.destroy();
    }
    this.widgets.clear();
  }
}; 













Messages.NavigationMarker = function(response, timestamp)
{
  Messages.BaseMessage.call(this);

  
  
  this.response = response;
  this._url = response.url;
  this.textContent = "------ " + this._url;
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
    icon.title = l10n.getStr("severity." + this._severityNameCompat);

    
    
    let indent = this._groupDepthCompat * COMPAT.GROUP_INDENT;
    let indentNode = this.document.createElementNS(XHTML_NS, "span");
    indentNode.className = "indent";
    indentNode.style.width = indent + "px";

    let body = this._renderBody();
    this._repeatID.textContent += "|" + body.textContent;

    let repeatNode = this._renderRepeatNode();
    let location = this._renderLocation();

    Messages.BaseMessage.prototype.render.call(this);
    if (this._className) {
      this.element.className += " " + this._className;
    }

    this.element.appendChild(timestamp.element);
    this.element.appendChild(indentNode);
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
    body.className = "message-body-wrapper message-body devtools-monospace";

    let bodyInner = this.document.createElementNS(XHTML_NS, "span");
    body.appendChild(bodyInner);

    let anchor, container = bodyInner;
    if (this._link || this._linkCallback) {
      container = anchor = this.document.createElementNS(XHTML_NS, "a");
      anchor.href = this._link || "#";
      anchor.draggable = false;
      this._addLinkCallback(anchor, this._linkCallback);
      bodyInner.appendChild(anchor);
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
    repeatNode.className = "message-repeats";
    repeatNode.textContent = 1;
    repeatNode._uid = this.getRepeatID();
    return repeatNode;
  },

  




  _renderLocation: function()
  {
    if (!this.location) {
      return null;
    }

    let {url, line, column} = this.location;
    if (IGNORED_SOURCE_URLS.indexOf(url) != -1) {
      return null;
    }

    
    
    return this.output.owner.createLocationNode({url: url,
                                                 line: line,
                                                 column: column});
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

    return this._renderValueGrip(piece);
  },

  

















  _renderValueGrip: function(grip, options = {})
  {
    let isPrimitive = VariablesView.isPrimitive({ value: grip });
    let isActorGrip = WebConsoleUtils.isActorGrip(grip);
    let noStringQuotes = !this._quoteStrings;
    if ("noStringQuotes" in options) {
      noStringQuotes = options.noStringQuotes;
    }

    if (isActorGrip) {
      this._repeatID.actors.add(grip.actor);

      if (!isPrimitive) {
        return this._renderObjectActor(grip, options);
      }
      if (grip.type == "longString") {
        let widget = new Widgets.LongString(this, grip, options).render();
        return widget.element;
      }
    }

    let result = this.document.createElementNS(XHTML_NS, "span");
    if (isPrimitive) {
      if (Widgets.URLString.prototype.containsURL.call(Widgets.URLString.prototype, grip)) {
        let widget = new Widgets.URLString(this, grip, options).render();
        return widget.element;
      }

      let className = this.getClassNameForValueGrip(grip);
      if (className) {
        result.className = className;
      }

      result.textContent = VariablesView.getString(grip, {
        noStringQuotes: noStringQuotes,
        concise: options.concise,
      });
    } else {
      result.textContent = grip;
    }

    return result;
  },

  









  shortenValueGrip: function(grip)
  {
    let shortVal = grip;
    if (typeof(grip)=="string") {
      shortVal = grip.replace(/(\r\n|\n|\r)/gm," ");
      if (shortVal.length > MAX_STRING_GRIP_LENGTH) {
        shortVal = shortVal.substring(0,MAX_STRING_GRIP_LENGTH - 1) + ELLIPSIS;
      }
    }

    return shortVal;
  },

  







  getClassNameForValueGrip: function(grip)
  {
    let map = {
      "number": "cm-number",
      "longstring": "console-string",
      "string": "console-string",
      "regexp": "cm-string-2",
      "boolean": "cm-atom",
      "-infinity": "cm-atom",
      "infinity": "cm-atom",
      "null": "cm-atom",
      "undefined": "cm-comment",
      "symbol": "cm-atom"
    };

    let className = map[typeof grip];
    if (!className && grip && grip.type) {
      className = map[grip.type.toLowerCase()];
    }
    if (!className && grip && grip.class) {
      className = map[grip.class.toLowerCase()];
    }

    return className;
  },

  











  _renderObjectActor: function(objectActor, options = {})
  {
    let widget = Widgets.ObjectRenderers.byClass[objectActor.class];

    let { preview } = objectActor;
    if ((!widget || (widget.canRender && !widget.canRender(objectActor)))
        && preview
        && preview.kind) {
      widget = Widgets.ObjectRenderers.byKind[preview.kind];
    }

    if (!widget || (widget.canRender && !widget.canRender(objectActor))) {
      widget = Widgets.JSObject;
    }

    let instance = new widget(this, objectActor, options).render();
    return instance.element;
  },
}); 













Messages.JavaScriptEvalOutput = function(evalResponse, errorMessage)
{
  let severity = "log", msg, quoteStrings = true;

  
  
  this.response = evalResponse;

  if (errorMessage) {
    severity = "error";
    msg = errorMessage;
    quoteStrings = false;
  } else {
    msg = evalResponse.result;
  }

  let options = {
    className: "cm-s-mozilla",
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
    className: "cm-s-mozilla",
    timestamp: packet.timeStamp,
    category: "webdev",
    severity: CONSOLE_API_LEVELS_TO_SEVERITIES[packet.level],
    private: packet.private,
    filterDuplicates: true,
    location: {
      url: packet.filename,
      line: packet.lineNumber,
      column: packet.columnNumber
    },
  };

  switch (packet.level) {
    case "count": {
      let counter = packet.counter, label = counter.label;
      if (!label) {
        label = l10n.getStr("noCounterLabel");
      }
      Messages.Extended.call(this, [label+ ": " + counter.count], options);
      break;
    }
    default:
      Messages.Extended.call(this, packet.arguments, options);
      break;
  }

  this._repeatID.consoleApiLevel = packet.level;
  this._repeatID.styles = packet.styles;
  this._stacktrace = this._repeatID.stacktrace = packet.stacktrace;
  this._styles = packet.styles || [];

  this._onClickCollapsible = this._onClickCollapsible.bind(this);
};

Messages.ConsoleGeneric.prototype = Heritage.extend(Messages.Extended.prototype,
{
  _styles: null,
  _stacktrace: null,

  



  collapsible: false,

  



  get collapsed() {
    return this.collapsible && this.element && !this.element.hasAttribute("open");
  },

  _renderBodyPieceSeparator: function()
  {
    return this.document.createTextNode(" ");
  },

  render: function()
  {
    let msg = this.document.createElementNS(XHTML_NS, "span");
    msg.className = "message-body devtools-monospace";

    this._renderBodyPieces(msg);

    let repeatNode = Messages.Simple.prototype._renderRepeatNode.call(this);
    let location = Messages.Simple.prototype._renderLocation.call(this);
    if (location) {
      location.target = "jsdebugger";
    }

    let stack = null;
    let twisty = null;
    if (this._stacktrace && this._stacktrace.length > 0) {
      stack = new Widgets.Stacktrace(this, this._stacktrace).render().element;

      twisty = this.document.createElementNS(XHTML_NS, "a");
      twisty.className = "theme-twisty";
      twisty.href = "#";
      twisty.title = l10n.getStr("messageToggleDetails");
      twisty.addEventListener("click", this._onClickCollapsible);
    }

    let flex = this.document.createElementNS(XHTML_NS, "span");
    flex.className = "message-flex-body";

    if (twisty) {
      flex.appendChild(twisty);
    }

    flex.appendChild(msg);

    if (repeatNode) {
      flex.appendChild(repeatNode);
    }
    if (location) {
      flex.appendChild(location);
    }

    let result = this.document.createDocumentFragment();
    result.appendChild(flex);

    if (stack) {
      result.appendChild(this.document.createTextNode("\n"));
      result.appendChild(stack);
    }

    this._message = result;
    this._stacktrace = null;

    Messages.Simple.prototype.render.call(this);

    if (stack) {
      this.collapsible = true;
      this.element.setAttribute("collapsible", true);

      let icon = this.element.querySelector(".icon");
      icon.addEventListener("click", this._onClickCollapsible);
    }

    return this;
  },

  _renderBody: function()
  {
    let body = Messages.Simple.prototype._renderBody.apply(this, arguments);
    body.classList.remove("devtools-monospace", "message-body");
    return body;
  },

  _renderBodyPieces: function(container)
  {
    let lastStyle = null;

    for (let i = 0; i < this._messagePieces.length; i++) {
      let separator = i > 0 ? this._renderBodyPieceSeparator() : null;
      if (separator) {
        container.appendChild(separator);
      }

      let piece = this._messagePieces[i];
      let style = this._styles[i];

      
      if (style && typeof style == "string" ) {
        lastStyle = this.cleanupStyle(style);
      }

      container.appendChild(this._renderBodyPiece(piece, lastStyle));
    }

    this._messagePieces = null;
    this._styles = null;
  },

  _renderBodyPiece: function(piece, style)
  {
    let elem = Messages.Extended.prototype._renderBodyPiece.call(this, piece);
    let result = elem;

    if (style) {
      if (elem.nodeType == Ci.nsIDOMNode.ELEMENT_NODE) {
        elem.style = style;
      } else {
        let span = this.document.createElementNS(XHTML_NS, "span");
        span.style = style;
        span.appendChild(elem);
        result = span;
      }
    }

    return result;
  },

  
  
  _renderLocation: function() { },
  _renderRepeatNode: function() { },

  


  toggleDetails: function()
  {
    let twisty = this.element.querySelector(".theme-twisty");
    if (this.element.hasAttribute("open")) {
      this.element.removeAttribute("open");
      twisty.removeAttribute("open");
    } else {
      this.element.setAttribute("open", true);
      twisty.setAttribute("open", true);
    }
  },

  








  _onClickCollapsible: function(ev)
  {
    ev.preventDefault();
    this.toggleDetails();
  },

  












  cleanupStyle: function(style)
  {
    for (let r of RE_CLEANUP_STYLES) {
      style = style.replace(r, "notallowed");
    }

    let dummy = this.output._dummyElement;
    if (!dummy) {
      dummy = this.output._dummyElement =
        this.document.createElementNS(XHTML_NS, "div");
    }
    dummy.style = style;

    let toRemove = [];
    for (let i = 0; i < dummy.style.length; i++) {
      let prop = dummy.style[i];
      if (!RE_ALLOWED_STYLES.test(prop)) {
        toRemove.push(prop);
      }
    }

    for (let prop of toRemove) {
      dummy.style.removeProperty(prop);
    }

    style = dummy.style.cssText;

    dummy.style = "";

    return style;
  },
}); 









Messages.ConsoleTrace = function(packet)
{
  let options = {
    className: "cm-s-mozilla",
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

  this._renderStack = this._renderStack.bind(this);
  Messages.Simple.call(this, this._renderStack, options);

  this._repeatID.consoleApiLevel = packet.level;
  this._stacktrace = this._repeatID.stacktrace = packet.stacktrace;
  this._arguments = packet.arguments;
};

Messages.ConsoleTrace.prototype = Heritage.extend(Messages.Simple.prototype,
{
  





  _stacktrace: null,

  







  _arguments: null,

  init: function()
  {
    let result = Messages.Simple.prototype.init.apply(this, arguments);

    
    if (Array.isArray(this._arguments)) {
      for (let arg of this._arguments) {
        if (WebConsoleUtils.isActorGrip(arg)) {
          this.output._releaseObject(arg.actor);
        }
      }
    }
    this._arguments = null;

    return result;
  },

  render: function()
  {
    Messages.Simple.prototype.render.apply(this, arguments);
    this.element.setAttribute("open", true);
    return this;
  },

  





  _renderStack: function()
  {
    let cmvar = this.document.createElementNS(XHTML_NS, "span");
    cmvar.className = "cm-variable";
    cmvar.textContent = "console";

    let cmprop = this.document.createElementNS(XHTML_NS, "span");
    cmprop.className = "cm-property";
    cmprop.textContent = "trace";

    let title = this.document.createElementNS(XHTML_NS, "span");
    title.className = "message-body devtools-monospace";
    title.appendChild(cmvar);
    title.appendChild(this.document.createTextNode("."));
    title.appendChild(cmprop);
    title.appendChild(this.document.createTextNode("():"));

    let repeatNode = Messages.Simple.prototype._renderRepeatNode.call(this);
    let location = Messages.Simple.prototype._renderLocation.call(this);
    if (location) {
      location.target = "jsdebugger";
    }

    let widget = new Widgets.Stacktrace(this, this._stacktrace).render();

    let body = this.document.createElementNS(XHTML_NS, "span");
    body.className = "message-flex-body";
    body.appendChild(title);
    if (repeatNode) {
      body.appendChild(repeatNode);
    }
    if (location) {
      body.appendChild(location);
    }
    body.appendChild(this.document.createTextNode("\n"));

    let frag = this.document.createDocumentFragment();
    frag.appendChild(body);
    frag.appendChild(widget.element);

    return frag;
  },

  _renderBody: function()
  {
    let body = Messages.Simple.prototype._renderBody.apply(this, arguments);
    body.classList.remove("devtools-monospace", "message-body");
    return body;
  },

  
  
  _renderLocation: function() { },
  _renderRepeatNode: function() { },
}); 









Messages.ConsoleTable = function(packet)
{
  let options = {
    className: "cm-s-mozilla",
    timestamp: packet.timeStamp,
    category: "webdev",
    severity: CONSOLE_API_LEVELS_TO_SEVERITIES[packet.level],
    private: packet.private,
    filterDuplicates: false,
    location: {
      url: packet.filename,
      line: packet.lineNumber,
    },
  };

  this._populateTableData = this._populateTableData.bind(this);
  this._renderTable = this._renderTable.bind(this);
  Messages.Extended.call(this, [this._renderTable], options);

  this._repeatID.consoleApiLevel = packet.level;
  this._arguments = packet.arguments;
};

Messages.ConsoleTable.prototype = Heritage.extend(Messages.Extended.prototype,
{
  






  _arguments: null,

  





  _data: null,

  






  _columns: null,

  






  _populatePromise: null,

  init: function()
  {
    let result = Messages.Extended.prototype.init.apply(this, arguments);
    this._data = [];
    this._columns = {};

    this._populatePromise = this._populateTableData();

    return result;
  },

  








  _setColumns: function(columns)
  {
    if (columns.class == "Array") {
      let items = columns.preview.items;

      for (let item of items) {
        if (typeof item == "string") {
          this._columns[item] = item;
        }
      }
    } else if (typeof columns == "string" && columns) {
      this._columns[columns] = columns;
    }
  },

  







  _populateTableData: function()
  {
    let deferred = promise.defer();

    if (this._arguments.length <= 0) {
      return;
    }

    let data = this._arguments[0];
    if (data.class != "Array" && data.class != "Object" &&
        data.class != "Map" && data.class != "Set") {
      return;
    }

    let hasColumnsArg = false;
    if (this._arguments.length > 1) {
      if (data.class == "Object" || data.class == "Array") {
        this._columns["_index"] = l10n.getStr("table.index");
      } else {
        this._columns["_index"] = l10n.getStr("table.iterationIndex");
      }

      this._setColumns(this._arguments[1]);
      hasColumnsArg = true;
    }

    if (data.class == "Object" || data.class == "Array") {
      
      
      this.client = new ObjectClient(this.output.owner.jsterm.hud.proxy.client,
          data);
      this.client.getPrototypeAndProperties(aResponse => {
        let {ownProperties} = aResponse;
        let rowCount = 0;
        let columnCount = 0;

        for (let index of Object.keys(ownProperties || {})) {
          
          
          if (data.class == "Array" && index == "length") {
            continue;
          }

          if (!hasColumnsArg) {
            this._columns["_index"] = l10n.getStr("table.index");
          }

          if (data.class == "Array") {
            if (index == parseInt(index)) {
              index = parseInt(index);
            }
          }

          let property = ownProperties[index].value;
          let item = { _index: index };

          if (property.class == "Object" || property.class == "Array") {
            let {preview} = property;
            let entries = property.class == "Object" ?
                preview.ownProperties : preview.items;

            for (let key of Object.keys(entries)) {
              let value = property.class == "Object" ?
                  preview.ownProperties[key].value : preview.items[key];

              item[key] = this._renderValueGrip(value, { concise: true });

              if (!hasColumnsArg && !(key in this._columns) &&
                  (++columnCount <= TABLE_COLUMN_MAX_ITEMS)) {
                this._columns[key] = key;
              }
            }
          } else {
            
            item["_value"] = this._renderValueGrip(property, { concise: true });

            if (!hasColumnsArg && !("_value" in this._columns)) {
              this._columns["_value"] = l10n.getStr("table.value");
            }
          }

          this._data.push(item);

          if (++rowCount == TABLE_ROW_MAX_ITEMS) {
            break;
          }
        }

        deferred.resolve();
      });
    } else if (data.class == "Map") {
      let entries = data.preview.entries;

      if (!hasColumnsArg) {
        this._columns["_index"] = l10n.getStr("table.iterationIndex");
        this._columns["_key"] = l10n.getStr("table.key");
        this._columns["_value"] = l10n.getStr("table.value");
      }

      let rowCount = 0;
      for (let [key, value] of entries) {
        let item = {
          _index: rowCount,
          _key: this._renderValueGrip(key, { concise: true }),
          _value: this._renderValueGrip(value, { concise: true })
        };

        this._data.push(item);

        if (++rowCount == TABLE_ROW_MAX_ITEMS) {
          break;
        }
      }

      deferred.resolve();
    } else if (data.class == "Set") {
      let entries = data.preview.items;

      if (!hasColumnsArg) {
        this._columns["_index"] = l10n.getStr("table.iterationIndex");
        this._columns["_value"] = l10n.getStr("table.value");
      }

      let rowCount = 0;
      for (let entry of entries) {
        let item = {
          _index : rowCount,
          _value: this._renderValueGrip(entry, { concise: true })
        };

        this._data.push(item);

        if (++rowCount == TABLE_ROW_MAX_ITEMS) {
          break;
        }
      }

      deferred.resolve();
    }

    return deferred.promise;
  },

  render: function()
  {
    Messages.Extended.prototype.render.apply(this, arguments);
    this.element.setAttribute("open", true);
    return this;
  },

  





  _renderTable: function()
  {
    let cmvar = this.document.createElementNS(XHTML_NS, "span");
    cmvar.className = "cm-variable";
    cmvar.textContent = "console";

    let cmprop = this.document.createElementNS(XHTML_NS, "span");
    cmprop.className = "cm-property";
    cmprop.textContent = "table";

    let title = this.document.createElementNS(XHTML_NS, "span");
    title.className = "message-body devtools-monospace";
    title.appendChild(cmvar);
    title.appendChild(this.document.createTextNode("."));
    title.appendChild(cmprop);
    title.appendChild(this.document.createTextNode("():"));

    let repeatNode = Messages.Simple.prototype._renderRepeatNode.call(this);
    let location = Messages.Simple.prototype._renderLocation.call(this);
    if (location) {
      location.target = "jsdebugger";
    }

    let body = this.document.createElementNS(XHTML_NS, "span");
    body.className = "message-flex-body";
    body.appendChild(title);
    if (repeatNode) {
      body.appendChild(repeatNode);
    }
    if (location) {
      body.appendChild(location);
    }
    body.appendChild(this.document.createTextNode("\n"));

    let result = this.document.createElementNS(XHTML_NS, "div");
    result.appendChild(body);

    if (this._populatePromise) {
      this._populatePromise.then(() => {
        if (this._data.length > 0) {
          let widget = new Widgets.Table(this, this._data, this._columns).render();
          result.appendChild(widget.element);
        }

        result.scrollIntoView();
        this.output.owner.emit("messages-table-rendered");

        
        if (Array.isArray(this._arguments)) {
          for (let arg of this._arguments) {
            if (WebConsoleUtils.isActorGrip(arg)) {
              this.output._releaseObject(arg.actor);
            }
          }
        }
        this._arguments = null;
      });
    }

    return result;
  },

  _renderBody: function()
  {
    let body = Messages.Simple.prototype._renderBody.apply(this, arguments);
    body.classList.remove("devtools-monospace", "message-body");
    return body;
  },

  
  
  _renderLocation: function() { },
  _renderRepeatNode: function() { },
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

  





























  el: function(tagNameIdAndClasses)
  {
    let attrs, text;
    if (typeof arguments[1] == "object") {
      attrs = arguments[1];
      text = arguments[2];
    } else {
      text = arguments[1];
    }

    let tagName = tagNameIdAndClasses.split(/#|\./)[0];

    let elem = this.document.createElementNS(XHTML_NS, tagName);
    for (let name of Object.keys(attrs || {})) {
      elem.setAttribute(name, attrs[name]);
    }
    if (text !== undefined && text !== null) {
      elem.textContent = text;
    }

    let idAndClasses = tagNameIdAndClasses.match(/([#.][^#.]+)/g);
    for (let idOrClass of (idAndClasses || [])) {
      if (idOrClass.charAt(0) == "#") {
        elem.id = idOrClass.substr(1);
      } else {
        elem.classList.add(idOrClass.substr(1));
      }
    }

    return elem;
  },
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

    return this;
  },
}); 












Widgets.URLString = function(message, str)
{
  Widgets.BaseWidget.call(this, message);
  this.str = str;
};

Widgets.URLString.prototype = Heritage.extend(Widgets.BaseWidget.prototype,
{
  



  str: "",

  render: function()
  {
    if (this.element) {
      return this;
    }

    
    
    this.element = this.el("span", {
      class: "console-string"
    });
    this.element.appendChild(this._renderText("\""));

    
    
    let tokens = this.str.split(/\s+/);
    let textStart = 0;
    let tokenStart;
    for (let token of tokens) {
      tokenStart = this.str.indexOf(token, textStart);
      if (this._isURL(token)) {
        this.element.appendChild(this._renderText(this.str.slice(textStart, tokenStart)));
        textStart = tokenStart + token.length;
        this.element.appendChild(this._renderURL(token));
      }
    }

    
    this.element.appendChild(this._renderText(this.str.slice(textStart, this.str.length)));
    this.element.appendChild(this._renderText("\""));

    return this;
  },

  







  containsURL: function(grip)
  {
    if (typeof grip != "string") {
      return false;
    }

    let tokens = grip.split(/\s+/);
    return tokens.some(this._isURL);
  },

  







  _isURL: function(token) {
    try {
      let uri = URI.newURI(token, null, null);
      let url = uri.QueryInterface(Ci.nsIURL);
      return true;
    } catch (e) {
      return false;
    }
  },

  







  _renderURL: function(url)
  {
    let result = this.el("a", {
      class: "url",
      title: url,
      href: url,
      draggable: false
    }, url);
    this.message._addLinkCallback(result);
    return result;
  },

  _renderText: function(text) {
    return this.el("span", text);
  },
}); 














Widgets.JSObject = function(message, objectActor, options = {})
{
  Widgets.BaseWidget.call(this, message);
  this.objectActor = objectActor;
  this.options = options;
  this._onClick = this._onClick.bind(this);
};

Widgets.JSObject.prototype = Heritage.extend(Widgets.BaseWidget.prototype,
{
  



  objectActor: null,

  render: function()
  {
    if (!this.element) {
      this._render();
    }

    return this;
  },

  _render: function()
  {
    let str = VariablesView.getString(this.objectActor, this.options);
    let className = this.message.getClassNameForValueGrip(this.objectActor);
    if (!className && this.objectActor.class == "Object") {
      className = "cm-variable";
    }

    this.element = this._anchor(str, { className: className });
  },

  


  _renderConciseObject: function()
  {
    this.element = this._anchor(this.objectActor.class,
                                { className: "cm-variable" });
  },

  


  _renderObjectPrefix: function()
  {
    let { kind } = this.objectActor.preview;
    this.element = this.el("span.kind-" + kind);
    this._anchor(this.objectActor.class, { className: "cm-variable" });
    this._text(" { ");
  },

  


  _renderObjectSuffix: function()
  {
    this._text(" }");
  },

  















  _renderObjectProperty: function(key, value, container, needsComma, valueIsText = false)
  {
    if (needsComma) {
      this._text(", ");
    }

    container.appendChild(this.el("span.cm-property", key));
    this._text(": ");

    if (valueIsText) {
      this._text(value);
    } else {
      let shortVal = this.message.shortenValueGrip(value);
      let valueElem = this.message._renderValueGrip(shortVal, { concise: true });
      container.appendChild(valueElem);
    }
  },

  








  _renderObjectProperties: function(container, needsComma)
  {
    let { preview } = this.objectActor;
    let { ownProperties, safeGetterValues } = preview;

    let shown = 0;

    let getValue = desc => {
      if (desc.get) {
        return "Getter";
      } else if (desc.set) {
        return "Setter";
      } else {
        return desc.value;
      }
    };

    for (let key of Object.keys(ownProperties || {})) {
      this._renderObjectProperty(key, getValue(ownProperties[key]), container,
                                 shown > 0 || needsComma,
                                 ownProperties[key].get || ownProperties[key].set);
      shown++;
    }

    let ownPropertiesShown = shown;

    for (let key of Object.keys(safeGetterValues || {})) {
      this._renderObjectProperty(key, safeGetterValues[key].getterValue,
                                 container, shown > 0 || needsComma);
      shown++;
    }

    if (typeof preview.ownPropertiesLength == "number" &&
        ownPropertiesShown < preview.ownPropertiesLength) {
      this._text(", ");

      let n = preview.ownPropertiesLength - ownPropertiesShown;
      let str = VariablesView.stringifiers._getNMoreString(n);
      this._anchor(str);
    }
  },

  



















  _anchor: function(text, options = {})
  {
    if (!options.onClick) {
      
      
      options.onClick = options.href ? this._onClickAnchor : this._onClick;
    }

    let anchor = this.el("a", {
      class: options.className,
      draggable: false,
      href: options.href || "#",
    }, text);

    this.message._addLinkCallback(anchor, options.onClick);

    if (options.appendTo) {
      options.appendTo.appendChild(anchor);
    } else if (!("appendTo" in options) && this.element) {
      this.element.appendChild(anchor);
    }

    return anchor;
  },

  



  _onClick: function()
  {
    this.output.openVariablesView({
      label: VariablesView.getString(this.objectActor, { concise: true }),
      objectActor: this.objectActor,
      autofocus: true,
    });
  },

  









  _text: function(str, target = this.element)
  {
    target.appendChild(this.document.createTextNode(str));
  },
}); 

Widgets.ObjectRenderers = {};
Widgets.ObjectRenderers.byKind = {};
Widgets.ObjectRenderers.byClass = {};



























Widgets.ObjectRenderers.add = function(obj)
{
  let extendObj = obj.extends || Widgets.JSObject;

  let constructor = function() {
    if (obj.initialize) {
      obj.initialize.apply(this, arguments);
    } else {
      extendObj.apply(this, arguments);
    }
  };

  let proto = WebConsoleUtils.cloneObject(obj, false, function(key) {
    if (key == "initialize" || key == "canRender" ||
        (key == "render" && extendObj === Widgets.JSObject)) {
      return false;
    }
    return true;
  });

  if (extendObj === Widgets.JSObject) {
    proto._render = obj.render;
  }

  constructor.canRender = obj.canRender;
  constructor.prototype = Heritage.extend(extendObj.prototype, proto);

  if (obj.byClass) {
    Widgets.ObjectRenderers.byClass[obj.byClass] = constructor;
  } else if (obj.byKind) {
    Widgets.ObjectRenderers.byKind[obj.byKind] = constructor;
  } else {
    throw new Error("You are adding an object renderer without any byClass or " +
                    "byKind property.");
  }
};





Widgets.ObjectRenderers.add({
  byClass: "Date",

  render: function()
  {
    let {preview} = this.objectActor;
    this.element = this.el("span.class-" + this.objectActor.class);

    let anchorText = this.objectActor.class;
    let anchorClass = "cm-variable";
    if (preview && "timestamp" in preview && typeof preview.timestamp != "number") {
      anchorText = new Date(preview.timestamp).toString(); 
      anchorClass = "";
    }

    this._anchor(anchorText, { className: anchorClass });

    if (!preview || !("timestamp" in preview) || typeof preview.timestamp != "number") {
      return;
    }

    this._text(" ");

    let elem = this.el("span.cm-string-2", new Date(preview.timestamp).toISOString());
    this.element.appendChild(elem);
  },
});




Widgets.ObjectRenderers.add({
  byClass: "Function",

  render: function()
  {
    let grip = this.objectActor;
    this.element = this.el("span.class-" + this.objectActor.class);

    
    let name = grip.userDisplayName || grip.displayName || grip.name || "";
    name = VariablesView.getString(name, { noStringQuotes: true });

    let str = this.options.concise ? name || "function " : "function " + name;

    if (this.options.concise) {
      this._anchor(name || "function", {
        className: name ? "cm-variable" : "cm-keyword",
      });
      if (!name) {
        this._text(" ");
      }
    } else if (name) {
      this.element.appendChild(this.el("span.cm-keyword", "function"));
      this._text(" ");
      this._anchor(name, { className: "cm-variable" });
    } else {
      this._anchor("function", { className: "cm-keyword" });
      this._text(" ");
    }

    this._text("(");

    
    
    let params = grip.parameterNames || [];
    let shown = 0;
    for (let param of params) {
      if (shown > 0) {
        this._text(", ");
      }
      this.element.appendChild(this.el("span.cm-def", param));
      shown++;
    }

    this._text(")");
  },
}); 




Widgets.ObjectRenderers.add({
  byKind: "ArrayLike",

  render: function()
  {
    let {preview} = this.objectActor;
    let {items} = preview;
    this.element = this.el("span.kind-" + preview.kind);

    this._anchor(this.objectActor.class, { className: "cm-variable" });

    if (!items || this.options.concise) {
      this._text("[");
      this.element.appendChild(this.el("span.cm-number", preview.length));
      this._text("]");
      return this;
    }

    this._text(" [ ");

    let isFirst = true;
    let emptySlots = 0;
    
    let renderSeparator = () => !isFirst && this._text(", ");

    for (let item of items) {
      if (item === null) {
        emptySlots++;
      }
      else {
        renderSeparator();
        isFirst = false;

        if (emptySlots) {
          this._renderEmptySlots(emptySlots);
          emptySlots = 0;
        }

        let shortVal = this.message.shortenValueGrip(item);
        let elem = this.message._renderValueGrip(shortVal, { concise: true });
        this.element.appendChild(elem);
      }
    }

    if (emptySlots) {
      renderSeparator();
      this._renderEmptySlots(emptySlots, false);
    }

    let shown = items.length;
    if (shown < preview.length) {
      this._text(", ");

      let n = preview.length - shown;
      let str = VariablesView.stringifiers._getNMoreString(n);
      this._anchor(str);
    }

    this._text(" ]");
  },

  _renderEmptySlots: function(aNumSlots, aAppendComma=true) {
    let slotLabel = l10n.getStr("emptySlotLabel");
    let slotText = PluralForm.get(aNumSlots, slotLabel);
    this._text("<" + slotText.replace("#1", aNumSlots) + ">");
    if (aAppendComma) {
      this._text(", ");
    }
  },

}); 




Widgets.ObjectRenderers.add({
  byKind: "MapLike",

  render: function()
  {
    let {preview} = this.objectActor;
    let {entries} = preview;

    let container = this.element = this.el("span.kind-" + preview.kind);
    this._anchor(this.objectActor.class, { className: "cm-variable" });

    if (!entries || this.options.concise) {
      if (typeof preview.size == "number") {
        this._text("[");
        container.appendChild(this.el("span.cm-number", preview.size));
        this._text("]");
      }
      return;
    }

    this._text(" { ");

    let shown = 0;
    for (let [key, value] of entries) {
      if (shown > 0) {
        this._text(", ");
      }

      let keyElem = this.message._renderValueGrip(key, {
        concise: true,
        noStringQuotes: true,
      });

      
      if (keyElem.classList && keyElem.classList.contains("console-string")) {
        keyElem.classList.remove("console-string");
        keyElem.classList.add("cm-property");
      }

      container.appendChild(keyElem);

      this._text(": ");

      let valueElem = this.message._renderValueGrip(value, { concise: true });
      container.appendChild(valueElem);

      shown++;
    }

    if (typeof preview.size == "number" && shown < preview.size) {
      this._text(", ");

      let n = preview.size - shown;
      let str = VariablesView.stringifiers._getNMoreString(n);
      this._anchor(str);
    }

    this._text(" }");
  },
}); 




Widgets.ObjectRenderers.add({
  byKind: "ObjectWithURL",

  render: function()
  {
    this.element = this._renderElement(this.objectActor,
                                       this.objectActor.preview.url);
  },

  _renderElement: function(objectActor, url)
  {
    let container = this.el("span.kind-" + objectActor.preview.kind);

    this._anchor(objectActor.class, {
      className: "cm-variable",
      appendTo: container,
    });

    if (!VariablesView.isFalsy({ value: url })) {
      this._text(" \u2192 ", container);
      let shortUrl = WebConsoleUtils.abbreviateSourceURL(url, {
        onlyCropQuery: !this.options.concise
      });
      this._anchor(shortUrl, { href: url, appendTo: container });
    }

    return container;
  },
}); 




Widgets.ObjectRenderers.add({
  byKind: "ObjectWithText",

  render: function()
  {
    let {preview} = this.objectActor;
    this.element = this.el("span.kind-" + preview.kind);

    this._anchor(this.objectActor.class, { className: "cm-variable" });

    if (!this.options.concise) {
      this._text(" ");
      this.element.appendChild(this.el("span.console-string",
                                       VariablesView.getString(preview.text)));
    }
  },
});




Widgets.ObjectRenderers.add({
  byKind: "DOMEvent",

  render: function()
  {
    let {preview} = this.objectActor;

    let container = this.element = this.el("span.kind-" + preview.kind);

    this._anchor(preview.type || this.objectActor.class,
                 { className: "cm-variable" });

    if (this.options.concise) {
      return;
    }

    if (preview.eventKind == "key" && preview.modifiers &&
        preview.modifiers.length) {
      this._text(" ");

      let mods = 0;
      for (let mod of preview.modifiers) {
        if (mods > 0) {
          this._text("-");
        }
        container.appendChild(this.el("span.cm-keyword", mod));
        mods++;
      }
    }

    this._text(" { ");

    let shown = 0;
    if (preview.target) {
      container.appendChild(this.el("span.cm-property", "target"));
      this._text(": ");
      let target = this.message._renderValueGrip(preview.target, { concise: true });
      container.appendChild(target);
      shown++;
    }

    for (let key of Object.keys(preview.properties || {})) {
      if (shown > 0) {
        this._text(", ");
      }

      container.appendChild(this.el("span.cm-property", key));
      this._text(": ");

      let value = preview.properties[key];
      let valueElem = this.message._renderValueGrip(value, { concise: true });
      container.appendChild(valueElem);

      shown++;
    }

    this._text(" }");
  },
}); 




Widgets.ObjectRenderers.add({
  byKind: "DOMNode",

  canRender: function(objectActor) {
    let {preview} = objectActor;
    if (!preview) {
      return false;
    }

    switch (preview.nodeType) {
      case Ci.nsIDOMNode.DOCUMENT_NODE:
      case Ci.nsIDOMNode.ATTRIBUTE_NODE:
      case Ci.nsIDOMNode.TEXT_NODE:
      case Ci.nsIDOMNode.COMMENT_NODE:
      case Ci.nsIDOMNode.DOCUMENT_FRAGMENT_NODE:
      case Ci.nsIDOMNode.ELEMENT_NODE:
        return true;
      default:
        return false;
    }
  },

  render: function()
  {
    switch (this.objectActor.preview.nodeType) {
      case Ci.nsIDOMNode.DOCUMENT_NODE:
        this._renderDocumentNode();
        break;
      case Ci.nsIDOMNode.ATTRIBUTE_NODE: {
        let {preview} = this.objectActor;
        this.element = this.el("span.attributeNode.kind-" + preview.kind);
        let attr = this._renderAttributeNode(preview.nodeName, preview.value, true);
        this.element.appendChild(attr);
        break;
      }
      case Ci.nsIDOMNode.TEXT_NODE:
        this._renderTextNode();
        break;
      case Ci.nsIDOMNode.COMMENT_NODE:
        this._renderCommentNode();
        break;
      case Ci.nsIDOMNode.DOCUMENT_FRAGMENT_NODE:
        this._renderDocumentFragmentNode();
        break;
      case Ci.nsIDOMNode.ELEMENT_NODE:
        this._renderElementNode();
        break;
      default:
        throw new Error("Unsupported nodeType: " + preview.nodeType);
    }
  },

  _renderDocumentNode: function()
  {
    let fn = Widgets.ObjectRenderers.byKind.ObjectWithURL.prototype._renderElement;
    this.element = fn.call(this, this.objectActor,
                           this.objectActor.preview.location);
    this.element.classList.add("documentNode");
  },

  _renderAttributeNode: function(nodeName, nodeValue, addLink)
  {
    let value = VariablesView.getString(nodeValue, { noStringQuotes: true });

    let fragment = this.document.createDocumentFragment();
    if (addLink) {
      this._anchor(nodeName, { className: "cm-attribute", appendTo: fragment });
    } else {
      fragment.appendChild(this.el("span.cm-attribute", nodeName));
    }

    this._text("=", fragment);
    fragment.appendChild(this.el("span.console-string",
                                 '"' + escapeHTML(value) + '"'));

    return fragment;
  },

  _renderTextNode: function()
  {
    let {preview} = this.objectActor;
    this.element = this.el("span.textNode.kind-" + preview.kind);

    this._anchor(preview.nodeName, { className: "cm-variable" });
    this._text(" ");

    let text = VariablesView.getString(preview.textContent);
    this.element.appendChild(this.el("span.console-string", text));
  },

  _renderCommentNode: function()
  {
    let {preview} = this.objectActor;
    let comment = "<!-- " + VariablesView.getString(preview.textContent, {
      noStringQuotes: true,
    }) + " -->";

    this.element = this._anchor(comment, {
      className: "kind-" + preview.kind + " commentNode cm-comment",
    });
  },

  _renderDocumentFragmentNode: function()
  {
    let {preview} = this.objectActor;
    let {childNodes} = preview;
    let container = this.element = this.el("span.documentFragmentNode.kind-" +
                                           preview.kind);

    this._anchor(this.objectActor.class, { className: "cm-variable" });

    if (!childNodes || this.options.concise) {
      this._text("[");
      container.appendChild(this.el("span.cm-number", preview.childNodesLength));
      this._text("]");
      return;
    }

    this._text(" [ ");

    let shown = 0;
    for (let item of childNodes) {
      if (shown > 0) {
        this._text(", ");
      }

      let elem = this.message._renderValueGrip(item, { concise: true });
      container.appendChild(elem);
      shown++;
    }

    if (shown < preview.childNodesLength) {
      this._text(", ");

      let n = preview.childNodesLength - shown;
      let str = VariablesView.stringifiers._getNMoreString(n);
      this._anchor(str);
    }

    this._text(" ]");
  },

  _renderElementNode: function()
  {
    let doc = this.document;
    let {attributes, nodeName} = this.objectActor.preview;

    this.element = this.el("span." + "kind-" + this.objectActor.preview.kind + ".elementNode");

    let openTag = this.el("span.cm-tag");
    openTag.textContent = "<";
    this.element.appendChild(openTag);

    let tagName = this._anchor(nodeName, {
      className: "cm-tag",
      appendTo: openTag
    });

    if (this.options.concise) {
      if (attributes.id) {
        tagName.appendChild(this.el("span.cm-attribute", "#" + attributes.id));
      }
      if (attributes.class) {
        tagName.appendChild(this.el("span.cm-attribute", "." + attributes.class.split(/\s+/g).join(".")));
      }
    } else {
      for (let name of Object.keys(attributes)) {
        let attr = this._renderAttributeNode(" " + name, attributes[name]);
        this.element.appendChild(attr);
      }
    }

    let closeTag = this.el("span.cm-tag");
    closeTag.textContent = ">";
    this.element.appendChild(closeTag);

    
    
    this.message.widgets.add(this);

    this.linkToInspector().then(null, Cu.reportError);
  },

  









  linkToInspector: Task.async(function*()
  {
    if (this._linkedToInspector) {
      return;
    }

    
    if (this.objectActor.preview.nodeType !== Ci.nsIDOMNode.ELEMENT_NODE) {
      throw new Error("The object cannot be linked to the inspector as it " +
        "isn't an element node");
    }

    
    let target = this.message.output.toolboxTarget;
    this.toolbox = gDevTools.getToolbox(target);
    if (!this.toolbox) {
      throw new Error("The object cannot be linked to the inspector without a " +
        "toolbox");
    }

    
    yield this.toolbox.initInspector();
    this._nodeFront = yield this.toolbox.walker.getNodeActorFromObjectActor(this.objectActor.actor);
    if (!this._nodeFront) {
      throw new Error("The object cannot be linked to the inspector, the " +
        "corresponding nodeFront could not be found");
    }

    
    if (!this.document) {
      throw new Error("The object cannot be linked to the inspector, the " +
        "message was got cleared away");
    }

    this.highlightDomNode = this.highlightDomNode.bind(this);
    this.element.addEventListener("mouseover", this.highlightDomNode, false);
    this.unhighlightDomNode = this.unhighlightDomNode.bind(this);
    this.element.addEventListener("mouseout", this.unhighlightDomNode, false);

    this._openInspectorNode = this._anchor("", {
      className: "open-inspector",
      onClick: this.openNodeInInspector.bind(this)
    });
    this._openInspectorNode.title = l10n.getStr("openNodeInInspector");

    this._linkedToInspector = true;
  }),

  




  highlightDomNode: Task.async(function*()
  {
    yield this.linkToInspector();
    let isAttached = yield this.toolbox.walker.isInDOMTree(this._nodeFront);
    if (isAttached) {
      yield this.toolbox.highlighterUtils.highlightNodeFront(this._nodeFront);
    } else {
      throw null;
    }
  }),

  




  unhighlightDomNode: function()
  {
    return this.linkToInspector().then(() => {
      return this.toolbox.highlighterUtils.unhighlight();
    }).then(null, Cu.reportError);
  },

  






  openNodeInInspector: Task.async(function*()
  {
    yield this.linkToInspector();
    yield this.toolbox.selectTool("inspector");

    let isAttached = yield this.toolbox.walker.isInDOMTree(this._nodeFront);
    if (isAttached) {
      let onReady = promise.defer();
      this.toolbox.inspector.once("inspector-updated", onReady.resolve);
      yield this.toolbox.selection.setNodeFront(this._nodeFront, "console");
      yield onReady.promise;
    } else {
      throw null;
    }
  }),

  destroy: function()
  {
    if (this.toolbox && this._nodeFront) {
      this.element.removeEventListener("mouseover", this.highlightDomNode, false);
      this.element.removeEventListener("mouseout", this.unhighlightDomNode, false);
      this._openInspectorNode.removeEventListener("mousedown", this.openNodeInInspector, true);
      this.toolbox = null;
      this._nodeFront = null;
    }
  },
}); 




Widgets.ObjectRenderers.add({
  byClass: "Promise",

  render: function()
  {
    let { ownProperties, safeGetterValues } = this.objectActor.preview || {};
    if ((!ownProperties && !safeGetterValues) || this.options.concise) {
      this._renderConciseObject();
      return;
    }

    this._renderObjectPrefix();
    let container = this.element;
    let addedPromiseInternalProps = false;

    if (this.objectActor.promiseState) {
      const { state, value, reason } = this.objectActor.promiseState;

      this._renderObjectProperty("<state>", state, container, false);
      addedPromiseInternalProps = true;

      if (state == "fulfilled") {
        this._renderObjectProperty("<value>", value, container, true);
      } else if (state == "rejected") {
        this._renderObjectProperty("<reason>", reason, container, true);
      }
    }

    this._renderObjectProperties(container, addedPromiseInternalProps);
    this._renderObjectSuffix();
  }
}); 




Widgets.ObjectRenderers.add({
  byKind: "Object",

  render: function()
  {
    let { ownProperties, safeGetterValues } = this.objectActor.preview || {};
    if ((!ownProperties && !safeGetterValues) || this.options.concise) {
      this._renderConciseObject();
      return;
    }

    this._renderObjectPrefix();
    this._renderObjectProperties(this.element, false);
    this._renderObjectSuffix();
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
    result.className = "longString console-string";
    this._renderString(this.longStringActor.initial);
    result.appendChild(this._renderEllipsis());

    return this;
  },

  





  _renderString: function(str)
  {
    this.element.textContent = VariablesView.getString(str, {
      noStringQuotes: !this.message._quoteStrings,
      noEllipsis: true,
    });
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

    this.output.owner.emit("new-messages", new Set([{
      update: true,
      node: this.message.element,
      response: response,
    }]));

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













Widgets.Stacktrace = function(message, stacktrace)
{
  Widgets.BaseWidget.call(this, message);
  this.stacktrace = stacktrace;
};

Widgets.Stacktrace.prototype = Heritage.extend(Widgets.BaseWidget.prototype,
{
  



  stacktrace: null,

  render: function()
  {
    if (this.element) {
      return this;
    }

    let result = this.element = this.document.createElementNS(XHTML_NS, "ul");
    result.className = "stacktrace devtools-monospace";

    for (let frame of this.stacktrace) {
      result.appendChild(this._renderFrame(frame));
    }

    return this;
  },

  








  _renderFrame: function(frame)
  {
    let fn = this.document.createElementNS(XHTML_NS, "span");
    fn.className = "function";
    if (frame.functionName) {
      let span = this.document.createElementNS(XHTML_NS, "span");
      span.className = "cm-variable";
      span.textContent = frame.functionName;
      fn.appendChild(span);
      fn.appendChild(this.document.createTextNode("()"));
    } else {
      fn.classList.add("cm-comment");
      fn.textContent = l10n.getStr("stacktrace.anonymousFunction");
    }

    let location = this.output.owner.createLocationNode({url: frame.filename,
                                                        line: frame.lineNumber},
                                                        "jsdebugger");

    
    
    
    location.classList.remove("devtools-monospace");

    let elem = this.document.createElementNS(XHTML_NS, "li");
    elem.appendChild(fn);
    elem.appendChild(location);
    elem.appendChild(this.document.createTextNode("\n"));

    return elem;
  },
}); 















Widgets.Table = function(message, data, columns)
{
  Widgets.BaseWidget.call(this, message);
  this.data = data;
  this.columns = columns;
};

Widgets.Table.prototype = Heritage.extend(Widgets.BaseWidget.prototype,
{
  



  data: null,

  




  columns: null,

  render: function() {
    if (this.element) {
      return this;
    }

    let result = this.element = this.document.createElementNS(XHTML_NS, "div");
    result.className = "consoletable devtools-monospace";

    this.table = new TableWidget(result, {
      initialColumns: this.columns,
      uniqueId: "_index",
      firstColumn: "_index"
    });

    for (let row of this.data) {
      this.table.push(row);
    }

    return this;
  }
}); 

function gSequenceId()
{
  return gSequenceId.n++;
}
gSequenceId.n = 0;

exports.ConsoleOutput = ConsoleOutput;
exports.Messages = Messages;
exports.Widgets = Widgets;
