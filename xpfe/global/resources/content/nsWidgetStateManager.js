











































var wsm;

function nsWidgetStateManager (aFrameID)
{

  this.dataManager = {
    


    pageData: { },

    setPageData: function (aPageTag, aDataObject)
    {
      this.pageData[aPageTag] = aDataObject;
    },

    getPageData: function (aPageTag)
    {
      if (!(aPageTag in this.pageData))
        this.pageData[aPageTag] = { };
      return this.pageData[aPageTag];
    },

    setItemData: function (aPageTag, aItemID, aDataObject)
    {
      if (!(aPageTag in this.pageData))
        this.pageData[aPageTag] = new Object();
      
      this.pageData[aPageTag][aItemID] = aDataObject;
    },

    getItemData: function (aPageTag, aItemID)
    {
      if (!(aItemID in this.pageData[aPageTag]))
        this.pageData[aPageTag][aItemID] = new Object();
      return this.pageData[aPageTag][aItemID];
    }
  }

  this.contentID    = aFrameID;

  wsm               = this;

  



  this.handlers     = {
    colorpicker:
      {  get: wsm.get_Colorpicker, set: wsm.set_Colorpicker   },
    menulist:
      {  get: wsm.get_Menulist,    set: wsm.set_Menulist      },
    radiogroup:
      {  get: wsm.get_Radiogroup,  set: wsm.set_Radiogroup    },
    checkbox:
      {  get: wsm.get_Checkbox,    set: wsm.set_Checkbox      },
    textbox:
      {  get: wsm.get_Textbox,     set: wsm.set_Textbox       },
    listitem:
      {  get: wsm.get_Listitem,    set: wsm.set_Listitem      },
    data:
      {  get: wsm.get_Data,        set: wsm.set_Data          },
    default_handler:
      {  get: wsm.get_Default,     set: wsm.set_Default       }
  }

  
  this.attributes   = [];
}

