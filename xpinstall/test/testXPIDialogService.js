

















































const XPIDIALOGSERVICE_CONTRACTID =
    "@mozilla.org/embedui/xpinstall-dialog-service;1";

const XPIDIALOGSERVICE_CID =
    Components.ID("{9A5BEF68-3FDA-4926-9809-87A5A1CC8505}");

const XPI_TOPIC = "xpinstall-progress";
const OPEN      = "open";
const CANCEL    = "cancel";






function testXPIDialogService() {}

testXPIDialogService.prototype =
{
    QueryInterface: function( iid )
    {
        if (iid.equals(Components.interfaces.nsIXPIDialogService) ||
            iid.equals(Components.interfaces.nsIXPIProgressDialog) ||
            iid.equals(Components.interfaces.nsISupports))
            return this;

        Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
        return null;
    },

    confirmInstall: function( parent, packages, count )
    {
        
        this.mParent = parent;

        
        var str = "num packages: " + count/2 + "\n\n";
        for ( i = 0; i < count; ++i)
            str += packages[i++] + ' -- ' + packages[i] + '\n';

        str += "\nDo you want to install?";

        return parent.confirm(str);
    },

    openProgressDialog: function( packages, count, mgr )
    {
        this.dlg = this.mParent.open();
        mgr.observe( this, XPI_TOPIC, OPEN );
    },

    onStateChange: function( index, state, error )
    {
        dump("---XPIDlg--- State: "+index+', '+state+', '+error+'\n');
    },

    onProgress: function( index, value, max )
    {
        dump("---XPIDlg---     "+index+": "+value+' of '+max+'\n');
    }
};








function NSGetModule(compMgr, fileSpec) { return XPIDlgSvcModule; }



var XPIDlgSvcModule =
{
    registerSelf: function( compMgr, fileSpec, location, type )
    {
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);

        compMgr.registerFactoryLocation(XPIDIALOGSERVICE_CID,
            'XPInstall Dialog Service test component',
            XPIDIALOGSERVICE_CONTRACTID, fileSpec,
            location, type);
    },

    unregisterSelf: function( compMgr, fileSpec, location )
    {
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
        compMgr.unregisterFactoryLocation(XPIDIALOGSERVICE_CID, fileSpec);
    },

    getClassObject: function( compMgr, cid, iid )
    {
        if (!cid.equals(XPIDIALOGSERVICE_CID))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return XPIDlgSvcFactory;
    },

    canUnload: function( compMgr ) { return true; }
};



var XPIDlgSvcFactory =
{
    createInstance: function( outer, iid )
    {
        if (outer != null)
            throw Components.results.NS_ERROR_NO_AGGREGATION;

        if (!iid.equals(Components.interfaces.nsIXPIDialogService) &&
            !iid.equals(Components.interfaces.nsISupports))
            throw Components.results.NS_ERROR_INVALID_ARG;

        return new testXPIDialogService();
    }
};
