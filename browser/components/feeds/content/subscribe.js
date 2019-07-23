





































var SubscribeHandler = {
  


  _feedWriter: null,
  
  init: function SH_init() {
    this._feedWriter = new BrowserFeedWriter();
    this._feedWriter.init(window);
  },

  writeContent: function SH_writeContent() {
    this._feedWriter.writeContent();
  },

  uninit: function SH_uninit() {
    this._feedWriter.close();
  },
  
  subscribe: function FH_subscribe() {
    this._feedWriter.subscribe();
  }
};
