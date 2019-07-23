






































var gManager;
var gBundle;
var gCanClose = false;
var gCancelled = false;


var progressHooks =
{
    onStateChange: function( aIndex, aState, aValue )
    {
        const state = Components.interfaces.nsIXPIProgressDialog;
        var status = document.getElementById("status"+aIndex);
        var progress = document.getElementById("progress"+aIndex);

        switch( aState ) {
        case state.DOWNLOAD_START:
            status.setAttribute("value",
                        gBundle.getString("progress.downloading"));
            progress.setAttribute("value","0%");
            break;

        case state.DOWNLOAD_DONE:
            status.setAttribute("value",
                        gBundle.getString("progress.downloaded"));
            progress.setAttribute("value","100%");
            break;

        case state.INSTALL_START:
            status.setAttribute("value",
                        gBundle.getString("progress.installing"));
            progress.setAttribute("mode","undetermined");
            break;

        case state.INSTALL_DONE:
            progress.setAttribute("mode","determined");
            progress.hidden = true;
            var msg;
            try
            {
                msg = gBundle.getString("error"+aValue);
            }
            catch (e)
            {
                msg = gBundle.stringBundle.formatStringFromName(
                        "unknown.error", [aValue], 1 );
            }
            status.setAttribute("value",msg);
            break;

        case state.DIALOG_CLOSE:
            
            
            
            document.getElementById("ok").disabled = false;
            document.getElementById("cancel").disabled = true;
            gCanClose = true;

            if (gCancelled)
                window.close();

            break;
        }
    },

    onProgress: function( aIndex, aValue, aMaxValue )
    {
        var percent = Math.round( 100 * (aValue/aMaxValue) );
        var node = document.getElementById("progress"+aIndex);
        node.setAttribute("value", percent);
    },

    QueryInterface: function( iid )
    {
        if (!iid.equals(Components.interfaces.nsISupports) &&
            !iid.equals(Components.interfaces.nsIXPIProgressDialog))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        return this;
    }
}


function onLoad() 
{
    doSetOKCancel(dlgOK, dlgCancel);
    document.getElementById("ok").disabled = true;
    document.getElementById("cancel").focus();
    gBundle = document.getElementById("xpinstallBundle");

    var param = window.arguments[0].QueryInterface(
                    Components.interfaces.nsIDialogParamBlock );
    if ( !param )
        dump (" error getting param block interface \n");

    var i = 0;
    var row = 0;
    var numElements = param.GetInt(1);
    while ( i < numElements )
    {
        var moduleName = param.GetString(i++);
        var URL = param.GetString(i++);
        var certName = param.GetString(i++);
        addTreeItem(row++, moduleName, URL);
    }

    gManager = window.arguments[1];

    
    gManager.observe( progressHooks, "xpinstall-progress", "open" );
}

function addTreeItem(aRow, aName, aUrl)
{
    
    var item = document.createElement("description");
    item.setAttribute("class", "packageName");
    item.setAttribute("id", "package"+aRow);
    item.setAttribute("value", aName);
    item.setAttribute("tooltiptext", aUrl);

    
    var status = document.createElement('description');
    status.setAttribute("class", "packageStatus");
    status.setAttribute("id", "status"+aRow);
    status.setAttribute("value", gBundle.getString("progress.queued"));

    
    var progress = document.createElement("progressmeter");
    progress.setAttribute("class", "packageProgress");
    progress.setAttribute("id", "progress"+aRow);
    progress.setAttribute("value", "0%");

    
    var row  = document.createElement("row");
    row.appendChild(item);
    row.appendChild(status);
    row.appendChild(progress);
    document.getElementById("xpirows").appendChild(row);
}

function dlgOK() { return true; }

function dlgCancel()
{
    gCancelled = true;
    if (gManager)
        gManager.observe( progressHooks, "xpinstall-progress", "cancel");

    
    return gCanClose;
}
