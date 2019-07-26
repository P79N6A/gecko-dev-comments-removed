





const { Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const promise = require("projecteditor/helpers/promise");
const { ItchEditor } = require("projecteditor/editors");

var ImageEditor = Class({
  extends: ItchEditor,

  initialize: function() {
    ItchEditor.prototype.initialize.apply(this, arguments);
    this.label = "image";
    this.appended = promise.resolve();
  },

  load: function(resource) {
    this.elt.innerHTML = "";
    let image = this.image = this.doc.createElement("image");
    image.className = "editor-image";
    image.setAttribute("src", resource.uri);

    let box1 = this.doc.createElement("box");
    box1.appendChild(image);

    let box2 = this.doc.createElement("box");
    box2.setAttribute("flex", 1);

    this.elt.appendChild(box1);
    this.elt.appendChild(box2);

    this.appended.then(() => {
      this.emit("load");
    });
  },

  destroy: function() {
    if (this.image) {
      this.image.remove();
      this.image = null;
    }
  }

});

exports.ImageEditor = ImageEditor;
