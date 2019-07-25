

#ifdef 0






































#endif

const INSPECTOR_INVISIBLE_ELEMENTS = {
  "head": true,
  "base": true,
  "basefont": true,
  "isindex": true,
  "link": true,
  "meta": true,
  "script": true,
  "style": true,
  "title": true,
};
















function PanelHighlighter(aBrowser, aColor, aBorderSize, anOpacity)
{
  this.panel = document.getElementById("highlighter-panel");
  this.panel.hidden = false;
  this.browser = aBrowser;
  this.win = this.browser.contentWindow;
  this.backgroundColor = aColor;
  this.border = aBorderSize;
  this.opacity = anOpacity;
  this.updatePanelStyles();
}

PanelHighlighter.prototype = {
  
  




  updatePanelStyles: function PanelHighlighter_updatePanelStyles()
  {
    let style = this.panel.style;
    style.backgroundColor = this.backgroundColor;
    style.border = "solid blue " + this.border + "px";
    style.MozBorderRadius = "4px";
    style.opacity = this.opacity;
  },

  





  highlight: function PanelHighlighter_highlight(scroll)
  {
    
    if (!this.isNodeHighlightable()) {
      return;
    }

    this.unhighlight();

    let rect = this.node.getBoundingClientRect();

    if (scroll) {
      this.node.scrollIntoView();
    }

    if (this.viewContainsRect(rect)) {
      
      this.panel.openPopup(this.node, "overlap", 0, 0, false, false);
      this.panel.sizeTo(rect.width, rect.height);
    } else {
      this.highlightVisibleRegion(rect);
    }
  },

  







  highlightNode: function PanelHighlighter_highlightNode(aNode, aParams)
  {
    this.node = aNode;
    this.highlight(aParams && aParams.scroll);
  },

  






  highlightVisibleRegion: function PanelHighlighter_highlightVisibleRegion(aRect)
  {
    let offsetX = 0;
    let offsetY = 0;
    let width = 0;
    let height = 0;
    let visibleWidth = this.win.innerWidth;
    let visibleHeight = this.win.innerHeight;

    
    
    if (aRect.top > visibleHeight || aRect.left > visibleWidth ||
        aRect.bottom < 0 || aRect.right < 0) {
      return false;
    }

    
    
    
    offsetX = aRect.left < 0 ? Math.abs(aRect.left) : 0;
    offsetY = aRect.top < 0 ? Math.abs(aRect.top) : 0;

    
    
    width = aRect.right > visibleWidth ? visibleWidth - aRect.left :
      aRect.width;
    width -= offsetX;

    
    height = aRect.bottom > visibleHeight ? visibleHeight - aRect.top :
      aRect.height;
    height -= offsetY;

    
    
    if (width > 0 && height > 0) {
      this.panel.openPopup(this.node, "overlap", offsetX, offsetY, false,
        false);
      this.panel.sizeTo(width, height);
      return true;
    }

    return false;
  },

  


  unhighlight: function PanelHighlighter_unhighlight()
  {
    if (this.isHighlighting) {
      this.panel.hidePopup();
    }
  },

  




  get isHighlighting()
  {
    return this.panel.state == "open";
  },

  









  midPoint: function PanelHighlighter_midPoint(aPointA, aPointB)
  {
    let pointC = { };
    pointC.x = (aPointB.x - aPointA.x) / 2 + aPointA.x;
    pointC.y = (aPointB.y - aPointA.y) / 2 + aPointA.y;
    return pointC;
  },

  






  get highlitNode()
  {
    
    if (!this.isHighlighting) {
      return null;
    }

    let browserRect = this.browser.getBoundingClientRect();
    let clientRect = this.panel.getBoundingClientRect();

    
    let a = {
      x: clientRect.left - browserRect.left,
      y: clientRect.top - browserRect.top
    };

    
    let b = {
      x: clientRect.right - browserRect.left,
      y: clientRect.bottom - browserRect.top
    };

    
    let midpoint = this.midPoint(a, b);

    return InspectorUI.elementFromPoint(this.win.document, midpoint.x,
      midpoint.y);
  },

  




  isNodeHighlightable: function PanelHighlighter_isNodeHighlightable()
  {
    if (!this.node) {
      return false;
    }
    let nodeName = this.node.nodeName.toLowerCase();
    if (nodeName[0] == '#') {
      return false;
    }
    return !INSPECTOR_INVISIBLE_ELEMENTS[nodeName];
  },

  







  viewContainsRect: function PanelHighlighter_viewContainsRect(aRect)
  {
    let visibleWidth = this.win.innerWidth;
    let visibleHeight = this.win.innerHeight;

    return ((0 <= aRect.left) && (aRect.right <= visibleWidth) && 
        (0 <= aRect.top) && (aRect.bottom <= visibleHeight))
  },

  
  

  





  handleMouseMove: function PanelHighlighter_handleMouseMove(aEvent)
  {
    if (!InspectorUI.inspecting) {
      return;
    }
    let browserRect = this.browser.getBoundingClientRect();
    let element = InspectorUI.elementFromPoint(this.win.document,
      aEvent.clientX - browserRect.left, aEvent.clientY - browserRect.top);
    if (element && element != this.node) {
      InspectorUI.inspectNode(element);
    }
  },

  






  handlePixelScroll: function PanelHighlighter_handlePixelScroll(aEvent) {
    if (!InspectorUI.inspecting) {
      return;
    }
    let browserRect = this.browser.getBoundingClientRect();
    let element = InspectorUI.elementFromPoint(this.win.document,
      aEvent.clientX - browserRect.left, aEvent.clientY - browserRect.top);
    let win = element.ownerDocument.defaultView;

    if (aEvent.axis == aEvent.HORIZONTAL_AXIS) {
      win.scrollBy(aEvent.detail, 0);
    } else {
      win.scrollBy(0, aEvent.detail);
    }
  }
};











