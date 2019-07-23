








































var dialog;




window.addEventListener("load", InsertDialog_initialize, false);

function InsertDialog_initialize()
{
  dialog = new InsertDialog();
  dialog.initialize();
}




function InsertDialog()
{
  this.mDoc  = window.arguments[0];
  this.mData = window.arguments[1];

  this.nodeType  = document.getElementById("ml_nodeType");
  this.tagName = document.getElementById("tx_tagName");
  this.nodeValue = document.getElementById("tx_nodeValue");
  this.namespace = document.getElementById("tx_namespace");
  this.menulist  = document.getElementById("ml_namespace");
}

InsertDialog.prototype =
{
 


  initialize: function initialize()
  {
    var menuitems  = this.menulist.firstChild.childNodes;
    var defaultNS  = document.getElementById("mi_namespace");
    var customNS   = document.getElementById("mi_custom");
    var accept     = document.documentElement.getButton("accept");

    this.menulist.disabled = !this.enableNamespaces();
    defaultNS.value        = this.mDoc.documentElement.namespaceURI;

    this.toggleNamespace();
    this.updateType();
  },

 


  accept: function accept()
  {
    switch (this.nodeType.value)
    {
      case "element":
      	this.mData.type     = Components.interfaces.nsIDOMNode.ELEMENT_NODE;
        this.mData.value    = this.tagName.value;
        break;
      case "text":
      	this.mData.type     = Components.interfaces.nsIDOMNode.TEXT_NODE;
        this.mData.value    = this.nodeValue.value;
        break;
    }
    this.mData.namespaceURI = this.namespace.value;
    this.mData.accepted     = true;
    return true;
  },

 


  updateType: function updateType()
  {
    switch (dialog.nodeType.value)
    {
      case "text":
        document.getElementById("row_text").hidden = false;
        document.getElementById("row_element").hidden = true;
        break;
      case "element":
        document.getElementById("row_text").hidden = true;
        document.getElementById("row_element").hidden = false;
        break;
    }
    dialog.toggleAccept();
  },

 


  toggleNamespace: function toggleNamespace()
  {
    dialog.namespace.disabled = dialog.menulist.selectedItem.id != "mi_custom";
    dialog.namespace.value    = dialog.menulist.value;
  },

 





  enableNamespaces: function enableNamespaces()
  {
    return this.mDoc.contentType != "text/html";
  },

 



  toggleAccept: function toggleAccept()
  {
    document.documentElement.getButton("accept").disabled = 
      (dialog.tagName.value == "") && (dialog.nodeType.selectedItem.value == "element");
  }
};
