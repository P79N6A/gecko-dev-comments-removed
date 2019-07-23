





































var nsJSSupportsUtils = {

  createSupportsArray: function ()
    {
      return Components.classes["@mozilla.org/supports-array;1"]
                       .createInstance(Components.interfaces.nsISupportsArray);
    },

  createSupportsWString: function ()
    {  
      return Components.classes["@mozilla.org/supports-string;1"]
                       .createInstance(Components.interfaces.nsISupportsString);
    },
    
  createSupportsString: function ()
    {
      return Components.classes["@mozilla.org/supports-cstring;1"]
                       .createInstance(Components.interfaces.nsISupportsCString);
    }                                                 

};
