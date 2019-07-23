








































function run_test()
{
    var ms = Components.classes["@mozilla.org/mime;1"].getService(Components.interfaces.nsIMIMEService);

    
    var types = {
        "image/jpeg": ["jpg", "jpeg"], 
        "image/gif": ["gif"],
        "image/png": ["png"]
    };

    
    for (var mimetype in types) {
        var exts = types[mimetype];
        var primary = ms.getFromTypeAndExtension(mimetype, null).primaryExtension.toLowerCase();

        do_check_true (exts.indexOf(primary) != -1);
    }
}