nsWidgetStateManager.prototype =
{
  get contentArea ()
  {
    return window.frames[this.contentID];
  },

  savePageData: function (aPageTag)
  {
    if (!(aPageTag in this.dataManager.pageData))
      return;

    
    
    
    
    
    var elements;
    if ("_elementIDs" in this.contentArea) {
      elements = [];
      for (var i = 0; i < this.contentArea._elementIDs.length; i++) {
        var elt = this.contentArea.document.getElementById(this.contentArea._elementIDs[i]);
        if (elt)
          elements[elements.length] = elt;
        else {
          
          dump("*** FIX ME: '_elementIDs' in '" + this.contentArea.location.href.split('/').pop() +
               "' contains a reference to a non-existent element ID '" +
          this.contentArea._elementIDs[i] + "'.\n");
        }
      }
    }
    else 
      elements = this.contentArea.document.getElementsByAttribute("wsm_persist", "true");

    for (var ii = 0; ii < elements.length; ii++) {
      var elementID   = elements[ii].id;
      var elementType = elements[ii].localName;

      if (!(aPageTag in this.dataManager.pageData))
        this.dataManager.pageData[aPageTag] = [];
      this.dataManager.pageData[aPageTag][elementID] = [];

      
      var get_Func = (elementType in this.handlers) ?
      this.handlers[elementType].get :
      this.handlers.default_handler.get;
      this.dataManager.setItemData(aPageTag, elementID, get_Func(elementID));
    }

    if ("GetFields" in this.contentArea) {
      
      var dataObject = this.dataManager.getPageData(aPageTag);
      dataObject = this.contentArea.GetFields(dataObject);
      if (dataObject)        
        this.dataManager.setPageData(aPageTag, dataObject);
    }
  },

  setPageData: function (aPageTag)
  {
    var pageData = this.dataManager.getPageData(aPageTag);
    if ("SetFields" in this.contentArea) {
      if (!this.contentArea.SetFields(pageData))
      {
        
        
        return;
      }
    }

    for (var elementID in pageData) {
      var element = this.contentArea.document.getElementById(elementID);
      if (element) {
        var elementType = element.localName;
        var set_Func = (elementType in this.handlers) ?
          this.handlers[elementType].set :
          this.handlers.default_handler.set;
        set_Func(elementID, pageData[elementID]);
      }
    }
  },


  


  generic_Set: function (aElement, aDataObject)
  {
    if (aElement) {
      for (var property in aDataObject) {
        if (property == "localname")
          continue;
        if (!aDataObject[property] && typeof aDataObject[property] == "boolean")
          aElement.removeAttribute(property);
        else
          aElement.setAttribute(property, aDataObject[property]);
      }
      
      if (!aElement.getAttribute("disabled","true"))
        aElement.removeAttribute("disabled");
    }
  },

  generic_Get: function (aElement)
  {
    if (aElement) {
      var dataObject = new Object();
      var wsmAttributes = aElement.getAttribute("wsm_attributes");
      var attributes = wsm.attributes;              
      if (wsmAttributes != "")
        attributes.push(wsmAttributes.split(" "));  

      for (var i = 0; i < attributes.length; i++)
        dataObject[attributes[i]] = aElement.getAttribute(attributes[i]);

      dataObject.localname = aElement.localName;
      return dataObject;
    }
    return null;
  },

  
  set_Colorpicker: function (aElementID, aDataObject)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    
    wsm.generic_Set(element, aDataObject);
    
    if ('color' in aDataObject) {
      try {
        element.color = aDataObject.color;
      }
      catch (ex) {
        dump(aElementID +", ex: " + ex + "\n");
      }
    }
  },

  get_Colorpicker: function (aElementID)
  {
    var element     = wsm.contentArea.document.getElementById(aElementID);
    
    var dataObject  = wsm.generic_Get(element);
    
    if (dataObject) {
      dataObject.color = element.color;
      return dataObject;
    }
    return null;
  },

  
  set_Menulist: function (aElementID, aDataObject)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    
    wsm.generic_Set(element, aDataObject);
    
    if ("value" in aDataObject) {
      try {
        element.value = aDataObject.value;
      }
      catch (ex) {
        dump(aElementID + ", ex: " + ex + "\n");
      }
    }
  },

  get_Menulist: function (aElementID)
  {
    var element     = wsm.contentArea.document.getElementById(aElementID);
    
    var dataObject  = wsm.generic_Get(element);
    
    if (dataObject) {
      dataObject.value = element.getAttribute("value");
      return dataObject;
    }
    return null;
  },

  
  set_Radiogroup: function (aElementID, aDataObject)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    wsm.generic_Set(element, aDataObject);
    if ("value" in aDataObject)
      element.value = aDataObject.value;
    if ("disabled" in aDataObject)
      element.disabled = aDataObject.disabled;
  },

  get_Radiogroup: function (aElementID)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    var dataObject = wsm.generic_Get(element);
    if (dataObject) {
      dataObject.value = element.getAttribute("value");
      return dataObject;
    }
    return null;
  },

  
  set_Textbox: function (aElementID, aDataObject)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    wsm.generic_Set(element, aDataObject);
  },

  get_Textbox: function (aElementID)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    var dataObject = wsm.generic_Get(element);
    if (dataObject) {
      dataObject.value = element.value;
      return dataObject;
    }
    return null;
  },

  
  set_Checkbox: function (aElementID, aDataObject)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    
    wsm.generic_Set(element, aDataObject);
    
    if ("checked" in aDataObject && element.hasAttribute("reversed"))
      element.checked = !aDataObject.checked; 
  },

  get_Checkbox: function (aElementID)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    var dataObject = wsm.generic_Get(element);
    if (dataObject) {
      var checked = element.checked;
      dataObject.checked = element.getAttribute("reversed") == "true" ? !checked : checked;
      return dataObject;
    }
    return null;
  },

  
  set_Listitem: function (aElementID, aDataObject)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    wsm.generic_Set(element, aDataObject);
    
    if ("checked" in aDataObject && element.hasAttribute("reversed"))
      element.checked = !aDataObject.checked; 
  },

  get_Listitem: function (aElementID)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    var dataObject = wsm.generic_Get(element);
    if (dataObject) {
      if (element.getAttribute("type") == "checkbox") {
        var checked = element.checked;
        dataObject.checked = element.getAttribute("reversed") == "true" ? !checked : checked;
      }
      return dataObject;
    }
    return null;
  },

  
  set_Data: function (aElementID, aDataObject)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    wsm.generic_Set(element, aDataObject);
    if ("value" in aDataObject)
      element.setAttribute("value", aDataObject.value);
  },

  get_Data: function (aElementID)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    var dataObject = wsm.generic_Get(element);
    if (dataObject) {
      dataObject.value = element.getAttribute("value");
      return dataObject;
    }
    return null;
  },

  
  set_Default: function (aElementID, aDataObject)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    wsm.generic_Set(element, aDataObject);
  },

  get_Default: function (aElementID)
  {
    var element = wsm.contentArea.document.getElementById(aElementID);
    var dataObject = wsm.generic_Get(element);
    return dataObject ? dataObject : null;
  }
}








