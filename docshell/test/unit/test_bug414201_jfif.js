








































function run_test()
{
    var ms = Components.classes["@mozilla.org/mime;1"].getService(Components.interfaces.nsIMIMEService);

    
    var types = {
        "image/jpeg": "jpg",
        "image/gif": "gif",
        "image/png": "png"
    };

    
    for (var mimetype in types) {
        var ext = types[mimetype];
        do_check_true (ms.getFromTypeAndExtension(mimetype, null).primaryExtension.toLowerCase() == ext);
    }
}
