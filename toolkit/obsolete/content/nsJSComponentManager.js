





































var nsJSComponentManager = {
  createInstance: function (aContractID, aIID)
    {
      try
        {
          var iid = Components.interfaces[aIID];
          return Components.classes[aContractID].createInstance(iid);
        }
      catch(e)
        {
        }
        return null;
    },

  createInstanceByID: function (aID, aIID)
    {
      try
        {
          var iid = Components.interfaces[aIID];
          return Components.classesByID[aID].createInstance(iid);
        }
      catch(e)
        {
        }
        return null;
    },
        
  getService: function (aContractID, aIID)
    {
      try
        {
          var iid = Components.interfaces[aIID];
          return Components.classes[aContractID].getService(iid);
        }
      catch(e)
        {
        }
        return null;
    },
  
  getServiceByID: function (aID, aIID)  
    {
      try
        {
          var iid = Components.interfaces[aIID];
          return Components.classesByID[aID].getService(iid);
        }
      catch(e)
        {
        }
        return null;
    }
};
