







const BrowserTouchHandler = {
  _debugEvents: false,

  init: function init() {
    
    document.addEventListener("PopupChanged", this, false);
    document.addEventListener("CancelTouchSequence", this, false);

    
    messageManager.addMessageListener("Content:ContextMenu", this);
  },

  
  onContentContextMenu: function onContentContextMenu(aMessage) {
    
    
    
    if (!InputSourceHelper.isPrecise &&
        !SelectionHelperUI.isActive &&
        SelectionHelperUI.canHandleContextMenuMsg(aMessage)) {
      SelectionHelperUI.openEditSession(aMessage.target,
                                        aMessage.json.xPos,
                                        aMessage.json.yPos);
      return;
    }

    
    
    let contextInfo = { name: aMessage.name,
                        json: aMessage.json,
                        target: aMessage.target };
    if (ContextMenuUI.showContextMenu(contextInfo)) {
      let event = document.createEvent("Events");
      event.initEvent("CancelTouchSequence", true, false);
      document.dispatchEvent(event);
    } else {
      
      
      let event = document.createEvent("Events");
      event.initEvent("MozEdgeUIGesture", true, false);
      window.dispatchEvent(event);
    }
  },

  



  handleEvent: function handleEvent(aEvent) {
    
    if (this._debugEvents)
      Util.dumpLn("BrowserTouchHandler:", aEvent.type);

    switch (aEvent.type) {
      case "PopupChanged":
      case "CancelTouchSequence":
        if (!aEvent.detail)
          ContextMenuUI.reset();
        break;
    }
  },

  receiveMessage: function receiveMessage(aMessage) {
    if (this._debugEvents) Util.dumpLn("BrowserTouchHandler:", aMessage.name);
    switch (aMessage.name) {
      
      case "Content:ContextMenu":
        this.onContentContextMenu(aMessage);
        break;
    }
  },
};
