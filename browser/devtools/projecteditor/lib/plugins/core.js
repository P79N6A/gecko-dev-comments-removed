







const { Class } = require("sdk/core/heritage");

var Plugin = Class({
  initialize: function(host) {
    this.host = host;
    this.init(host);
  },

  destroy: function(host) { },

  init: function(host) {},

  showForCategories: function(elt, categories) {
    this._showFor = this._showFor || [];
    let set = new Set(categories);
    this._showFor.push({
      elt: elt,
      categories: new Set(categories)
    });
    if (this.host.currentEditor) {
      this.onEditorActivated(this.host.currentEditor);
    } else {
      elt.classList.add("plugin-hidden");
    }
  },

  priv: function(item) {
    if (!this._privData) {
      this._privData = new WeakMap();
    }
    if (!this._privData.has(item)) {
       this._privData.set(item, {});
    }
    return this._privData.get(item);
  },
  onTreeSelected: function(resource) {},


  
  onEditorCreated: function(editor) {},
  onEditorDestroyed: function(editor) {},

  onEditorActivated: function(editor) {
    if (this._showFor) {
      let category = editor.category;
      for (let item of this._showFor) {
        if (item.categories.has(category)) {
          item.elt.classList.remove("plugin-hidden");
        } else {
          item.elt.classList.add("plugin-hidden");
        }
      }
    }
  },
  onEditorDeactivated: function(editor) {
    if (this._showFor) {
      for (let item of this._showFor) {
        item.elt.classList.add("plugin-hidden");
      }
    }
  },

  onEditorLoad: function(editor) {},
  onEditorSave: function(editor) {},
  onEditorChange: function(editor) {},
  onEditorCursorActivity: function(editor) {},
});
exports.Plugin = Plugin;

function registerPlugin(constr) {
  exports.registeredPlugins.push(constr);
}
exports.registerPlugin = registerPlugin;

exports.registeredPlugins = [];
