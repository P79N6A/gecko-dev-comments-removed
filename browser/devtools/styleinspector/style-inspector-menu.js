







"use strict";

const {Cc, Ci, Cu} = require("chrome");
const {PREF_ORIG_SOURCES} = require("devtools/styleeditor/utils");

loader.lazyRequireGetter(this, "overlays", "devtools/styleinspector/style-inspector-overlays");
loader.lazyImporter(this, "Services", "resource://gre/modules/Services.jsm");
loader.lazyServiceGetter(this, "clipboardHelper", "@mozilla.org/widget/clipboardhelper;1", "nsIClipboardHelper");
loader.lazyGetter(this, "_strings", () => {
  return Services.strings
  .createBundle("chrome://global/locale/devtools/styleinspector.properties");
});

const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
const PREF_ENABLE_MDN_DOCS_TOOLTIP = "devtools.inspector.mdnDocsTooltip.enabled";






function StyleInspectorMenu(view, options) {
  this.view = view;
  this.inspector = this.view.inspector;
  this.styleDocument = this.view.styleDocument;
  this.styleWindow = this.view.styleWindow;

  this.isRuleView = options.isRuleView;

  this._onAddNewRule = this._onAddNewRule.bind(this);
  this._onCopy = this._onCopy.bind(this);
  this._onCopyColor = this._onCopyColor.bind(this);
  this._onCopyImageDataUrl = this._onCopyImageDataUrl.bind(this);
  this._onCopyLocation = this._onCopyLocation.bind(this);
  this._onCopyPropertyDeclaration = this._onCopyPropertyDeclaration.bind(this);
  this._onCopyPropertyName = this._onCopyPropertyName.bind(this);
  this._onCopyPropertyValue = this._onCopyPropertyValue.bind(this);
  this._onCopyRule = this._onCopyRule.bind(this);
  this._onCopySelector = this._onCopySelector.bind(this);
  this._onCopyUrl = this._onCopyUrl.bind(this);
  this._onSelectAll = this._onSelectAll.bind(this);
  this._onShowMdnDocs = this._onShowMdnDocs.bind(this);
  this._onToggleOrigSources = this._onToggleOrigSources.bind(this);
  this._updateMenuItems = this._updateMenuItems.bind(this);

  this._createContextMenu();
}

module.exports = StyleInspectorMenu;

