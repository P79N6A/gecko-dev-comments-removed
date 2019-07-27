




"use strict";

const {Cu} = require("chrome");
const STORAGE_STRINGS = "chrome://browser/locale/devtools/storage.properties";

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyGetter(this, "TreeWidget",
  () => require("devtools/shared/widgets/TreeWidget").TreeWidget);
XPCOMUtils.defineLazyGetter(this, "TableWidget",
  () => require("devtools/shared/widgets/TableWidget").TableWidget);
XPCOMUtils.defineLazyModuleGetter(this, "EventEmitter",
  "resource://gre/modules/devtools/event-emitter.js");
XPCOMUtils.defineLazyModuleGetter(this, "ViewHelpers",
  "resource:///modules/devtools/ViewHelpers.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "VariablesView",
  "resource:///modules/devtools/VariablesView.jsm");

let Telemetry = require("devtools/shared/telemetry");




let L10N = new ViewHelpers.L10N(STORAGE_STRINGS);

const GENERIC_VARIABLES_VIEW_SETTINGS = {
  lazyEmpty: true,
  lazyEmptyDelay: 10, 
  searchEnabled: true,
  searchPlaceholder: L10N.getStr("storage.search.placeholder"),
  preventDescriptorModifiers: true
};


const HIDDEN_COLUMNS = [
  "creationTime",
  "isDomain",
  "isSecure"
];











this.StorageUI = function StorageUI(front, target, panelWin) {
  EventEmitter.decorate(this);

  this._target = target;
  this._window = panelWin;
  this._panelDoc = panelWin.document;
  this.front = front;

  let treeNode = this._panelDoc.getElementById("storage-tree");
  this.tree = new TreeWidget(treeNode, {defaultType: "dir"});
  this.onHostSelect = this.onHostSelect.bind(this);
  this.tree.on("select", this.onHostSelect);

  let tableNode = this._panelDoc.getElementById("storage-table");
  this.table = new TableWidget(tableNode, {
    emptyText: L10N.getStr("table.emptyText"),
    highlightUpdated: true,
  });
  this.displayObjectSidebar = this.displayObjectSidebar.bind(this);
  this.table.on(TableWidget.EVENTS.ROW_SELECTED, this.displayObjectSidebar)

  this.sidebar = this._panelDoc.getElementById("storage-sidebar");
  this.sidebar.setAttribute("width", "300");
  this.view = new VariablesView(this.sidebar.firstChild,
                                GENERIC_VARIABLES_VIEW_SETTINGS);

  this.front.listStores().then(storageTypes => {
    this.populateStorageTree(storageTypes);
  }).then(null, Cu.reportError);
  this.onUpdate = this.onUpdate.bind(this);
  this.front.on("stores-update", this.onUpdate);

  this.handleKeypress = this.handleKeypress.bind(this);
  this._panelDoc.addEventListener("keypress", this.handleKeypress);

  this._telemetry = new Telemetry();
  this._telemetry.toolOpened("storage");
}

exports.StorageUI = StorageUI;

