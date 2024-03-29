




function write_atomic(file, str) {
    var stream = Cc["@mozilla.org/network/atomic-file-output-stream;1"]
                   .createInstance(Ci.nsIFileOutputStream);
    stream.init(file, -1, -1, 0);
    do {
        var written = stream.write(str, str.length);
        if (written == str.length)
            break;
        str = str.substring(written);
    } while (1);
    stream.QueryInterface(Ci.nsISafeOutputStream).finish();
    stream.close();
}

function write(file, str) {
    var stream = Cc["@mozilla.org/network/safe-file-output-stream;1"]
                   .createInstance(Ci.nsIFileOutputStream);
    stream.init(file, -1, -1, 0);
    do {
        var written = stream.write(str, str.length);
        if (written == str.length)
            break;
        str = str.substring(written);
    } while (1);
    stream.QueryInterface(Ci.nsISafeOutputStream).finish();
    stream.close();
}

function checkFile(file, str) {
    var stream = Cc["@mozilla.org/network/file-input-stream;1"]
                   .createInstance(Ci.nsIFileInputStream);
    stream.init(file, -1, -1, 0);

    var scriptStream = Cc["@mozilla.org/scriptableinputstream;1"]
                         .createInstance(Ci.nsIScriptableInputStream);
    scriptStream.init(stream);

    do_check_eq(scriptStream.read(scriptStream.available()), str);
    scriptStream.close();
}

function run_test()
{
    var filename = "\u0913";
    var file = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties)
                 .get("TmpD", Ci.nsIFile);
    file.append(filename);

    write(file, "First write");
    checkFile(file, "First write");

    write(file, "Second write");
    checkFile(file, "Second write");

    write_atomic(file, "First write: Atomic");
    checkFile(file, "First write: Atomic");

    write_atomic(file, "Second write: Atomic");
    checkFile(file, "Second write: Atomic");
}
