
































































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var dialog = {
  
  

  _handlerInfo: null,
  _URI: null,
  _itemChoose: null,
  _okButton: null,
  _windowCtxt: null,
  
  
  

 


  initialize: function initialize()
  {
    this._handlerInfo = window.arguments[7].QueryInterface(Ci.nsIHandlerInfo);
    this._URI         = window.arguments[8].QueryInterface(Ci.nsIURI);
    this._windowCtxt  = window.arguments[9];
    if (this._windowCtxt)
      this._windowCtxt.QueryInterface(Ci.nsIInterfaceRequestor);
    this._itemChoose  = document.getElementById("item-choose");
    this._okButton    = document.documentElement.getButton("accept");

    this.updateOKButton();

    var description = {
      image: document.getElementById("description-image"),
      text:  document.getElementById("description-text")
    };
    var options = document.getElementById("item-action-text");
    var checkbox = {
      desc: document.getElementById("remember"),
      text:  document.getElementById("remember-text")
    };

    
    document.title               = window.arguments[0];
    description.image.src        = window.arguments[1];
    description.text.textContent = window.arguments[2];
    options.value                = window.arguments[3];
    checkbox.desc.label          = window.arguments[4];
    checkbox.desc.accessKey      = window.arguments[5];
    checkbox.text.textContent    = window.arguments[6];

    
    if (!checkbox.desc.label)
      checkbox.desc.hidden = true;

    
    this.populateList();
  },

 


  populateList: function populateList()
  {
    var items = document.getElementById("items");
    var possibleHandlers = this._handlerInfo.possibleApplicationHandlers;
    var preferredHandler = this._handlerInfo.preferredApplicationHandler;
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    for (let i = possibleHandlers.length - 1; i >= 0; --i) {
      let app = possibleHandlers.queryElementAt(i, Ci.nsIHandlerApp);
      let elm = document.createElement("richlistitem");
      elm.setAttribute("type", "handler");
      elm.setAttribute("name", app.name);
      elm.obj = app;

      if (app instanceof Ci.nsILocalHandlerApp) {
        
        let uri = ios.newFileURI(app.executable);
        elm.setAttribute("image", "moz-icon://" + uri.spec + "?size=32");
      }
      else if (app instanceof Ci.nsIWebHandlerApp) {
        let uri = ios.newURI(app.uriTemplate, null, null);
        if (/^https?/.test(uri.scheme)) {
          
          
          
          
          
          
          elm.setAttribute("image", uri.prePath + "/favicon.ico");
        }
        elm.setAttribute("description", uri.prePath);
      }
      else if (app instanceof Ci.nsIDBusHandlerApp){
	  elm.setAttribute("description", app.method);  
      }
      else
        throw "unknown handler type";

      items.insertBefore(elm, this._itemChoose);
      if (preferredHandler && app == preferredHandler)
        this.selectedItem = elm;
    }

    if (this._handlerInfo.hasDefaultHandler) {
      let elm = document.createElement("richlistitem");
      elm.setAttribute("type", "handler");
      elm.id = "os-default-handler";
      elm.setAttribute("name", this._handlerInfo.defaultDescription);
    
      items.insertBefore(elm, items.firstChild);
      if (this._handlerInfo.preferredAction == 
          Ci.nsIHandlerInfo.useSystemDefault) 
          this.selectedItem = elm;
    }
    items.ensureSelectedElementIsVisible();
  },
  
 


  chooseApplication: function chooseApplication()
  {
    var bundle = document.getElementById("base-strings");
    var title = bundle.getString("choose.application.title");

    var fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, title, Ci.nsIFilePicker.modeOpen);
    fp.appendFilters(Ci.nsIFilePicker.filterApps);

    if (fp.show() == Ci.nsIFilePicker.returnOK && fp.file) {
      let uri = Cc["@mozilla.org/network/util;1"].
                getService(Ci.nsIIOService).
                newFileURI(fp.file);

      let handlerApp = Cc["@mozilla.org/uriloader/local-handler-app;1"].
                       createInstance(Ci.nsILocalHandlerApp);
      handlerApp.executable = fp.file;

      
      let parent = document.getElementById("items");
      for (let i = 0; i < parent.childNodes.length; ++i) {
        let elm = parent.childNodes[i];
        if (elm.obj instanceof Ci.nsILocalHandlerApp && elm.obj.equals(handlerApp)) {
          parent.selectedItem = elm;
          parent.ensureSelectedElementIsVisible();
          return;
        }
      }

      let elm = document.createElement("richlistitem");
      elm.setAttribute("type", "handler");
      elm.setAttribute("name", fp.file.leafName);
      elm.setAttribute("image", "moz-icon://" + uri.spec + "?size=32");
      elm.obj = handlerApp;

      parent.selectedItem = parent.insertBefore(elm, parent.firstChild);
      parent.ensureSelectedElementIsVisible();
    }
  },

 


  onAccept: function onAccept()
  {
    var checkbox = document.getElementById("remember");
    if (!checkbox.hidden) {
      
      if (this.selectedItem.obj) {
        
        this._handlerInfo.preferredAction = Ci.nsIHandlerInfo.useHelperApp;
        this._handlerInfo.preferredApplicationHandler = this.selectedItem.obj;
      }
      else
        this._handlerInfo.preferredAction = Ci.nsIHandlerInfo.useSystemDefault;
    }
    this._handlerInfo.alwaysAskBeforeHandling = !checkbox.checked;

    var hs = Cc["@mozilla.org/uriloader/handler-service;1"].
             getService(Ci.nsIHandlerService);
    hs.store(this._handlerInfo);

    this._handlerInfo.launchWithURI(this._URI, this._windowCtxt);

    return true;
  },

 


  updateOKButton: function updateOKButton()
  {
    this._okButton.disabled = this._itemChoose.selected;
  },

 


  onCheck: function onCheck()
  {
    if (document.getElementById("remember").checked)
      document.getElementById("remember-text").setAttribute("visible", "true");
    else
      document.getElementById("remember-text").removeAttribute("visible");
  },

  


  onDblClick: function onDblClick()
  {
    if (this.selectedItem == this._itemChoose)
      this.chooseApplication();
    else
      document.documentElement.acceptDialog();
  },

  
  

 


  get selectedItem()
  {
    return document.getElementById("items").selectedItem;
  },
  set selectedItem(aItem)
  {
    return document.getElementById("items").selectedItem = aItem;
  }
  
};
