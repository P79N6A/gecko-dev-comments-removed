




"use strict";

const Heritage = require("sdk/core/heritage");
const XHTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";




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
};















function ConsoleOutput(owner)
{
  this.owner = owner;
  this._onFlushOutputMessage = this._onFlushOutputMessage.bind(this);
}

ConsoleOutput.prototype = {
  



  get document() this.owner.document,

  



  get window() this.owner.window,

  






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

  


  destroy: function()
  {
    this.owner = null;
  },
}; 





let Messages = {};







Messages.BaseMessage = function()
{
  this.widgets = new Set();
};

Messages.BaseMessage.prototype = {
  





  output: null,

  





  parent: null,

  





  element: null,

  



  get visible() {
    return this.element && this.element.parentNode;
  },

  



  textContent: "",

  



  widgets: null,

  
  
  _elementClassCompat: "",
  _categoryCompat: null,
  _severityCompat: null,

  








  init: function(output, parent=null)
  {
    this.output = output;
    this.parent = parent;
    return this;
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
    let container = doc.createElementNS(XUL_NS, "richlistitem");
    container.setAttribute("id", "console-msg-" + gSequenceId());
    container.setAttribute("class", "hud-msg-node " + this._elementClassCompat);
    container.category = this._categoryCompat;
    container.severity = this._severityCompat;
    container.clipboardText = this.textContent;
    container.timestamp = this.timestamp;
    container._messageObject = this;

    let body = doc.createElementNS(XUL_NS, "description");
    body.flex = 1;
    body.classList.add("webconsole-msg-body");
    container.appendChild(body);

    return container;
  },
}; 













Messages.NavigationMarker = function(url, timestamp)
{
  Messages.BaseMessage.apply(this, arguments);
  this._url = url;
  this.textContent = "------ " + url;
  this.timestamp = timestamp;
};

Messages.NavigationMarker.prototype = Heritage.extend(Messages.BaseMessage.prototype,
{
  





  timestamp: 0,

  
  _elementClassCompat: "webconsole-msg-network webconsole-msg-info hud-networkinfo",
  _categoryCompat: COMPAT.CATEGORIES.NETWORK,
  _severityCompat: COMPAT.SEVERITIES.LOG,

  



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
    let urlnode = doc.createElementNS(XHTML_NS, "span");
    urlnode.className = "url";
    urlnode.textContent = url;

    
    let render = Messages.BaseMessage.prototype.render.bind(this);
    render().element.firstChild.appendChild(urlnode);
    this.element.classList.add("navigation-marker");
    this.element.url = this._url;

    return this;
  },
}); 


function gSequenceId()
{
  return gSequenceId.n++;
}
gSequenceId.n = 0;

exports.ConsoleOutput = ConsoleOutput;
exports.Messages = Messages;
