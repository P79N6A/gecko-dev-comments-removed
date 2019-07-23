













































var inspector;



const kObserverServiceIID  = "@mozilla.org/observer-service;1";

const gNavigator = window._content;



function InspectorSidebar_initialize()
{
  inspector = new InspectorSidebar();
  inspector.initialize();
}

window.addEventListener("load", InspectorSidebar_initialize, false);




function InspectorSidebar()
{
}

InspectorSidebar.prototype = 
{
  
  

  get document() { return this.mDocPanel.viewer.subject; },

  initialize: function()
  {
    this.installNavObserver();

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

  
  
  
  initViewerPanels: function()
  {
    this.mDocPanel = this.mPanelSet.getPanel(0);
    this.mDocPanel.addObserver("subjectChange", this, false);
    this.mObjectPanel = this.mPanelSet.getPanel(1);
  },

  onEvent: function(aEvent)
  {
    if (aEvent.type == "panelsetready") {
      this.initViewerPanels();
    }
  },
  
  
  
  
  setTargetWindow: function(aWindow)
  {
    this.setTargetDocument(aWindow.document);
  },

  setTargetDocument: function(aDoc)
  {
    this.mPanelSet.getPanel(0).subject = aDoc;
  },

  installNavObserver: function()
  {
		var observerService = XPCU.getService(kObserverServiceIID, "nsIObserverService");
    observerService.addObserver(NavLoadObserver, "EndDocumentLoad", false);
  }
};

var NavLoadObserver = {
  observe: function(aWindow)
  {
    inspector.setTargetWindow(aWindow);
  }
};
