# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <ben@netscape.com> (Original Author)
#   Pierre Chanial <pierrechanial@netscape.net>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****






 
 
var nsTransferable = {
  






 
  set: function (aTransferDataSet)
    {
      var trans = this.createTransferable();
      for (var i = 0; i < aTransferDataSet.dataList.length; ++i) 
        {
          var currData = aTransferDataSet.dataList[i];
          var currFlavour = currData.flavour.contentType;
          trans.addDataFlavor(currFlavour);
          var supports = null; 
          var length = 0;
          if (currData.flavour.dataIIDKey == "nsISupportsString")
            {
              supports = Components.classes["@mozilla.org/supports-string;1"]
                                   .createInstance(Components.interfaces.nsISupportsString);

              supports.data = currData.supports;
              length = supports.data.length;
            }
          else 
            {
              
              supports = currData.supports;
              length = 0; 
            }
          trans.setTransferData(currFlavour, supports, length * 2);
        }
      return trans;
    },
  
  
















  get: function (aFlavourSet, aRetrievalFunc, aAnyFlag)
    {
      if (!aRetrievalFunc) 
        throw "No data retrieval handler provided!";
      
      var supportsArray = aRetrievalFunc(aFlavourSet);
      var dataArray = [];
      var count = supportsArray.Count();
      
      
      
      
      for (var i = 0; i < count; i++)
        {
          var trans = supportsArray.GetElementAt(i);
          if (!trans) continue;
          trans = trans.QueryInterface(Components.interfaces.nsITransferable);
            
          var data = { };
          var length = { };
          
          var currData = null;
          if (aAnyFlag)
            { 
              var flavour = { };
              trans.getAnyTransferData(flavour, data, length);
              if (data && flavour)
                {
                  var selectedFlavour = aFlavourSet.flavourTable[flavour.value];
                  if (selectedFlavour) 
                    dataArray[i] = FlavourToXfer(data.value, length.value, selectedFlavour);
                }
            }
          else
            {
              var firstFlavour = aFlavourSet.flavours[0];
              trans.getTransferData(firstFlavour, data, length);
              if (data && firstFlavour)
                dataArray[i] = FlavourToXfer(data.value, length.value, firstFlavour);
            }
        }
      return new TransferDataSet(dataArray);
    },

  



    
  createTransferable: function ()
    {
      const kXferableContractID = "@mozilla.org/widget/transferable;1";
      const kXferableIID = Components.interfaces.nsITransferable;
      return Components.classes[kXferableContractID].createInstance(kXferableIID);
    }
};  











function FlavourSet(aFlavourList)
{
  this.flavours = aFlavourList || [];
  this.flavourTable = { };

  this._XferID = "FlavourSet";
  
  for (var i = 0; i < this.flavours.length; ++i)
    this.flavourTable[this.flavours[i].contentType] = this.flavours[i];
}

FlavourSet.prototype = {
  appendFlavour: function (aFlavour, aFlavourIIDKey)
  {
    var flavour = new Flavour (aFlavour, aFlavourIIDKey);
    this.flavours.push(flavour);
    this.flavourTable[flavour.contentType] = flavour;
  }
};







 
function Flavour(aContentType, aDataIIDKey)
{
  this.contentType = aContentType;
  this.dataIIDKey = aDataIIDKey || "nsISupportsString";

  this._XferID = "Flavour";
}

function TransferDataBase() {}
TransferDataBase.prototype = {
  push: function (aItems)
  {
    this.dataList.push(aItems);
  },

  get first ()
  {
    return "dataList" in this && this.dataList.length ? this.dataList[0] : null;
  }
};





function TransferDataSet(aTransferDataList)
{
  this.dataList = aTransferDataList || [];

  this._XferID = "TransferDataSet";
}
TransferDataSet.prototype = TransferDataBase.prototype;





function TransferData(aFlavourDataList)
{
  this.dataList = aFlavourDataList || [];

  this._XferID = "TransferData";
}
TransferData.prototype = {
  __proto__: TransferDataBase.prototype,
  
  addDataForFlavour: function (aFlavourString, aData, aLength, aDataIIDKey)
  {
    this.dataList.push(new FlavourData(aData, aLength, 
                       new Flavour(aFlavourString, aDataIIDKey)));
  }
};