StorageUI.prototype = {

  storageTypes: null,
  shouldResetColumns: true,

  destroy: function() {
    this.front.off("stores-update", this.onUpdate);
    this._panelDoc.removeEventListener("keypress", this.handleKeypress);
    this._telemetry.toolClosed("storage");
  },

  


  hideSidebar: function() {
    this.view.empty();
    this.sidebar.hidden = true;
    this.table.clearSelection();
  },

  




  removeItemFromTable: function(name) {
    if (this.table.isSelected(name)) {
      if (this.table.selectedIndex == 0) {
        this.table.selectNextRow()
      }
      else {
        this.table.selectPreviousRow();
      }
      this.table.remove(name);
      this.displayObjectSidebar();
    }
    else {
      this.table.remove(name);
    }
  },

  






















  onUpdate: function({ changed, added, deleted }) {
    if (deleted) {
      for (let type in deleted) {
        for (let host in deleted[type]) {
          if (!deleted[type][host].length) {
            
            
            if (this.tree.isSelected([type, host])) {
              this.table.clear();
              this.hideSidebar();
              this.tree.selectPreviousItem();
            }

            this.tree.remove([type, host]);
          }
          else if (this.tree.isSelected([type, host])) {
            for (let name of deleted[type][host]) {
              try {
                
                let names = JSON.parse(name);
                if (!names[2]) {
                  if (this.tree.isSelected([type, host, names[0], names[1]])) {
                    this.tree.selectPreviousItem();
                    this.tree.remove([type, host, names[0], names[1]]);
                    this.table.clear();
                    this.hideSidebar();
                  }
                }
                else if (this.tree.isSelected([type, host, names[0], names[1]])) {
                  this.removeItemFromTable(names[2]);
                }
              }
              catch (ex) {
                this.removeItemFromTable(name);
              }
            }
          }
        }
      }
    }

    if (added) {
      for (let type in added) {
        for (let host in added[type]) {
          this.tree.add([type, {id: host, type: "url"}]);
          for (let name of added[type][host]) {
            try {
              name = JSON.parse(name);
              if (name.length == 3) {
                name.splice(2, 1);
              }
              this.tree.add([type, host, ...name]);
              if (!this.tree.selectedItem) {
                this.tree.selectedItem = [type, host, name[0], name[1]];
                this.fetchStorageObjects(type, host, [JSON.stringify(name)], 1);
              }
            } catch(ex) {}
          }

          if (this.tree.isSelected([type, host])) {
            this.fetchStorageObjects(type, host, added[type][host], 1);
          }
        }
      }
    }

    if (changed) {
      let [type, host, db, objectStore] = this.tree.selectedItem;
      if (changed[type] && changed[type][host]) {
        if (changed[type][host].length) {
          try {
            let toUpdate = [];
            for (let name of changed[type][host]) {
              let names = JSON.parse(name);
              if (names[0] == db && names[1] == objectStore && names[2]) {
                toUpdate.push(name);
              }
            }
            this.fetchStorageObjects(type, host, toUpdate, 2);
          }
          catch (ex) {
            this.fetchStorageObjects(type, host, changed[type][host], 2);
          }
        }
      }
    }

    if (added || deleted || changed) {
      this.emit("store-objects-updated");
    }
  },

  













  fetchStorageObjects: function(type, host, names, reason) {
    this.storageTypes[type].getStoreObjects(host, names).then(({data}) => {
      if (!data.length) {
        this.emit("store-objects-updated");
        return;
      }
      if (this.shouldResetColumns) {
        this.resetColumns(data[0], type);
      }
      this.populateTable(data, reason);
      this.emit("store-objects-updated");
    }, Cu.reportError);
  },

  







  populateStorageTree: function(storageTypes) {
    this.storageTypes = {};
    for (let type in storageTypes) {
      let typeLabel = L10N.getStr("tree.labels." + type);
      this.tree.add([{id: type, label: typeLabel, type: "store"}]);
      if (storageTypes[type].hosts) {
        this.storageTypes[type] = storageTypes[type];
        for (let host in storageTypes[type].hosts) {
          this.tree.add([type, {id: host, type: "url"}]);
          for (let name of storageTypes[type].hosts[host]) {

            try {
              let names = JSON.parse(name);
              this.tree.add([type, host, ...names]);
              if (!this.tree.selectedItem) {
                this.tree.selectedItem = [type, host, names[0], names[1]];
                this.fetchStorageObjects(type, host, [name], 0);
              }
            } catch(ex) {}
          }
          if (!this.tree.selectedItem) {
            this.tree.selectedItem = [type, host];
            this.fetchStorageObjects(type, host, null, 0);
          }
        }
      }
    }
  },

  



  displayObjectSidebar: function() {
    let item = this.table.selectedRow;
    if (!item) {
      
      this.sidebar.hidden = true;
      return;
    }
    this.sidebar.hidden = false;
    this.view.empty();
    let mainScope = this.view.addScope(L10N.getStr("storage.data.label"));
    mainScope.expanded = true;

    if (item.name && item.valueActor) {
      let itemVar = mainScope.addItem(item.name + "", {}, true);

      item.valueActor.string().then(value => {
        
        itemVar.setGrip(value);

        
        this.parseItemValue(item.name, value);

        
        
        let itemProps = Object.keys(item);
        if (itemProps.length == 3) {
          this.emit("sidebar-updated");
          return;
        }

        
        
        let rawObject = Object.create(null);
        let otherProps =
          itemProps.filter(e => e != "name" && e != "value" && e != "valueActor");
        for (let prop of otherProps) {
          rawObject[prop] = item[prop];
        }
        itemVar.populate(rawObject, {sorted: true});
        itemVar.twisty = true;
        itemVar.expanded = true;
        this.emit("sidebar-updated");
      });
      return;
    }

    
    for (let key in item) {
      mainScope.addItem(key, {}, true).setGrip(item[key]);
      this.parseItemValue(key, item[key]);
    }
    this.emit("sidebar-updated");
  },

  









  parseItemValue: function(name, value) {
    let json = null
    try {
      json = JSON.parse(value);
    }
    catch (ex) {
      json = null;
    }

    if (!json && value) {
      json = this._extractKeyValPairs(value);
    }

    
    if (!json || json == value || typeof json == "string") {
      return;
    }

    
    if ((json.length == 2 || Object.keys(json).length == 1) &&
        ((json[0] || Object.keys(json)[0]) + "").match(/^(http|file|ftp)/)) {
      return;
    }

    let jsonObject = Object.create(null);
    jsonObject[name] = json;
    let valueScope = this.view.getScopeAtIndex(1) ||
                     this.view.addScope(L10N.getStr("storage.parsedValue.label"));
    valueScope.expanded = true;
    let jsonVar = valueScope.addItem("", Object.create(null), true);
    jsonVar.expanded = true;
    jsonVar.twisty = true;
    jsonVar.populate(jsonObject, {expanded: true});
  },

  







  _extractKeyValPairs: function(value) {
    let makeObject = (keySep, pairSep) => {
      let object = {};
      for (let pair of value.split(pairSep)) {
        let [key, val] = pair.split(keySep);
        object[key] = val;
      }
      return object;
    };

    
    const separators = ["=", ":", "~", "#", "&", "\\*", ",", "\\."];
    
    for (let i = 0; i < separators.length; i++) {
      let kv = separators[i];
      for (let j = 0; j < separators.length; j++) {
        if (i == j) {
          continue;
        }
        let p = separators[j];
        let regex = new RegExp("^([^" + kv + p + "]*" + kv + "+[^" + kv + p +
                               "]*" + p + "*)+$", "g");
        if (value.match(regex) && value.contains(kv) &&
            (value.contains(p) || value.split(kv).length == 2)) {
          return makeObject(kv, p);
        }
      }
    }
    
    for (let i = 0; i < separators.length; i++) {
      let p = separators[i];
      let regex = new RegExp("^[^" + p + "]+(" + p + "+[^" + p + "]*)+$", "g");
      if (value.match(regex)) {
        return value.split(p.replace(/\\*/g, ""));
      }
    }
    return null;
  },

  









  onHostSelect: function(event, item) {
    this.table.clear();
    this.hideSidebar();

    let [type, host] = item;
    let names = null;
    if (!host) {
      return;
    }
    if (item.length > 2) {
      names = [JSON.stringify(item.slice(2))];
    }
    this.shouldResetColumns = true;
    this.fetchStorageObjects(type, host, names, 0);
  },

  









  resetColumns: function(data, type) {
    let columns = {};
    let uniqueKey = null;
    for (let key in data) {
      if (!uniqueKey) {
        this.table.uniqueId = uniqueKey = key;
      }
      columns[key] = L10N.getStr("table.headers." + type + "." + key);
    }
    this.table.setColumns(columns, null, HIDDEN_COLUMNS);
    this.shouldResetColumns = false;
    this.hideSidebar();
  },

  









  populateTable: function(data, reason) {
    for (let item of data) {
      if (item.value) {
        item.valueActor = item.value;
        item.value = item.value.initial || "";
      }
      if (item.expires != null) {
        item.expires = item.expires
          ? new Date(item.expires).toLocaleString()
          : L10N.getStr("label.expires.session");
      }
      if (item.creationTime != null) {
        item.creationTime = new Date(item.creationTime).toLocaleString();
      }
      if (item.lastAccessed != null) {
        item.lastAccessed = new Date(item.lastAccessed).toLocaleString();
      }
      if (reason < 2) {
        this.table.push(item, reason == 0);
      }
      else {
        this.table.update(item);
        if (item == this.table.selectedRow && !this.sidebar.hidden) {
          this.displayObjectSidebar();
        }
      }
    }
  },

  





  handleKeypress: function(event) {
    if (event.keyCode == event.DOM_VK_ESCAPE && !this.sidebar.hidden) {
      
      this.hideSidebar();
      event.stopPropagation();
      event.preventDefault();
    }
  }
}
