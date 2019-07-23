














































var FrameExchange = {
  mData: {},

  loadURL: function(aFrame, aURL, aData)
  {
    this.mData[aURL] = aData;
    aFrame.setAttribute("src", aURL);
  },

  receiveData: function(aWindow)
  {
    var id = aWindow.location.href;
    var data = this.mData[id];
    this.mData[id] = null;

    return data;
  }
};