function FlavourData(aData, aLength, aFlavour) 
{
  this.supports = aData;
  this.contentLength = aLength;
  this.flavour = aFlavour || null;
  
  this._XferID = "FlavourData";
}

FlavourData.prototype = {
  get data ()
  {
    if (this.flavour && 
        this.flavour.dataIIDKey != "nsISupportsString" )
      return this.supports.QueryInterface(Components.interfaces[this.flavour.dataIIDKey]); 
    else {
      var unicode = this.supports.QueryInterface(Components.interfaces.nsISupportsString);
      if (unicode) 
        return unicode.data.substring(0, this.contentLength/2);
     
      return this.supports;
    }
    return "";
  }
}





function FlavourToXfer(aData, aLength, aFlavour) 
{
  return new TransferData([new FlavourData(aData, aLength, aFlavour)]);
}

var transferUtils = {

  retrieveURLFromData: function (aData, flavour)
  {
    switch (flavour) {
      case "text/unicode":
        return aData.replace(/^\s+|\s+$/g, "");
      case "text/x-moz-url":
        return aData.toString().split("\n")[0];
      case "application/x-moz-file":
        var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                                  .getService(Components.interfaces.nsIIOService);
        var fileHandler = ioService.getProtocolHandler("file")
                                   .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
        return fileHandler.getURLSpecFromFile(aData);
    }
    return null;                                                   
  }

}
























   

