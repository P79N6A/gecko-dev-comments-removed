








































 
var nsClipboard = {
  _CB: null,
  get mClipboard()
    {
      if (!this._CB) 
        {
          const kCBContractID = "@mozilla.org/widget/clipboard;1";
          const kCBIID = Components.interfaces.nsIClipboard;
          this._CB = Components.classes[kCBContractID].getService(kCBIID);
        }
      return this._CB;
    },
    
  currentClipboard: null,
  











  read: function (aFlavourList, aClipboard, aAnyFlag)
    {
      this.currentClipboard = aClipboard;
      var data = nsTransferable.get(aFlavourList, this.getClipboardTransferable, aAnyFlag);
      return data.first.first;  
    },
    
  







  getClipboardTransferable: function (aFlavourList)
    {
      const supportsContractID = "@mozilla.org/supports-array;1";
      const supportsIID = Components.interfaces.nsISupportsArray;
      var supportsArray = Components.classes[supportsContractID].createInstance(supportsIID);
      var trans = nsTransferable.createTransferable();
      for (var flavour in aFlavourList) 
        trans.addDataFlavor(flavour);
      nsClipboard.mClipboard.getData(trans, nsClipboard.currentClipboard)
      supportsArray.AppendElement(trans);
      return supportsArray;
    }
};

