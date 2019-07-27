





const { Cu } = require("chrome");
const { Class } = require("sdk/core/heritage");
const { emit } = require("sdk/event/core");
const { EventTarget } = require("sdk/event/target");
const { merge } = require("sdk/util/object");
const promise = require("projecteditor/helpers/promise");
const { InplaceEditor } = require("devtools/shared/inplace-editor");
const { on, forget } = require("projecteditor/helpers/event");
const { OS } = Cu.import("resource://gre/modules/osfile.jsm", {});

const HTML_NS = "http://www.w3.org/1999/xhtml";





var ResourceContainer = Class({
  



  initialize: function(tree, resource) {
    this.tree = tree;
    this.resource = resource;
    this.elt = null;
    this.expander = null;
    this.children = null;

    let doc = tree.doc;

    this.elt = doc.createElementNS(HTML_NS, "li");
    this.elt.classList.add("child");

    this.line = doc.createElementNS(HTML_NS, "div");
    this.line.classList.add("child");
    this.line.classList.add("entry");
    this.line.setAttribute("theme", "dark");
    this.line.setAttribute("tabindex", "0");

    this.elt.appendChild(this.line);

    this.highlighter = doc.createElementNS(HTML_NS, "span");
    this.highlighter.classList.add("highlighter");
    this.line.appendChild(this.highlighter);

    this.expander = doc.createElementNS(HTML_NS, "span");
    this.expander.className = "arrow expander";
    this.expander.setAttribute("open", "");
    this.line.appendChild(this.expander);

    this.icon = doc.createElementNS(HTML_NS, "span");
    this.line.appendChild(this.icon);

    this.label = doc.createElementNS(HTML_NS, "span");
    this.label.className = "file-label";
    this.line.appendChild(this.label);

    this.line.addEventListener("contextmenu", (ev) => {
      this.select();
      this.openContextMenu(ev);
    }, false);

    this.children = doc.createElementNS(HTML_NS, "ul");
    this.children.classList.add("children");

    this.elt.appendChild(this.children);

    this.line.addEventListener("click", (evt) => {
      if (!this.selected) {
        this.select();
        this.expanded = true;
        evt.stopPropagation();
      }
    }, false);
    this.expander.addEventListener("click", (evt) => {
      this.expanded = !this.expanded;
      this.select();
      evt.stopPropagation();
    }, true);

    if (!this.resource.isRoot) {
      this.expanded = false;
    }
    this.update();
  },

  destroy: function() {
    this.elt.remove();
    this.expander.remove();
    this.icon.remove();
    this.highlighter.remove();
    this.children.remove();
    this.label.remove();
    this.elt = this.expander = this.icon = this.highlighter = this.children = this.label = null;
  },

  






  openContextMenu: function(ev) {
    ev.preventDefault();
    let popup = this.tree.options.contextMenuPopup;
    popup.openPopupAtScreen(ev.screenX, ev.screenY, true);
  },

  


  update: function() {
    let visible = this.tree.options.resourceVisible ?
      this.tree.options.resourceVisible(this.resource) :
      true;

    this.elt.hidden = !visible;

    this.tree.options.resourceFormatter(this.resource, this.label);

    this.icon.className = "file-icon";

    let contentCategory = this.resource.contentCategory;
    let baseName = this.resource.basename || "";

    if (!this.resource.parent) {
      this.icon.classList.add("icon-none");
    } else if (this.resource.isDir) {
      this.icon.classList.add("icon-folder");
    } else if (baseName.endsWith(".manifest") || baseName.endsWith(".webapp")) {
      this.icon.classList.add("icon-manifest");
    } else if (contentCategory === "js") {
      this.icon.classList.add("icon-js");
    } else if (contentCategory === "css") {
      this.icon.classList.add("icon-css");
    } else if (contentCategory === "html") {
      this.icon.classList.add("icon-html");
    } else if (contentCategory === "image") {
      this.icon.classList.add("icon-img");
    } else {
      this.icon.classList.add("icon-file");
    }

    this.expander.style.visibility = this.resource.hasChildren ? "visible" : "hidden";

  },

  


  select: function() {
    this.tree.selectContainer(this);
  },

  



  get selected() {
    return this.line.classList.contains("selected");
  },

  


  set selected(v) {
    if (v) {
      this.line.classList.add("selected");
    } else {
      this.line.classList.remove("selected");
    }
  },

  



  get expanded() {
    return !this.elt.classList.contains("tree-collapsed");
  },

  


  set expanded(v) {
    if (v) {
      this.elt.classList.remove("tree-collapsed");
      this.expander.setAttribute("open", "");
    } else {
      this.expander.removeAttribute("open");
      this.elt.classList.add("tree-collapsed");
    }
  }
});






