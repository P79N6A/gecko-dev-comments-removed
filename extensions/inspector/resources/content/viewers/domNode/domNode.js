













































var viewer;

var gPromptService;



const kDOMViewCID          = "@mozilla.org/inspector/dom-view;1";



window.addEventListener("load", DOMNodeViewer_initialize, false);

function DOMNodeViewer_initialize()
{
  viewer = new DOMNodeViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}




function DOMNodeViewer()  
{
  this.mObsMan = new ObserverManager(this);

  this.mURL = window.location;
  this.mAttrTree = document.getElementById("olAttr");

  
  this.mDOMView = XPCU.createInstance(kDOMViewCID, "inIDOMView");
  this.mDOMView.whatToShow = NodeFilter.SHOW_ATTRIBUTE;
  this.mAttrTree.treeBoxObject.view = this.mDOMView;
}

DOMNodeViewer.prototype = 
{
  
  
  
  mDOMView: null,
  mSubject: null,
  mPanel: null,

  get selectedIndex()
  {
    return this.mAttrTree.currentIndex;
  },

 



  get selectedIndices()
  {
    var indices = [];
    var rangeCount = this.mAttrTree.view.selection.getRangeCount();
    for (var i = 0; i < rangeCount; ++i) {
      var start = {};
      var end = {};
      this.mAttrTree.view.selection.getRangeAt(i, start, end);
      for (var c = start.value; c <= end.value; ++c) {
        indices.push(c);
      }
    }
    return indices;
  },

 



  get selectedAttribute()
  {
    var index = this.selectedIndex;
    return index >= 0 ?
      new DOMAttribute(this.mDOMView.getNodeFromRowIndex(index)) : null;
  },

 



  get selectedAttributes()
  {
    var indices = this.selectedIndices;
    var attrs = [];
    for (var i = 0; i < indices.length; ++i) {
      attrs.push(new DOMAttribute(this.mDOMView.getNodeFromRowIndex(indices[i])));
    }
    return attrs;
  },

  
  

  

  get uid() { return "domNode" },
  get pane() { return this.mPanel },

  get selection() { return null },

  get subject() { return this.mSubject },
  set subject(aObject) 
  {
    
    
    viewer.pane.panelset.execCommand('cmdEditNodeValue');

    this.mSubject = aObject;
    var deck = document.getElementById("dkContent");

    switch (aObject.nodeType) {
      
      case Node.TEXT_NODE:
      case Node.CDATA_SECTION_NODE:
      case Node.COMMENT_NODE:
      case Node.PROCESSING_INSTRUCTION_NODE:
        deck.setAttribute("selectedIndex", 1);
        var txb = document.getElementById("txbTextNodeValue").value = 
                  aObject.nodeValue;
        break;
      
      
      default:
        var bundle = this.pane.panelset.stringBundle;
        deck.setAttribute("selectedIndex", 0);
        
        this.setTextValue("nodeName", aObject.nodeName);
        this.setTextValue("nodeType", bundle.getString(aObject.nodeType));
        this.setTextValue("namespace", aObject.namespaceURI);

        if (aObject != this.mDOMView.rootNode) {
          this.mDOMView.rootNode = aObject;
          this.mAttrTree.view.selection.select(-1);
        }
    }
    
    this.mObsMan.dispatchEvent("subjectChange", { subject: aObject });
  },

  

  initialize: function(aPane)
  {
    this.mPanel = aPane;
    aPane.notifyViewerReady(this);
  },

  destroy: function()
  {
    
    
    viewer.pane.panelset.execCommand('cmdEditNodeValue');
  },

  isCommandEnabled: function(aCommand)
  {
    switch (aCommand) {
      case "cmdEditPaste":
        var flavor = this.mPanel.panelset.clipboardFlavor;
        return (flavor == "inspector/dom-attribute" ||
                flavor == "inspector/dom-attributes");
      case "cmdEditInsert":
        return true;
      case "cmdEditCut":
      case "cmdEditCopy":
      case "cmdEditDelete":
        return this.selectedAttribute != null;
      case "cmdEditEdit":
        return this.mAttrTree.currentIndex >= 0 &&
                 this.mAttrTree.view.selection.count == 1;
      case "cmdEditNodeValue":
        
        if (this.subject) {
          
          if (this.subject.nodeType == Node.TEXT_NODE ||
              this.subject.nodeType == Node.CDATA_SECTION_NODE ||
              this.subject.nodeType == Node.COMMENT_NODE ||
              this.subject.nodeType == Node.PROCESSING_INSTRUCTION_NODE) {
            
            return this.subject.nodeValue != 
                   document.getElementById("txbTextNodeValue").value;
          }
        }
        return false;
    }
    return false;
  },
  
  getCommand: function(aCommand)
  {
    switch (aCommand) {
      case "cmdEditCut":
        return new cmdEditCut();
      case "cmdEditCopy":
        return new cmdEditCopy(this.selectedAttributes);
      case "cmdEditPaste":
        return new cmdEditPaste();
      case "cmdEditInsert":
        return new cmdEditInsert();
      case "cmdEditEdit":
        return new cmdEditEdit();
      case "cmdEditDelete":
        return new cmdEditDelete();
      case "cmdEditNodeValue":
        return new cmdEditNodeValue();
    }
    return null;
  },
  
  
  

  addObserver: function(aEvent, aObserver) { this.mObsMan.addObserver(aEvent, aObserver); },
  removeObserver: function(aEvent, aObserver) { this.mObsMan.removeObserver(aEvent, aObserver); },

  
  

  setTextValue: function(aName, aText)
  {
    var field = document.getElementById("tx_"+aName);
    if (field)
      field.value = aText;
  }
};




