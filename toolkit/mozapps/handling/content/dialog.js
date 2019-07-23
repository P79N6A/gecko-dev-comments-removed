


























































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var dialog = {
  
  

  _handlerInfo: null,
  _URI: null,
  _itemChoose: null,
  _okButton: null,

  
  

 


  initialize: function initialize()
  {
    this._handlerInfo = window.arguments[6].QueryInterface(Ci.nsIHandlerInfo);
    this._URI         = window.arguments[7].QueryInterface(Ci.nsIURI);
    this._itemChoose  = document.getElementById("item-choose");
    this._okButton    = document.documentElement.getButton("accept");

    this.updateOKButton();

    let description = {
      image: document.getElementById("description-image"),
      text:  document.getElementById("description-text")
    };
    let options = document.getElementById("item-action-text");
    let checkbox = {
      desc: document.getElementById("remember"),
      text:  document.getElementById("remember-text")
    };

    
    document.title               = window.arguments[0];
    description.image.src        = window.arguments[1];
    description.text.textContent = window.arguments[2];
    options.value                = window.arguments[3];
    checkbox.desc.label          = window.arguments[4];
    checkbox.text.textContent    = window.arguments[5];

    
    if (!checkbox.desc.label)
      checkbox.desc.hidden = true;

    
    this.populateList();
  },

 


  populateList: function populateList()
  {
    
    let app = this._handlerInfo.preferredApplicationHandler;

    if (app) {
      let elm = document.createElement("richlistitem");
      elm.setAttribute("type", "handler");
      elm.setAttribute("name", app.name);
      elm.obj = app;

      document.getElementById("items").insertBefore(elm, this._itemChoose);
    }

    if (this._handlerInfo.hasDefaultHandler) {
      let elm = document.createElement("richlistitem");
      elm.setAttribute("type", "handler");
      elm.id = "os-default-handler";
      elm.setAttribute("name", this._handlerInfo.defaultDescription);
    
      document.getElementById("items").insertBefore(elm, this._itemChoose);
    }
  },
  
 


  chooseApplication: function chooseApplication()
  {
    let bundle = document.getElementById("base-strings");
    let title = bundle.getString("choose.application.title");

    let fp = Cc["@mozilla.org/filepicker;1"].createInstance(Ci.nsIFilePicker);
    fp.init(window, title, Ci.nsIFilePicker.modeOpen);
    fp.appendFilters(Ci.nsIFilePicker.filterApps);

    if (fp.show() == Ci.nsIFilePicker.returnOK && fp.file) {
      let uri = Cc["@mozilla.org/network/util;1"].
                getService(Ci.nsIIOService).
                newFileURI(fp.file);
      let elm = document.createElement("richlistitem");
      elm.setAttribute("type", "handler");
      elm.setAttribute("name", fp.file.leafName);
      elm.setAttribute("image", "moz-icon://" + uri.spec + "?size=32");

      let handlerApp = Cc["@mozilla.org/uriloader/local-handler-app;1"].
                       createInstance(Ci.nsILocalHandlerApp);
      handlerApp.executable = fp.file;
      elm.obj = handlerApp;

      let parent = document.getElementById("items");
      parent.selectedItem = parent.insertBefore(elm, parent.firstChild);
      parent.ensureSelectedElementIsVisible();
    }
  },

 


  onAccept: function onAccept()
  {
    let checkbox = document.getElementById("remember");
    if (!checkbox.hidden) {
      
      if (this.selectedItem.obj) {
        
        this._handlerInfo.preferredAction = Ci.nsIHandlerInfo.useHelperApp;
        this._handlerInfo.preferredApplicationHandler = this.selectedItem.obj;
      }
      else
        this._handlerInfo.preferredAction = Ci.nsIHandlerInfo.useSystemDefault;
    }
    this._handlerInfo.alwaysAskBeforeHandling = !checkbox.checked;

    let hs = Cc["@mozilla.org/uriloader/handler-service;1"].
             getService(Ci.nsIHandlerService);
    hs.store(this._handlerInfo);

    this._handlerInfo.launchWithURI(this._URI);

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

  
  

 


  get selectedItem()
  {
    return document.getElementById("items").selectedItem;
  }
};