var TreeView = Class({
  extends: EventTarget,

  








  initialize: function(doc, options) {
    this.doc = doc;
    this.options = merge({
      resourceFormatter: function(resource, elt) {
        elt.textContent = resource.toString();
      }
    }, options);
    this.models = new Set();
    this.roots = new Set();
    this._containers = new Map();
    this.elt = this.doc.createElementNS(HTML_NS, "div");
    this.elt.tree = this;
    this.elt.className = "sources-tree";
    this.elt.setAttribute("with-arrows", "true");
    this.elt.setAttribute("theme", "dark");
    this.elt.setAttribute("flex", "1");

    this.children = this.doc.createElementNS(HTML_NS, "ul");
    this.elt.appendChild(this.children);

    this.resourceChildrenChanged = this.resourceChildrenChanged.bind(this);
    this.removeResource = this.removeResource.bind(this);
    this.updateResource = this.updateResource.bind(this);
  },

  destroy: function() {
    this._destroyed = true;
    this.elt.remove();
  },

  













  promptNew: function(initial, parent, sibling=null) {
    let deferred = promise.defer();

    let parentContainer = this._containers.get(parent);
    let item = this.doc.createElement("li");
    item.className = "child";
    let placeholder = this.doc.createElementNS(HTML_NS, "div");
    placeholder.className = "child";
    item.appendChild(placeholder);

    let children = parentContainer.children;
    sibling = sibling ? this._containers.get(sibling).elt : null;
    parentContainer.children.insertBefore(item, sibling ? sibling.nextSibling : children.firstChild);

    new InplaceEditor({
      element: placeholder,
      initial: initial,
      start: editor => {
        editor.input.select();
      },
      done: function(val, commit) {
        if (commit) {
          deferred.resolve(val);
        } else {
          deferred.reject(val);
        }
        parentContainer.line.focus();
      },
      destroy: () => {
        item.parentNode.removeChild(item);
      },
    });

    return deferred.promise;
  },

  




  addModel: function(model) {
    if (this.models.has(model)) {
      
      return;
    }
    this.models.add(model);
    let placeholder = this.doc.createElementNS(HTML_NS, "li");
    placeholder.style.display = "none";
    this.children.appendChild(placeholder);
    this.roots.add(model.root);
    model.root.refresh().then(root => {
      if (this._destroyed || !this.models.has(model)) {
        
        
        return;
      }
      let container = this.importResource(root);
      container.line.classList.add("entry-group-title");
      container.line.setAttribute("theme", "dark");
      this.selectContainer(container);

      this.children.insertBefore(container.elt, placeholder);
      this.children.removeChild(placeholder);
    });
  },

  




  removeModel: function(model) {
    this.models.delete(model);
    this.removeResource(model.root);
  },


  





  getViewContainer: function(resource) {
    return this._containers.get(resource);
  },

  




  selectContainer: function(container) {
    if (this.selectedContainer === container) {
      return;
    }
    if (this.selectedContainer) {
      this.selectedContainer.selected = false;
    }
    this.selectedContainer = container;
    container.selected = true;
    emit(this, "selection", container.resource);
  },

  




  selectResource: function(resource) {
    this.selectContainer(this._containers.get(resource));
  },

  




  getSelectedResource: function() {
    return this.selectedContainer.resource;
  },

  





  importResource: function(resource) {
    if (!resource) {
      return null;
    }

    if (this._containers.has(resource)) {
      return this._containers.get(resource);
    }
    var container = ResourceContainer(this, resource);
    this._containers.set(resource, container);
    this._updateChildren(container);

    on(this, resource, "children-changed", this.resourceChildrenChanged);
    on(this, resource, "label-change", this.updateResource);
    on(this, resource, "deleted", this.removeResource);

    return container;
  },

  




  removeResource: function(resource) {
    let toRemove = resource.allDescendants();
    toRemove.add(resource);
    for (let remove of toRemove) {
      this._removeResource(remove);
    }
  },

  




  _removeResource: function(resource) {
    forget(this, resource);
    if (this._containers.get(resource)) {
      this._containers.get(resource).destroy();
      this._containers.delete(resource);
    }
    emit(this, "resource-removed", resource);
  },

  





  resourceChildrenChanged: function(resource) {
    this.updateResource(resource);
    this._updateChildren(this._containers.get(resource));
  },

  






  updateResource: function(resource) {
    let container = this._containers.get(resource);
    container.update();
  },

  





  _updateChildren: function(container) {
    let resource = container.resource;
    let fragment = this.doc.createDocumentFragment();
    if (resource.children) {
      for (let child of resource.childrenSorted) {
        let childContainer = this.importResource(child);
        fragment.appendChild(childContainer.elt);
      }
    }

    while (container.children.firstChild) {
      container.children.firstChild.remove();
    }

    container.children.appendChild(fragment);
  },
});






var ProjectTreeView = Class({
  extends: TreeView,

  





  initialize: function(document, options) {
    TreeView.prototype.initialize.apply(this, arguments);
  },

  destroy: function() {
    this.forgetProject();
    TreeView.prototype.destroy.apply(this, arguments);
  },

  


  forgetProject: function() {
    if (this.project) {
      forget(this, this.project);
      for (let store of this.project.allStores()) {
        this.removeModel(store);
      }
    }
  },

  





  setProject: function(project) {
    this.forgetProject();
    this.project = project;
    if (this.project) {
      on(this, project, "store-added", this.addModel.bind(this));
      on(this, project, "store-removed", this.removeModel.bind(this));
      on(this, project, "project-saved", this.refresh.bind(this));
      this.refresh();
    }
  },

  


  refresh: function() {
    for (let store of this.project.allStores()) {
      this.addModel(store);
    }
  }
});

exports.ProjectTreeView = ProjectTreeView;
