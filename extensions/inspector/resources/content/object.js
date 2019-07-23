













































var inspector;





window.addEventListener("load", ObjectApp_initialize, false);
window.addEventListener("unload", ObjectApp_destroy, false);

function ObjectApp_initialize()
{
  inspector = new ObjectApp();
  inspector.initialize();
}

function ObjectApp_destroy()
{
  inspector.destroy();
}




function ObjectApp()
{
}

ObjectApp.prototype = 
{
  
  

  initialize: function()
  {
    this.mInitTarget = window.arguments && window.arguments.length > 0 ? window.arguments[0] : null;

    this.mPanelSet = document.getElementById("bxPanelSet");
    this.mPanelSet.addObserver("panelsetready", this, false);
    this.mPanelSet.initialize();
  },
  
  destroy: function()
  {
  },
  
  doViewerCommand: function(aCommand)
  {
    this.mPanelSet.execCommand(aCommand);
  },
  
  getViewer: function(aUID)
  {
    return this.mPanelSet.registry.getViewerByUID(aUID);
  },
  
  onEvent: function(aEvent)
  {
    switch (aEvent.type) {
      case "panelsetready":
        this.initViewerPanels();
    }
  },

  initViewerPanels: function()
  {
    if (this.mInitTarget)
      this.target = this.mInitTarget;
  },
  
  set target(aObj)
  {
    this.mPanelSet.getPanel(0).subject = aObj;
  }
};