function cmdEditCut() {}
cmdEditCut.prototype =
{
  cmdCopy: null,
  cmdDelete: null,
  doCommand: function()
  {
    if (!this.cmdCopy) {
      this.cmdDelete = new cmdEditDelete();
      this.cmdCopy = new cmdEditCopy(viewer.selectedAttributes);
    }
    this.cmdCopy.doTransaction();
    this.cmdDelete.doCommand();    
  },

  undoCommand: function()
  {
    this.cmdDelete.undoCommand();    
  }
};

function cmdEditPaste() {}
cmdEditPaste.prototype =
{
  pastedAttr: null,
  previousAttrValue: null,
  subject: null,
  flavor: null,
  
  doCommand: function()
  {
    var subject, pastedAttr, flavor;
    if (this.subject) {
      subject = this.subject;
      pastedAttr = this.pastedAttr;
      flavor = this.flavor;
    } else {
      subject = viewer.subject;
      pastedAttr = viewer.pane.panelset.getClipboardData();
      flavor = viewer.pane.panelset.clipboardFlavor;
      this.pastedAttr = pastedAttr;
      this.subject = subject;
      this.flavor = flavor;
      if (flavor == "inspector/dom-attributes") {
        this.previousAttrValue = [];
        for (var i = 0; i < pastedAttr.length; ++i) {
          this.previousAttrValue[pastedAttr[i].node.nodeName] =
            viewer.subject.getAttribute(pastedAttr[i].node.nodeName);
        }
      } else if (flavor == "inspector/dom-attribute") {
        this.previousAttrValue =
          viewer.subject.getAttribute(pastedAttr.node.nodeName);
      }
    }
    
    if (subject && pastedAttr) {
      if (flavor == "inspector/dom-attributes") {
        for (var i = 0; i < pastedAttr.length; ++i) {
          subject.setAttribute(pastedAttr[i].node.nodeName,
                               pastedAttr[i].node.nodeValue);
        }
      } else if (flavor == "inspector/dom-attribute") {
        subject.setAttribute(pastedAttr.node.nodeName,
                             pastedAttr.node.nodeValue);
      }
    }
  },
  
  undoCommand: function()
  {
    if (this.pastedAttr) {
      if (this.flavor == "inspector/dom-attributes") {
        for (var i = 0; i < this.pastedAttr.length; ++i) {
          if (this.previousAttrValue[this.pastedAttr[i].node.nodeName])
            this.subject.setAttribute(this.pastedAttr[i].node.nodeName,
                      this.previousAttrValue[this.pastedAttr[i].node.nodeName]);
          else
            this.subject.removeAttribute(this.pastedAttr[i].node.nodeName);
        }
      } else if (this.flavor == "inspector/dom-attribute") {
        if (this.previousAttrValue)
          this.subject.setAttribute(this.pastedAttr.node.nodeName,
                                    this.previousAttrValue);
        else
          this.subject.removeAttribute(this.pastedAttr.node.nodeName);
      }
    }
  }
};

function cmdEditInsert() {}
cmdEditInsert.prototype =
{
  attr: null,
  subject: null,
  
  promptFor: function()
  {
    var bundle = viewer.pane.panelset.stringBundle;
    var title = bundle.getString("newAttribute.title");
    var doc = viewer.subject.ownerDocument;
    var out = { name: null, value: null, namespaceURI: null, accepted: false };

    window.openDialog("chrome://inspector/content/viewers/domNode/domNodeDialog.xul",
                      "insert", "chrome,modal,centerscreen", out, title, doc);

    this.subject = viewer.subject;
    if (out.accepted)
      this.subject.setAttributeNS(out.namespaceURI, out.name, out.value);
    
    this.attr = this.subject.getAttributeNode(out.name);
    return false;
  },
  
  doCommand: function()
  {
    if (!this.attr)
      return this.promptFor();
    
    this.subject.setAttributeNS(this.attr.namespaceURI,
                                this.attr.nodeName,
                                this.attr.nodeValue);
    return false;
  },
  
  undoCommand: function()
  {
    if (this.attr && this.subject == viewer.subject)
      this.subject.removeAttributeNS(this.attr.namespaceURI,
                                     this.attr.localName);
  }
};

