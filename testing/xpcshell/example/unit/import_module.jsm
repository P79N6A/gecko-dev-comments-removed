

const EXPORTED_SYMBOLS = [ "MODULE_IMPORTED", "MODULE_URI", "SUBMODULE_IMPORTED", "same_scope" ];

const MODULE_IMPORTED = true;
const MODULE_URI = __URI__;


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.importRelative(this, "import_sub_module.jsm");


var scope1 = { __URI__: __URI__ };
var scope2 = { __URI__: __URI__ };

XPCOMUtils.importRelative(scope1, "import_sub_module.jsm");
scope1.test_obj.i++;

XPCOMUtils.importRelative(scope2, "duh/../import_sub_module.jsm");




var same_scope = (scope1.test_obj.i == scope2.test_obj.i);