StyleInspectorMenu.prototype = {
  


  show: function(event) {
    try {
      
      
      this.styleDocument.popupNode = event.explicitOriginalTarget;
      this.styleWindow.focus();
      this._menupopup.openPopupAtScreen(event.screenX, event.screenY, true);
    } catch(e) {
      console.error(e);
    }
  },

  _createContextMenu: function() {
    this._menupopup = this.styleDocument.createElementNS(XUL_NS, "menupopup");
    this._menupopup.addEventListener("popupshowing", this._updateMenuItems);
    this._menupopup.id = "computed-view-context-menu";

    let parentDocument = this.styleWindow.parent.document;
    let popupset = parentDocument.documentElement.querySelector("popupset");
    if (!popupset) {
      popupset = parentDocument.createElementNS(XUL_NS, "popupset");
      parentDocument.documentElement.appendChild(popupset);
    }
    popupset.appendChild(this._menupopup);

    this._createContextMenuItems();
  },
  


  _createContextMenuItems: function() {
    this.menuitemCopy = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copy",
      accesskey: "styleinspector.contextmenu.copy.accessKey",
      command: this._onCopy
    });

    this.menuitemCopyLocation = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copyLocation",
      command: this._onCopyLocation
    });

    this.menuitemCopyRule = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copyRule",
      command: this._onCopyRule
    });

    this.menuitemCopyColor = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copyColor",
      accesskey: "styleinspector.contextmenu.copyColor.accessKey",
      command: this._onCopyColor
    });

    this.menuitemCopyUrl = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copyUrl",
      accesskey: "styleinspector.contextmenu.copyUrl.accessKey",
      command: this._onCopyUrl
    });

    this.menuitemCopyImageDataUrl = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copyImageDataUrl",
      accesskey: "styleinspector.contextmenu.copyImageDataUrl.accessKey",
      command: this._onCopyImageDataUrl
    });

    this.menuitemCopyPropertyDeclaration = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copyPropertyDeclaration",
      command: this._onCopyPropertyDeclaration
    });

    this.menuitemCopyPropertyName = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copyPropertyName",
      command: this._onCopyPropertyName
    });

    this.menuitemCopyPropertyValue = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copyPropertyValue",
      command: this._onCopyPropertyValue
    });

    this.menuitemCopySelector = this._createContextMenuItem({
      label: "styleinspector.contextmenu.copySelector",
      command: this._onCopySelector
    });

    this._createMenuSeparator();

    
    this.menuitemSelectAll = this._createContextMenuItem({
      label: "styleinspector.contextmenu.selectAll",
      accesskey: "styleinspector.contextmenu.selectAll.accessKey",
      command: this._onSelectAll
    });

    this._createMenuSeparator();

    
    this.menuitemAddRule = this._createContextMenuItem({
      label: "styleinspector.contextmenu.addNewRule",
      accesskey: "styleinspector.contextmenu.addNewRule.accessKey",
      command: this._onAddNewRule
    });

    
    this.menuitemShowMdnDocs = this._createContextMenuItem({
      label: "styleinspector.contextmenu.showMdnDocs",
      accesskey: "styleinspector.contextmenu.showMdnDocs.accessKey",
      command: this._onShowMdnDocs
    });

    
    this.menuitemSources = this._createContextMenuItem({
      label: "styleinspector.contextmenu.toggleOrigSources",
      accesskey: "styleinspector.contextmenu.toggleOrigSources.accessKey",
      command: this._onToggleOrigSources,
      type: "checkbox"
    });
  },

  



  _createContextMenuItem: function(attributes) {
    let ownerDocument = this._menupopup.ownerDocument;
    let item = ownerDocument.createElementNS(XUL_NS, "menuitem");

    item.setAttribute("label", _strings.GetStringFromName(attributes.label));
    if (attributes.accesskey) {
      item.setAttribute("accesskey", _strings.GetStringFromName(attributes.accesskey));
    }
    item.addEventListener("command", attributes.command);

    if (attributes.type) {
      item.setAttribute("type", attributes.type);
    }

    this._menupopup.appendChild(item);
    return item;
  },

  _createMenuSeparator: function() {
    let ownerDocument = this._menupopup.ownerDocument;
    let separator = ownerDocument.createElementNS(XUL_NS, "menuseparator");
    this._menupopup.appendChild(separator);
  },

  



  _updateMenuItems: function() {
    this._updateCopyMenuItems();

    let showOrig = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
    this.menuitemSources.setAttribute("checked", showOrig);

    let enableMdnDocsTooltip = Services.prefs.getBoolPref(PREF_ENABLE_MDN_DOCS_TOOLTIP);
    this.menuitemShowMdnDocs.hidden = !(enableMdnDocsTooltip && this._isPropertyName());

    this.menuitemAddRule.disabled = !(this.isRuleView && !this.inspector.selection.isAnonymousNode());
  },

  



  _updateCopyMenuItems: function() {
    this.menuitemCopy.hidden = !this._hasTextSelected();
    this.menuitemCopyColor.hidden = !this._isColorPopup();
    this.menuitemCopyImageDataUrl.hidden = !this._isImageUrl();

    this.menuitemCopyRule.hidden = true;
    this.menuitemCopyLocation.hidden = true;
    this.menuitemCopyPropertyDeclaration.hidden = true;
    this.menuitemCopyPropertyName.hidden = true;
    this.menuitemCopyPropertyValue.hidden = true;
    this.menuitemCopySelector.hidden = true;

    this._clickedNodeInfo = this._getClickedNodeInfo();
    if (this.isRuleView && this._clickedNodeInfo) {
      this.menuitemCopyRule.hidden = false;

      switch (this._clickedNodeInfo.type) {
        case overlays.VIEW_NODE_PROPERTY_TYPE :
          this.menuitemCopyPropertyDeclaration.hidden = false;
          this.menuitemCopyPropertyName.hidden = false;
          break;
        case overlays.VIEW_NODE_VALUE_TYPE :
          this.menuitemCopyPropertyDeclaration.hidden = false;
          this.menuitemCopyPropertyValue.hidden = false;
          break;
        case overlays.VIEW_NODE_SELECTOR_TYPE :
          this.menuitemCopySelector.hidden = false;
          break;
        case overlays.VIEW_NODE_LOCATION_TYPE :
          this.menuitemCopyLocation.hidden = false;
          break;
      }
    }
  },

  _hasTextSelected: function() {
    let hasTextSelected;
    let selection = this.styleWindow.getSelection();

    let node = this._getClickedNode();
    if (node.nodeName == "input") {
       
      let { selectionStart, selectionEnd } = node;
      hasTextSelected = isFinite(selectionStart) && isFinite(selectionEnd)
        && selectionStart !== selectionEnd;
    } else {
      hasTextSelected = selection.toString() && !selection.isCollapsed;
    }

    return hasTextSelected;
  },

  


  _getClickedNodeInfo: function() {
    let node = this._getClickedNode();
    return this.view.getNodeInfo(node);
  },

  






  _isColorPopup: function() {
    this._colorToCopy = "";

    let container = this._getClickedNode();
    if (!container) {
      return false;
    }

    let isColorNode = el => el.dataset && "color" in el.dataset;

    while (!isColorNode(container)) {
      container = container.parentNode;
      if (!container) {
        return false;
      }
    }

    this._colorToCopy = container.dataset.color;
    return true;
  },

  _isPropertyName: function() {
    let nodeInfo = this._getClickedNodeInfo();
    if (!nodeInfo) {
      return false;
    }
    return nodeInfo.type == overlays.VIEW_NODE_PROPERTY_TYPE;
  },

  



  _isImageUrl: function() {
    let nodeInfo = this._getClickedNodeInfo();
    if (!nodeInfo) {
      return false;
    }
    return nodeInfo.type == overlays.VIEW_NODE_IMAGE_URL_TYPE;
  },

  




  _getClickedNode: function() {
    let container = null;
    let node = this.styleDocument.popupNode;

    if (node) {
      let isTextNode = node.nodeType == node.TEXT_NODE;
      container = isTextNode ? node.parentElement : node;
    }

    return container;
  },

  


  _onSelectAll: function() {
    let selection = this.styleWindow.getSelection();
    selection.selectAllChildren(this.styleDocument.documentElement);
  },

  


  _onCopy: function(event) {
    this.view.copySelection(this.styleDocument.popupNode);
  },

  


  _onCopyColor: function() {
    clipboardHelper.copyString(this._colorToCopy);
  },

  


  _onCopyUrl: function() {
    clipboardHelper.copyString(this._clickedNodeInfo.value.url);
  },

  


  _onCopyImageDataUrl: Task.async(function*() {
    if (!this._clickedNodeInfo) {
      return;
    }

    let message;
    try {
      let inspectorFront = this.inspector.inspector;
      let imageUrl = this._clickedNodeInfo.value.url;
      let data = yield inspectorFront.getImageDataFromURL(imageUrl);
      message = yield data.data.string();
    } catch (e) {
      message = _strings.GetStringFromName("styleinspector.copyImageDataUrlError");
    }

    clipboardHelper.copyString(message);
  }),

  


  _onShowMdnDocs: function() {
    let cssPropertyName = this.styleDocument.popupNode.textContent;
    let anchor = this.styleDocument.popupNode.parentNode;
    let cssDocsTooltip = this.view.tooltips.cssDocs;
    cssDocsTooltip.show(anchor, cssPropertyName);
  },

  


  _onAddNewRule: function() {
    this.view._onAddRule();
  },

  


  _onCopyLocation: function() {
    if (!this._clickedNodeInfo) {
      return;
    }

    clipboardHelper.copyString(this._clickedNodeInfo.value);
  },

  


  _onCopyPropertyDeclaration: function() {
    if (!this._clickedNodeInfo) {
      return;
    }

    let textProp = this._clickedNodeInfo.value.textProperty;
    clipboardHelper.copyString(textProp.stringifyProperty());
  },

  


  _onCopyPropertyName: function() {
    if (!this._clickedNodeInfo) {
      return;
    }

    clipboardHelper.copyString(this._clickedNodeInfo.value.property);
  },

  


  _onCopyPropertyValue: function() {
    if (!this._clickedNodeInfo) {
      return;
    }

    clipboardHelper.copyString(this._clickedNodeInfo.value.value);
  },

  


  _onCopyRule: function() {
    let ruleEditor = this.styleDocument.popupNode.parentNode.offsetParent._ruleEditor;
    let rule = ruleEditor.rule;
    clipboardHelper.copyString(rule.stringifyRule());
  },

  


  _onCopySelector: function() {
    if (!this._clickedNodeInfo) {
      return;
    }

    clipboardHelper.copyString(this._clickedNodeInfo.value);
  },

  


  _onToggleOrigSources: function() {
    let isEnabled = Services.prefs.getBoolPref(PREF_ORIG_SOURCES);
    Services.prefs.setBoolPref(PREF_ORIG_SOURCES, !isEnabled);
  },

  destroy: function() {
    this._removeContextMenuItems();

    
    this._menupopup.removeEventListener("popupshowing", this._updateMenuItems);
    this._menupopup.parentNode.removeChild(this._menupopup);
    this._menupopup = null;

    this.popupNode = null;
    this.styleDocument.popupNode = null;
    this.view = null;
    this.inspector = null;
    this.styleDocument = null;
    this.styleWindow = null;
  },

  _removeContextMenuItems: function() {
    this._removeContextMenuItem("menuitemAddRule", this._onAddNewRule);
    this._removeContextMenuItem("menuitemCopy", this._onCopy);
    this._removeContextMenuItem("menuitemCopyColor", this._onCopyColor);
    this._removeContextMenuItem("menuitemCopyImageDataUrl", this._onCopyImageDataUrl);
    this._removeContextMenuItem("menuitemCopyLocation", this._onCopyLocation);
    this._removeContextMenuItem("menuitemCopyPropertyDeclaration", this._onCopyPropertyDeclaration);
    this._removeContextMenuItem("menuitemCopyPropertyName", this._onCopyPropertyName);
    this._removeContextMenuItem("menuitemCopyPropertyValue", this._onCopyPropertyValue);
    this._removeContextMenuItem("menuitemCopyRule", this._onCopyRule);
    this._removeContextMenuItem("menuitemCopySelector", this._onCopySelector);
    this._removeContextMenuItem("menuitemCopyUrl", this._onCopyUrl);
    this._removeContextMenuItem("menuitemSelectAll", this._onSelectAll);
    this._removeContextMenuItem("menuitemShowMdnDocs", this._onShowMdnDocs);
    this._removeContextMenuItem("menuitemSources", this._onToggleOrigSources);
  },

  _removeContextMenuItem: function(itemName, callback) {
    if (this[itemName]) {
      this[itemName].removeEventListener("command", callback);
      this[itemName] = null;
    }
  }
};
