









































function sessionHistoryListener()
{
   ddump("In sessionHistoryListener constructor\n");
   this.interfaceName = "nsISHistoryListener";

}


sessionHistoryListener.prototype =
{
    result: "",
    debug: 1,

    QueryInterface: function(aIID)
    {
        netscape.security.PrivilegeManager.enablePrivilege("UniversalBrowserRead");
        netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
        if (aIID.equals(Components.interfaces.nsISHistoryListener) ||
            aIID.equals(Components.interfaces.nsISupportsWeakReference) ||
            aIID.equals(Components.interfaces.nsISupports))
            return this;

        Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
        return null;
    },

    OnHistoryNewEntry: function(newUrl)
    {
       netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
       uriSpec = newUrl.spec;

       ddump("In OnHistoryNewEntry(). uriSpec = " + uriSpec + "\n");
    },

    OnHistoryGoBack: function(uri)
    {
       uriSpec = uri.spec;

       ddump("In OnHistoryGoBack(). uriSpec = " + uriSpec + "\n");
       backCallback = true;
       return true;
    },

    OnHistoryGoForward: function(uri)
    {
       uriSpec = uri.spec;

       ddump("In OnHistoryGoForward(). uriSpec = " + uriSpec + "\n");
       forwardCallback = true;
       return true;
    },

    OnHistoryReload: function(uri, reloadFlags)
    {
       uriSpec = uri.spec;

       ddump("In OnHistoryReload(). uriSpec = " + uriSpec + "\n");
       ddump("In OnHistoryReload(). reloadFlags = " + reloadFlags + "\n");
       reloadCallback = true;
       return true;
    },

    OnHistoryGotoIndex: function(index, uri)
    {
       uriSpec = uri.spec;

       ddump("In OnHistoryGotoIndex(). uriSpec = " + uriSpec + "\n");
       ddump("In OnHistoryGotoIndex(). index = " + index + "\n");
       gotoCallback = true;
       return true;
    },

    OnHistoryPurge: function(numEntries)
    {
       ddump("In OnHistoryPurge(). numEntries = " + numEntries + "\n");
       purgeCallback = true;
       return true;
    },

    ddump: function(s)
    {
       if (debug ==1)
          dump(s);
       else if (debug==2)
          alert(s);
    }
}