var nsDragAndDrop = {
  
  _mDS: null,
  get mDragService()
    {
      if (!this._mDS) 
        {
          const kDSContractID = "@mozilla.org/widget/dragservice;1";
          const kDSIID = Components.interfaces.nsIDragService;
          this._mDS = Components.classes[kDSContractID].getService(kDSIID);
        }
      return this._mDS;
    },

  









  
  startDrag: function (aEvent, aDragDropObserver)
    {
      if (!("onDragStart" in aDragDropObserver))
        return;

      const kDSIID = Components.interfaces.nsIDragService;
      var dragAction = { action: kDSIID.DRAGDROP_ACTION_COPY + kDSIID.DRAGDROP_ACTION_MOVE + kDSIID.DRAGDROP_ACTION_LINK };

      var transferData = { data: null };
      try 
        {
          aDragDropObserver.onDragStart(aEvent, transferData, dragAction);
        }
      catch (e) 
        {
          return;  
        }

      if (!transferData.data) return;
      transferData = transferData.data;
      
      var transArray = Components.classes["@mozilla.org/supports-array;1"]
                                 .createInstance(Components.interfaces.nsISupportsArray);

      var region = null;
      if (aEvent.originalTarget.localName == "treechildren") {
        
        var tree = aEvent.originalTarget.parentNode;
        try {
          region = Components.classes["@mozilla.org/gfx/region;1"].createInstance(Components.interfaces.nsIScriptableRegion);
          region.init();
          var obo = tree.treeBoxObject;
          var bo = obo.treeBody.boxObject;
          var sel= obo.view.selection;

          var rowX = bo.x;
          var rowY = bo.y;
          var rowHeight = obo.rowHeight;
          var rowWidth = bo.width;

          
          for (var i = obo.getFirstVisibleRow(); i <= obo.getLastVisibleRow(); i ++)
          {
            if (sel.isSelected(i))
              region.unionRect(rowX, rowY, rowWidth, rowHeight);
            rowY = rowY + rowHeight;
          }
      
          
          region.intersectRect(bo.x, bo.y, bo.width, bo.height);
        } catch(ex) {
          dump("Error while building selection region: " + ex + "\n");
          region = null;
        }
      }

      var count = 0;
      do 
        {
          var trans = nsTransferable.set(transferData._XferID == "TransferData" 
                                         ? transferData 
                                         : transferData.dataList[count++]);
          transArray.AppendElement(trans.QueryInterface(Components.interfaces.nsISupports));
        }
      while (transferData._XferID == "TransferDataSet" && 
             count < transferData.dataList.length);
      
      try {
        this.mDragService.invokeDragSessionWithImage(aEvent.target, transArray,
                                                     region, dragAction.action,
                                                     null, 0, 0, aEvent);
      }
      catch(ex) {
        
        
        
      }
      aEvent.stopPropagation();
    },

  










  dragOver: function (aEvent, aDragDropObserver)
    { 
      if (!("onDragOver" in aDragDropObserver)) 
        return;
      if (!this.checkCanDrop(aEvent, aDragDropObserver))
        return;
      var flavourSet = aDragDropObserver.getSupportedFlavours();
      for (var flavour in flavourSet.flavourTable)
        {
          if (this.mDragSession.isDataFlavorSupported(flavour))
            {
              aDragDropObserver.onDragOver(aEvent, 
                                           flavourSet.flavourTable[flavour], 
                                           this.mDragSession);
              aEvent.stopPropagation();
              break;
            }
        }
    },

  mDragSession: null,

  










  drop: function (aEvent, aDragDropObserver)
    {
      if (!("onDrop" in aDragDropObserver))
        return;
      if (!this.checkCanDrop(aEvent, aDragDropObserver))
        return;  
      if (this.mDragSession.canDrop) {
        var flavourSet = aDragDropObserver.getSupportedFlavours();
        var transferData = nsTransferable.get(flavourSet, this.getDragData, true);
        
        var multiple = "canHandleMultipleItems" in aDragDropObserver && aDragDropObserver.canHandleMultipleItems;
        var dropData = multiple ? transferData : transferData.first.first;
        aDragDropObserver.onDrop(aEvent, dropData, this.mDragSession);
      }
      aEvent.stopPropagation();
    },

  










  dragExit: function (aEvent, aDragDropObserver)
    {
      if (!this.checkCanDrop(aEvent, aDragDropObserver))
        return;
      if ("onDragExit" in aDragDropObserver)
        aDragDropObserver.onDragExit(aEvent, this.mDragSession);
    },  
    
  










  dragEnter: function (aEvent, aDragDropObserver)
    {
      if (!this.checkCanDrop(aEvent, aDragDropObserver))
        return;
      if ("onDragEnter" in aDragDropObserver)
        aDragDropObserver.onDragEnter(aEvent, this.mDragSession);
    },  
    
  







  
  getDragData: function (aFlavourSet)
    {
      var supportsArray = Components.classes["@mozilla.org/supports-array;1"]
                                    .createInstance(Components.interfaces.nsISupportsArray);

      for (var i = 0; i < nsDragAndDrop.mDragSession.numDropItems; ++i)
        {
          var trans = nsTransferable.createTransferable();
          for (var j = 0; j < aFlavourSet.flavours.length; ++j)
            trans.addDataFlavor(aFlavourSet.flavours[j].contentType);
          nsDragAndDrop.mDragSession.getData(trans, i);
          supportsArray.AppendElement(trans);
        }
      return supportsArray;
    },

  











  checkCanDrop: function (aEvent, aDragDropObserver)
    {
      if (!this.mDragSession) 
        this.mDragSession = this.mDragService.getCurrentSession();
      if (!this.mDragSession) 
        return false;
      this.mDragSession.canDrop = this.mDragSession.sourceNode != aEvent.target;
      if ("canDrop" in aDragDropObserver)
        this.mDragSession.canDrop &= aDragDropObserver.canDrop(aEvent, this.mDragSession);
      return true;
    },

  











  dragDropSecurityCheck: function (aEvent, aDragSession, aDraggedText)
    {
      var sourceDoc = aDragSession.sourceDocument;
      if (!sourceDoc)
        return;

      
      
      
      
      
      
      

      aDraggedText = aDraggedText.replace(/^\s*|\s*$/g, '');

      var uri;

      try {
        uri = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService)
                        .newURI(aDraggedText, null, null);
      } catch (e) {
      }

      if (!uri)
        return;

      
      const nsIScriptSecurityManager = Components.interfaces
                                                 .nsIScriptSecurityManager;
      var secMan = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                             .getService(nsIScriptSecurityManager);

      try {
        secMan.checkLoadURIStr(sourceDoc.documentURI, aDraggedText,
                               nsIScriptSecurityManager.STANDARD);
      } catch (e) {
        
        aEvent.stopPropagation();

        throw "Drop of " + aDraggedText + " denied.";
      }
    }
};