function InspectorTreeView(aWindow)
{
  this.tree = document.getElementById("inspector-tree");
  this.treeBody = document.getElementById("inspector-tree-body");
  this.view = Cc["@mozilla.org/inspector/dom-view;1"]
              .createInstance(Ci.inIDOMView);
  this.view.showSubDocuments = true;
  this.view.whatToShow = NodeFilter.SHOW_ALL;
  this.tree.view = this.view;
  this.contentWindow = aWindow;
  this.view.rootNode = aWindow.document;
  this.view.rebuild();
}

InspectorTreeView.prototype = {
  get editable() { return false; },
  get selection() { return this.view.selection; },

  


  destroy: function ITV_destroy()
  {
    this.tree.view = null;
    this.view = null;
    this.tree = null;
  },

  








  getCellText: function ITV_getCellText(aRow, aCol)
  {
    return this.view.getCellText(aRow, aCol);
  },

  




  get selectionIndex()
  {
    return this.selection.currentIndex;
  },

  




  get selectedNode()
  {
    let rowIndex = this.selectionIndex;
    return this.view.getNodeFromRowIndex(rowIndex);
  },

  





  set selectedRow(anIndex)
  {
    this.view.selection.select(anIndex);
    this.tree.treeBoxObject.ensureRowIsVisible(anIndex);
  },

  





  set selectedNode(aNode)
  {
    let rowIndex = this.view.getRowIndexFromNode(aNode);
    if (rowIndex > -1) {
      this.selectedRow = rowIndex;
    } else {
      this.selectElementInTree(aNode);
    }
  },

  








  selectElementInTree: function ITV_selectElementInTree(aNode)
  {
    if (!aNode) {
      this.view.selection.select(null);
      return false;      
    }

    
    
    let domUtils = Cc["@mozilla.org/inspector/dom-utils;1"].
                    getService(Ci.inIDOMUtils);
    let line = [];
    let parent = aNode;
    let index = null;

    while (parent) {
      index = this.view.getRowIndexFromNode(parent);
      line.push(parent);
      if (index < 0) {
        
        parent = domUtils.getParentForNode(parent,
          this.view.showAnonymousContent);
      } else {
        break;
      }
    }

    
    
    let lastIndex;
    let view = this.tree.treeBoxObject.view;

    for (let i = line.length - 1; i >= 0; --i) {
      index = this.view.getRowIndexFromNode(line[i]);
      if (index < 0) {
        
        break;
      }
      if (i > 0 && !view.isContainerOpen(index)) {
        view.toggleOpenState(index);
      }
      lastIndex = index;
    }

    if (lastIndex >= 0) {
      this.selectedRow = lastIndex;
      return true;
    }
    
    return false;
  },
};







