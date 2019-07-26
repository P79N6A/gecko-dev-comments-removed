




var SubscribeHandler = {
  


  _feedWriter: null,
  
  init: function SH_init() {
    this._feedWriter = new BrowserFeedWriter();
  },

  writeContent: function SH_writeContent() {
    this._feedWriter.writeContent();
  },

  uninit: function SH_uninit() {
    this._feedWriter.close();
  }
};
