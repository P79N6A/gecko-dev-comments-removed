



function debug(msg) {
  Services.console.logStringMessage("SessionStoreContent: " + msg);
}





let EventListener = {

  DOM_EVENTS: [
    "pageshow", "change", "input", "MozStorageChanged"
  ],

  init: function () {
    this.DOM_EVENTS.forEach(e => addEventListener(e, this, true));
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "pageshow":
        if (event.persisted)
          sendAsyncMessage("SessionStore:pageshow");
        break;
      case "input":
      case "change":
        sendAsyncMessage("SessionStore:input");
        break;
      case "MozStorageChanged":
        {
          let isSessionStorage = true;
          
          try {
            if (event.storageArea != content.sessionStorage) {
              isSessionStorage = false;
            }
          } catch (ex) {
            
            
            isSessionStorage = false;
          }
          if (isSessionStorage) {
            sendAsyncMessage("SessionStore:MozStorageChanged");
          }
          break;
        }
      default:
        debug("received unknown event '" + event.type + "'");
        break;
    }
  }
};

EventListener.init();
