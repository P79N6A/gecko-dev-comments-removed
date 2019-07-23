





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function run_test()
{
    var s = Cc["@mozilla.org/io/string-input-stream;1"]
              .createInstance(Ci.nsIStringInputStream);
    var body = "This is a test";
    s.setData(body, body.length);
    do_check_eq(s.available(), body.length);

    var sis = Cc["@mozilla.org/scriptableinputstream;1"]
                .createInstance(Ci.nsIScriptableInputStream);
    sis.init(s);

    do_check_eq(sis.read(body.length), body);
}
