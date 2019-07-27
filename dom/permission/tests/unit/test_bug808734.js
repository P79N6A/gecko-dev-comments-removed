const Cu = Components.utils;
const READWRITE = "readwrite";
const UNKNOWN = "foobar";

var gData = [

{
  permission: "contacts",
  access: READWRITE,
  expected: ["contacts-read", "contacts-create",
             "contacts-write"]
},

{
  permission: "settings",
  access: READWRITE,
  expected: ["settings-read", "settings-write",
             "indexedDB-chrome-settings-read",
             "indexedDB-chrome-settings-write"]
},

{
  permission: "storage",
  expected: ["indexedDB-unlimited",
             "default-persistent-storage"]
},

{
  permission: "contacts",
  access: UNKNOWN,
  expected: []
},

{
  permission: UNKNOWN,
  access: READWRITE,
  expected: []
}
];


function do_check_set_eq(a1, a2) {
  do_check_eq(a1.length, a2.length)

  Array.sort(a1);
  Array.sort(a2);

  for (let i = 0; i < a1.length; ++i) {
    do_check_eq(a1[i], a2[i])
  }
}

function test_substitute_does_not_break_substituted(scope) {
  const Ci = Components.interfaces;

  
  do_check_eq(scope.PermissionsTable["geolocation-noprompt"].substitute[0],
              "geolocation");
  
  do_check_eq(scope.PermissionsTable["geolocation-noprompt"].certified,
              Ci.nsIPermissionManager.ALLOW_ACTION)
  
  do_check_false(scope.isExplicitInPermissionsTable("geolocation-noprompt", Ci.nsIPrincipal.APP_STATUS_CERTIFIED));
  
  do_check_true(scope.isExplicitInPermissionsTable("geolocation", Ci.nsIPrincipal.APP_STATUS_CERTIFIED));
}

function run_test() {
  var scope = {};
  Cu.import("resource://gre/modules/PermissionsTable.jsm", scope);

  for (var i = 0; i < gData.length; i++) {
    var perms = scope.expandPermissions(gData[i].permission,
                                        gData[i].access);
    do_check_set_eq(perms, gData[i].expected);
  }
  test_substitute_does_not_break_substituted(scope);
}
