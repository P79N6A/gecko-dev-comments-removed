

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

  







  highlightNode: function PanelHighlighter_highlightNode(element, params)
  {
    this.node = element;
    this.highlight(params && params.scroll);
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

  









  midPoint: function PanelHighlighter_midPoint(pointA, pointB)
  {
    let pointC = { };
    pointC.x = (pointB.x - pointA.x) / 2 + pointA.x;
    pointC.y = (pointB.y - pointA.y) / 2 + pointA.y;
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

    return this.win.document.elementFromPoint(midpoint.x, midpoint.y);
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

  
  

  





  handleMouseMove: function PanelHighlighter_handleMouseMove(event)
  {
    if (!InspectorUI.inspecting) {
      return;
    }
    let browserRect = this.browser.getBoundingClientRect();
    let element = this.win.document.elementFromPoint(event.clientX -
      browserRect.left, event.clientY - browserRect.top);
    if (element && element != this.node) {
      InspectorUI.inspectNode(element);
    }
  },
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

    for (let i = line.length - 1; i >= 0; i--) {
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
  _showStylePanel: false,
  _showDOMPanel: false,
  highlightColor: "#EEEE66",
  highlightThickness: 4,
  highlightOpacity: 0.4,
  selectEventsSuppressed: false,
  inspecting: false,

  





  toggleInspectorUI: function InspectorUI_toggleInspectorUI()
  {
    if (this.isPanelOpen) {
      this.closeInspectorUI();
    } else {
      this.openInspectorUI();
    }
  },

  




  get isPanelOpen()
  {
    return this.treePanel && this.treePanel.state == "open";
  },

  


  openTreePanel: function InspectorUI_openTreePanel()
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

  openStylePanel: function InspectorUI_openStylePanel()
  {
    
  },

  openDOMPanel: function InspectorUI_openDOMPanel()
  {
    
  },

  



  openInspectorUI: function InspectorUI_openInspectorUI()
  {
    
    this.browser = gBrowser.selectedBrowser;
    this.win = this.browser.contentWindow;

    
    if (this._showTreePanel) {
      this.openTreePanel();
    }
    if (this._showStylePanel) {
      this.openStylePanel();
    }
    if (this._showDOMPanel) {
      this.openDOMPanel();
    }
    this.initializeHighlighter();
    this.startInspecting();
    this.win.document.addEventListener("scroll", this, false);
    gBrowser.tabContainer.addEventListener("TabSelect", this, false);
    this.inspectCmd.setAttribute("checked", true);
  },

  


  initializeHighlighter: function InspectorUI_initializeHighlighter()
  {
    this.highlighter = new PanelHighlighter(this.browser, this.highlightColor,
      this.highlightThickness, this.highlightOpacity);
  },

  




  closeInspectorUI: function InspectorUI_closeInspectorUI()
  {
    this.win.document.removeEventListener("scroll", this, false);
    gBrowser.tabContainer.removeEventListener("TabSelect", this, false);
    this.stopInspecting();
    if (this.highlighter && this.highlighter.isHighlighting) {
      this.highlighter.unhighlight();
    }
    if (this.isPanelOpen) {
      this.treePanel.hidePopup();
      this.treeView.destroy();
    }
    this.inspectCmd.setAttribute("checked", false);
    this.browser = this.win = null; 
  },

  



  startInspecting: function InspectorUI_startInspecting()
  {
    this.attachPageListeners();
    this.inspecting = true;
  },

  



  stopInspecting: function InspectorUI_stopInspecting()
  {
    if (!this.inspecting)
      return;
    this.detachPageListeners();
    this.inspecting = false;
  },

  
  

  


  createDocumentModel: function InspectorUI_createDocumentModel()
  {
    this.treeView = new InspectorTreeView(this.win);
  },

  
  

  





  handleEvent: function InspectorUI_handleEvent(event)
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
        let element = this.win.document.elementFromPoint(event.clientX,
          event.clientY);
        if (element && element != this.node) {
          this.inspectNode(element);
        }
        break;
      case "click":
        this.stopInspecting();
        break;
      case "scroll":
        this.highlighter.highlight();
        break;
    }
  },

  


  onTreeSelected: function InspectorUI_onTreeSelected()
  {
    if (this.selectEventsSuppressed) {
      return false;
    }

    let treeView = this.treeView;
    let node = treeView.selectedNode;
    this.highlighter.highlightNode(node); 
    this.stopInspecting();
    return true;
  },

  



  attachPageListeners: function InspectorUI_attachPageListeners()
  {
    this.win.addEventListener("keypress", this, true);
    this.browser.addEventListener("mousemove", this, true);
    this.browser.addEventListener("click", this, true);
  },

  



  detachPageListeners: function InspectorUI_detachPageListeners()
  {
    this.win.removeEventListener("keypress", this, true);
    this.browser.removeEventListener("mousemove", this, true);
    this.browser.removeEventListener("click", this, true);
  },

  
  

  






  inspectNode: function InspectorUI_inspectNode(element)
  {
    this.highlighter.highlightNode(element);
    this.selectEventsSuppressed = true;
    this.treeView.selectedNode = element;
    this.selectEventsSuppressed = false;
  },

  
  

  




  _log: function LOG(msg)
  {
    Services.console.logStringMessage(msg);
  },
}

XPCOMUtils.defineLazyGetter(InspectorUI, "inspectCmd", function () {
  return document.getElementById("Tools:Inspect");
});

