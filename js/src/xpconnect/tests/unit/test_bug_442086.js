






































var types = [
    'PRUint8',
    'PRUint16',
    'PRUint32',
    'PRUint64',
    'PRInt16',
    'PRInt32',
    'PRInt64',
    'float',
    'double'
];

function run_test()
{
  var i;
  for (i = 0; i < types.length; i++) {
    var name = types[i];
    var cls = Components.classes["@mozilla.org/supports-" + name + ";1"];
    var ifname = ("nsISupports" + name.charAt(0).toUpperCase() +
                  name.substring(1));
    var f = cls.createInstance(Components.interfaces[ifname]);

    f.data = 0;
    switch (f.data) {
      case 0:  break;
      default: do_throw("FAILED - bug 442086 (type=" + name + ")");
    }
  }
}