function cmdEditDelete() {}
cmdEditDelete.prototype =
{
  attrs: null,
  subject: null,
  
  doCommand: function()
  {
    var attrs = this.attrs ? this.attrs : viewer.selectedAttributes;
    if (attrs) {
      this.attrs = attrs;
      this.subject = viewer.subject;
      for (var i = 0; i < this.attrs.length; ++i) {
        this.subject.removeAttribute(this.attrs[i].node.nodeName);
      }
    }
  },
  
  undoCommand: function()
  {
    if (this.attrs) {
      for (var i = 0; i < this.attrs.length; ++i) {
        this.subject.setAttribute(this.attrs[i].node.nodeName,
                                  this.attrs[i].node.nodeValue);
      }
    }
  }
};





function cmdEditEdit() {}
cmdEditEdit.prototype =
{
  attr: null,
  previousValue: null,
  newValue: null,
  previousNamespaceURI: null,
  newNamespaceURI: null,
  subject: null,
  
  promptFor: function()
  {
    var attr = viewer.selectedAttribute.node;
    if (attr) {
      var bundle = viewer.pane.panelset.stringBundle;
      var title = bundle.getString("editAttribute.title");
      var doc = attr.ownerDocument;
      var out = {
        name: attr.nodeName,
        value: attr.nodeValue,
        namespaceURI: attr.namespaceURI,
        accepted: false
      };

      window.openDialog("chrome://inspector/content/viewers/domNode/domNodeDialog.xul",
                        "edit", "chrome,modal,centerscreen", out, title, doc);

      if (out.accepted) {
        this.subject              = viewer.subject;
        this.newValue             = out.value;
        this.newNamespaceURI      = out.namespaceURI || null;
        this.previousValue        = attr.nodeValue;
        this.previousNamespaceURI = attr.namespaceURI;
        if (this.previousNamespaceURI == this.newNamespaceURI) {
          this.subject.setAttributeNS(this.previousNamespaceURI,
                                      attr.nodeName,
                                      out.value);
        } else {
          this.subject.removeAttributeNS(this.previousNamespaceURI,
                                         attr.localName);
          this.subject.setAttributeNS(out.namespaceURI,
                                      attr.nodeName,
                                      out.value);
        }
        this.attr = this.subject.getAttributeNode(attr.nodeName);
        return false;
      }
    }
    return true;
  },
  
  doCommand: function()
  {
    if (!this.attr)
      return this.promptFor();

    this.subject.removeAttributeNS(this.previousNamespaceURI,
                                   this.attr.localName);
    this.subject.setAttributeNS(this.newNamespaceURI,
                                this.attr.nodeName,
                                this.newValue);
    return false;
  },
  
  undoCommand: function()
  {
    if (this.attr) {
      if (this.previousNamespaceURI == this.newNamespaceURI) {
        this.subject.setAttributeNS(this.previousNamespaceURI,
                                    this.attr.nodeName,
                                    this.previousValue);
      } else {
        this.subject.removeAttributeNS(this.newNamespaceURI,
                                       this.attr.localName);
        this.subject.setAttributeNS(this.previousNamespaceURI,
                                    this.attr.nodeName,
                                    this.previousValue);
      }
    }
  }
};




function cmdEditNodeValue() {
  this.newValue = document.getElementById("txbTextNodeValue").value;
  this.subject = viewer.subject;
  this.previousValue = this.subject.nodeValue;
}
cmdEditNodeValue.prototype =
{
  
  txnType: "standard",
  
  
  QueryInterface: txnQueryInterface,
  merge: txnMerge,
  isTransient: false,

  doTransaction: function doTransaction()
  {
    this.subject.nodeValue = this.newValue;
  },
  
  undoTransaction: function undoTransaction()
  {
    this.subject.nodeValue = this.previousValue;
    this.refreshView();
  },

  redoTransaction: function redoTransaction()
  {
    this.doTransaction();
    this.refreshView();
  },

  refreshView: function refreshView() {
    
    if (viewer.subject == this.subject) {
      document.getElementById("txbTextNodeValue").value =
               this.subject.nodeValue;
    }
  }
};
