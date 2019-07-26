



function debug(msg) {
  Services.console.logStringMessage("SessionStoreContent: " + msg);
}





let EventListener = {

  init: function () {
    addEventListener("pageshow", this, true);
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "pageshow":
        if (event.persisted)
          sendAsyncMessage("SessionStore:pageshow");
        break;
      default:
        debug("received unknown event '" + event.type + "'");
        break;
    }
  }
};

EventListener.init();