var InspectorUI = {
  browser: null,
  _showTreePanel: true,
  _showStylePanel: true,
  _showDOMPanel: false,
  highlightColor: "#EEEE66",
  highlightThickness: 4,
  highlightOpacity: 0.4,
  selectEventsSuppressed: false,
  inspecting: false,

  





  toggleInspectorUI: function IUI_toggleInspectorUI(aEvent)
  {
    if (this.isPanelOpen) {
      this.closeInspectorUI();
    } else {
      this.openInspectorUI();
    }
  },

  



  toggleInspection: function IUI_toggleInspection()
  {
    if (this.inspecting) {
      this.stopInspecting();
    } else {
      this.startInspecting();
    }
  },

  


  toggleStylePanel: function IUI_toggleStylePanel()
  {
    if (this._showStylePanel) {
      this.stylePanel.hidePopup();
    } else {
      this.openStylePanel();
      if (this.treeView.selectedNode) {
        this.updateStylePanel(this.treeView.selectedNode);
      }
    }
    this._showStylePanel = !this._showStylePanel;
  },

  




  get isPanelOpen()
  {
    return this.treePanel && this.treePanel.state == "open";
  },

  




  get isStylePanelOpen()
  {
    return this.stylePanel && this.stylePanel.state == "open";
  },

  


  openTreePanel: function IUI_openTreePanel()
  {
    if (!this.treePanel) {
      this.treePanel = document.getElementById("inspector-panel");
      this.treePanel.hidden = false;
    }
    if (!this.isPanelOpen) {
      const panelWidthRatio = 7 / 8;
      const panelHeightRatio = 1 / 5;
      let bar = document.getElementById("status-bar");
      this.treePanel.openPopup(bar, "overlap", 120, -120, false, false);
      this.treePanel.sizeTo(this.win.outerWidth * panelWidthRatio, 
        this.win.outerHeight * panelHeightRatio);
      this.tree = document.getElementById("inspector-tree");
      this.createDocumentModel();
    }
  },

  


  openStylePanel: function IUI_openStylePanel()
  {
    if (!this.stylePanel) {
      this.stylePanel = document.getElementById("inspector-style-panel");
      this.stylePanel.hidden = false;
    }
    if (!this.isStylePanelOpen) {
      
      this.stylePanel.openPopup(this.browser, "end_before", 0, 20, false, false);
      
      this.stylePanel.sizeTo(200, this.win.outerHeight / 2 - 60);
    }
  },

  






  toggleDimForPanel: function IUI_toggleDimForPanel(aDim)
  {
    if (aDim.hasAttribute("dimmed")) {
      aDim.removeAttribute("dimmed");
    } else {
      aDim.setAttribute("dimmed", "true");
    }
  },

  openDOMPanel: function IUI_openDOMPanel()
  {
    
  },

  



  openInspectorUI: function IUI_openInspectorUI()
  {
    
    this.browser = gBrowser.selectedBrowser;
    this.win = this.browser.contentWindow;
    if (!this.style) {
      Cu.import("resource:///modules/stylePanel.jsm", this);
      this.style.initialize();
    }

    
    if (this._showTreePanel) {
      this.openTreePanel();
    }
    if (this._showStylePanel) {
      this.styleBox = document.getElementById("inspector-style-listbox");
      this.clearStylePanel();
      this.openStylePanel();
    }
    if (this._showDOMPanel) {
      this.openDOMPanel();
    }
    this.inspectorBundle = Services.strings.createBundle("chrome://browser/locale/inspector.properties");
    this.initializeHighlighter();
    this.startInspecting();
    this.win.document.addEventListener("scroll", this, false);
    this.win.addEventListener("resize", this, false);
    gBrowser.tabContainer.addEventListener("TabSelect", this, false);
    this.inspectCmd.setAttribute("checked", true);
  },

  


  initializeHighlighter: function IUI_initializeHighlighter()
  {
    this.highlighter = new PanelHighlighter(this.browser, this.highlightColor,
      this.highlightThickness, this.highlightOpacity);
  },

  




  closeInspectorUI: function IUI_closeInspectorUI()
  {
    this.win.document.removeEventListener("scroll", this, false);
    this.win.removeEventListener("resize", this, false);
    gBrowser.tabContainer.removeEventListener("TabSelect", this, false);
    this.stopInspecting();
    if (this.highlighter && this.highlighter.isHighlighting) {
      this.highlighter.unhighlight();
    }
    if (this.isPanelOpen) {
      this.treePanel.hidePopup();
      this.treeView.destroy();
    }
    if (this.isStylePanelOpen) {
      this.stylePanel.hidePopup();
    }
    this.inspectCmd.setAttribute("checked", false);
    this.browser = this.win = null; 
  },

  



  startInspecting: function IUI_startInspecting()
  {
    this.attachPageListeners();
    this.inspecting = true;
    this.toggleDimForPanel(this.stylePanel);
  },

  



  stopInspecting: function IUI_stopInspecting()
  {
    if (!this.inspecting)
      return;
    this.detachPageListeners();
    this.inspecting = false;
    this.toggleDimForPanel(this.stylePanel);
    if (this.treeView.selection) {
      this.updateStylePanel(this.treeView.selectedNode);
    }
  },

  
  

  


  createDocumentModel: function IUI_createDocumentModel()
  {
    this.treeView = new InspectorTreeView(this.win);
  },

  









  addStyleItem: function IUI_addStyleItem(aLabel, aType, aContent)
  {
    let itemLabelString = this.inspectorBundle.GetStringFromName("style.styleItemLabel");
    let item = document.createElement("listitem");

    
    let label = aLabel;
    item.className = "style-" + aType;
    if (aContent) {
      label = itemLabelString.replace("#1", aLabel);
      label = label.replace("#2", aContent);
    }
    item.setAttribute("label", label);

    this.styleBox.appendChild(item);
  },

  





  createStyleRuleItems: function IUI_createStyleRuleItems(aRules)
  {
    let selectorLabel = this.inspectorBundle.GetStringFromName("style.selectorLabel");

    aRules.forEach(function(rule) {
      this.addStyleItem(selectorLabel, "selector", rule.id);
      rule.properties.forEach(function(property) {
        if (property.overridden)
          return; 
        
        let important = "";
        if (property.important)
          important += " !important";
        this.addStyleItem(property.name, "property", property.value + important);
      }, this);
    }, this);
  },

  









  createStyleItems: function IUI_createStyleItems(aRules, aSections)
  {
    this.createStyleRuleItems(aRules);
    let inheritedString = 
        this.inspectorBundle.GetStringFromName("style.inheritedFrom");
    aSections.forEach(function(section) {
      let sectionTitle = section.element.tagName;
      if (section.element.id)
        sectionTitle += "#" + section.element.id;
      let replacedString = inheritedString.replace("#1", sectionTitle);
      this.addStyleItem(replacedString, "section");
      this.createStyleRuleItems(section.rules);
    }, this);
  },

  


  clearStylePanel: function IUI_clearStylePanel()
  {
    for (let i = this.styleBox.childElementCount; i >= 0; --i)
      this.styleBox.removeItemAt(i);
  },

  






  updateStylePanel: function IUI_updateStylePanel(aNode)
  {
    if (this.inspecting || !this.isStylePanelOpen)
      return;
    let rules = [], styleSections = [], usedProperties = {};
    this.style.getInheritedRules(aNode, styleSections, usedProperties);
    this.style.getElementRules(aNode, rules, usedProperties);
    this.clearStylePanel();
    this.createStyleItems(rules, styleSections);
  },

  
  

  





  handleEvent: function IUI_handleEvent(event)
  {
    switch (event.type) {
      case "TabSelect":
        this.closeInspectorUI();
        break;
      case "keypress":
        switch (event.keyCode) {
          case KeyEvent.DOM_VK_RETURN:
          case KeyEvent.DOM_VK_ESCAPE:
            this.stopInspecting();
            break;
        }
        break;
      case "mousemove":
        let element = this.elementFromPoint(event.target.ownerDocument,
          event.clientX, event.clientY);
        if (element && element != this.node) {
          this.inspectNode(element);
        }
        break;
      case "click":
        this.stopInspecting();
        break;
      case "scroll":
      case "resize":
        this.highlighter.highlight();
        break;
    }
  },

  


  onTreeSelected: function IUI_onTreeSelected()
  {
    if (this.selectEventsSuppressed) {
      return false;
    }

    let treeView = this.treeView;
    let node = treeView.selectedNode;
    this.highlighter.highlightNode(node);
    this.stopInspecting();
    this.updateStylePanel(node);
    return true;
  },

  



  attachPageListeners: function IUI_attachPageListeners()
  {
    this.win.addEventListener("keypress", this, true);
    this.browser.addEventListener("mousemove", this, true);
    this.browser.addEventListener("click", this, true);
  },

  



  detachPageListeners: function IUI_detachPageListeners()
  {
    this.win.removeEventListener("keypress", this, true);
    this.browser.removeEventListener("mousemove", this, true);
    this.browser.removeEventListener("click", this, true);
  },

  
  

  






  inspectNode: function IUI_inspectNode(aNode)
  {
    this.highlighter.highlightNode(aNode);
    this.selectEventsSuppressed = true;
    this.treeView.selectedNode = aNode;
    this.selectEventsSuppressed = false;
    this.updateStylePanel(aNode);
  },

  








  elementFromPoint: function IUI_elementFromPoint(aDocument, aX, aY)
  {
    let node = aDocument.elementFromPoint(aX, aY);
    if (node && node.contentDocument) {
      switch (node.nodeName.toLowerCase()) {
        case "iframe":
          let rect = node.getBoundingClientRect();
          aX -= rect.left;
          aY -= rect.top;

        case "frame":
          let subnode = this.elementFromPoint(node.contentDocument, aX, aY);
          if (subnode) {
            node = subnode;
          }
      }
    }
    return node;
  },

  
  

  




  _log: function LOG(msg)
  {
    Services.console.logStringMessage(msg);
  },
}

XPCOMUtils.defineLazyGetter(InspectorUI, "inspectCmd", function () {
  return document.getElementById("Tools:Inspect");
});

