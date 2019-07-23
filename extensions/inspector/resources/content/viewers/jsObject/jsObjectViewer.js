














































const kClipboardHelperCID  = "@mozilla.org/widget/clipboardhelper;1";




function JSObjectViewer()
{
  this.mObsMan = new ObserverManager(this);
}

JSObjectViewer.prototype = 
{
  
  
  
  mSubject: null,
  mPane: null,

  
  

  get uid() { return "jsObject" },
  get pane() { return this.mPane },

  get selection() { return this.mSelection },
  
  get subject() { return this.mSubject },
  set subject(aObject) 
  {
    this.mSubject = aObject;
    this.emptyTree(this.mTreeKids);
    var ti = this.addTreeItem(this.mTreeKids, bundle.getString("root.title"), aObject, aObject);
    ti.setAttribute("open", "true");

    this.mObsMan.dispatchEvent("subjectChange", { subject: aObject });
  },

  initialize: function(aPane)
  {
    this.mPane = aPane;
    this.mTree = document.getElementById("treeJSObject");
    this.mTreeKids = document.getElementById("trchJSObject");
    
    aPane.notifyViewerReady(this);
  },

  destroy: function()
  {
  },
  
  isCommandEnabled: function(aCommand)
  {
    return false;
  },
  
  getCommand: function(aCommand)
  {
    return null;
  },
  
  
  

  addObserver: function(aEvent, aObserver) { this.mObsMan.addObserver(aEvent, aObserver); },
  removeObserver: function(aEvent, aObserver) { this.mObsMan.removeObserver(aEvent, aObserver); },

  
  

  cmdCopyValue: function()
  {
    var sel = getSelectedItem();
    if (sel) {
      var val = sel.__JSValue__;
      if (val) {
        var helper = XPCU.getService(kClipboardHelperCID, "nsIClipboardHelper");
        helper.copyString(val);
      }
    }
  },
  
  cmdEvalExpr: function()
  {
    var sel = getSelectedItem();
    if (sel) {
      var win = openDialog("chrome://inspector/content/viewers/jsObject/evalExprDialog.xul", 
                           "_blank", "chrome", this, sel);
    }
  },  
  
  doEvalExpr: function(aExpr, aItem, aNewView)
  {
    
    
    
    try {
      var f = Function("target", aExpr);
      var result = f(aItem.__JSValue__);
      
      if (result) {
        if (aNewView) {
          inspectObject(result);
        } else {
          this.subject = result;
        }
      }
    } catch (ex) {
      dump("Error in expression.\n");
      throw (ex);
    }
  },  
  
  cmdInspectInNewView: function()
  {
    var sel = getSelectedItem();
    if (sel)
      inspectObject(sel.__JSValue__);
  },
  
  
  

  emptyTree: function(aTreeKids)
  {
    while (aTreeKids.hasChildNodes()) {
      aTreeKids.removeChild(aTreeKids.lastChild);
    }
  },
  
  buildPropertyTree: function(aTreeChildren, aObject)
  {
    
    var propertyNames = [];
    for (var prop in aObject) {
      propertyNames.push(prop);
    }


   








    function sortNumeric(a, b) {
      if (isNaN(a))
        return isNaN(b) ? 0 : 1;
      if (isNaN(b))
        return -1;
      return a - b;
    }

   











    function sortFunction(a, b) {
      
      var aIsConstant = a == a.toUpperCase() && isNaN(a);
      var bIsConstant = b == b.toUpperCase() && isNaN(b);
      
      if (aIsConstant) {
        if (bIsConstant) {
          
          return sortNumeric(aObject[a], aObject[b]) || a.localeCompare(b);
        }
        
        return -1;
      }
      if (bIsConstant)
        
        return 1;
      
      
      return sortNumeric(a, b) || a.localeCompare(b);
    }
    propertyNames.sort(sortFunction);

    
    for (var i = 0; i < propertyNames.length; i++) {
      try {
        this.addTreeItem(aTreeChildren, propertyNames[i],
                         aObject[propertyNames[i]], aObject);
      } catch (ex) {
        
      }
    }
  },
  
  addTreeItem: function(aTreeChildren, aName, aValue, aObject)
  {
    var ti = document.createElement("treeitem");
    ti.__JSObject__ = aObject;
    ti.__JSValue__ = aValue;
    
    var value;
    if (aValue === null) {
      value = "(null)";
    } else if (aValue === undefined) {
      value = "(undefined)";
    } else {
      try {
        value = aValue.toString();
        value = value.replace(/\n|\r|\t|\v/g, " ");
      } catch (ex) {
        value = "";
      }
    }
    
    ti.setAttribute("typeOf", typeof(aValue));

    if (typeof(aValue) == "object" && aValue !== null) {
      ti.setAttribute("container", "true");
    } else if (typeof(aValue) == "string")
      value = "\"" + value + "\"";
    
    var tr = document.createElement("treerow");
    ti.appendChild(tr);
    
    var tc = document.createElement("treecell");
    tc.setAttribute("label", aName);
    tr.appendChild(tc);
    tc = document.createElement("treecell");
    tc.setAttribute("label", value);
    if (aValue === null) {
      tc.setAttribute("class", "inspector-null-value-treecell");
    }
    tr.appendChild(tc);
    
    aTreeChildren.appendChild(ti);

    
    this.mTreeKids.addEventListener("DOMAttrModified", onTreeItemAttrModified, false);
    
    return ti;
  },
  
  openTreeItem: function(aItem)
  {
    var treechildren = aItem.getElementsByTagName("treechildren").item(0);
    if (!treechildren) {
      treechildren = document.createElement("treechildren");
      this.buildPropertyTree(treechildren, aItem.__JSValue__);
      aItem.appendChild(treechildren);
    }
  },
  
  onCreateContext: function(aPopup)
  {
  }
  
};

function onTreeItemAttrModified(aEvent)
{
  if (aEvent.attrName == "open")
    viewer.openTreeItem(aEvent.target);
}

function getSelectedItem()
{
  var tree = document.getElementById("treeJSObject");
  if (tree.view.selection.count)
    return tree.contentView.getItemAtIndex(tree.currentIndex);
  else 
    return null;    
}

function toggleItem(aItem)
{
  var tree = document.getElementById("treeJSObject");
  var row = tree.currentView.getIndexOfItem(aItem);
  if (row >= 0) {
    tree.view.toggleOpenState(row);
  }
}
