



function debug(msg) {
  Services.console.logStringMessage("SessionStoreContent: " + msg);
}





let EventListener = {

  DOM_EVENTS: [
    "pageshow", "change", "input"
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
      default:
        debug("received unknown event '" + event.type + "'");
        break;
    }
  }
};

EventListener.init();
