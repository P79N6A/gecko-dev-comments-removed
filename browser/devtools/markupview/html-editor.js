



 "use strict";

const {Cu} = require("chrome");
const Editor = require("devtools/sourceeditor/editor");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/devtools/event-emitter.js");

exports.HTMLEditor = HTMLEditor;

function ctrl(k) {
  return (Services.appinfo.OS == "Darwin" ? "Cmd-" : "Ctrl-") + k;
}
function stopPropagation(e) {
  e.stopPropagation();
}















function HTMLEditor(htmlDocument)
{
  this.doc = htmlDocument;
  this.container = this.doc.createElement("div");
  this.container.className = "html-editor theme-body";
  this.container.style.display = "none";
  this.editorInner = this.doc.createElement("div");
  this.editorInner.className = "html-editor-inner";
  this.container.appendChild(this.editorInner);

  this.doc.body.appendChild(this.container);
  this.hide = this.hide.bind(this);
  this.refresh = this.refresh.bind(this);

  EventEmitter.decorate(this);

  this.doc.defaultView.addEventListener("resize",
    this.refresh, true);

  let config = {
    mode: Editor.modes.html,
    lineWrapping: true,
    styleActiveLine: false,
    extraKeys: {},
    theme: "mozilla markup-view"
  };

  config.extraKeys[ctrl("Enter")] = this.hide;
  config.extraKeys["F2"] = this.hide;
  config.extraKeys["Esc"] = this.hide.bind(this, false);

  this.container.addEventListener("click", this.hide, false);
  this.editorInner.addEventListener("click", stopPropagation, false);
  this.editor = new Editor(config);

  let iframe = this.editorInner.ownerDocument.createElement("iframe");
  this.editor.appendTo(this.editorInner, iframe).then(() => {
    this.hide(false);
  }).then(null, (err) => console.log(err.message));
}

HTMLEditor.prototype = {

  



  refresh: function() {
    let element = this._attachedElement;

    if (element) {
      this.container.style.top = element.offsetTop + "px";
      this.container.style.left = element.offsetLeft + "px";
      this.container.style.width = element.offsetWidth + "px";
      this.container.style.height = element.parentNode.offsetHeight + "px";
      this.editor.refresh();
    }
  },

  






  _attach: function(element)
  {
    this._detach();
    this._attachedElement = element;
    element.classList.add("html-editor-container");
    this.refresh();
  },

  


  _detach: function()
  {
    if (this._attachedElement) {
      this._attachedElement.classList.remove("html-editor-container");
      this._attachedElement = undefined;
    }
  },

  










  show: function(element, text)
  {
    if (this._visible) {
      return;
    }

    this._originalValue = text;
    this.editor.setText(text);
    this._attach(element);
    this.container.style.display = "flex";
    this._visible = true;

    this.editor.refresh();
    this.editor.focus();

    this.emit("popupshown");
  },

  






  hide: function(shouldCommit)
  {
    if (!this._visible) {
      return;
    }

    this.container.style.display = "none";
    this._detach();

    let newValue = this.editor.getText();
    let valueHasChanged = this._originalValue !== newValue;
    let preventCommit = shouldCommit === false || !valueHasChanged;
    this._originalValue = undefined;
    this._visible = undefined;
    this.emit("popuphidden", !preventCommit, newValue);
  },

  


  destroy: function()
  {
    this.doc.defaultView.removeEventListener("resize",
      this.refresh, true);
    this.container.removeEventListener("click", this.hide, false);
    this.editorInner.removeEventListener("click", stopPropagation, false);

    this.hide(false);
    this.container.remove();
    this.editor.destroy();
  }
};
