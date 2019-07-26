



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/FormHistory.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

const CURRENT_SCHEMA = 4;
const PR_HOURS = 60 * 60 * 1000000;

do_get_profile();

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);



var formHistoryStartup = Cc["@mozilla.org/satchel/form-history-startup;1"].
                         getService(Ci.nsIObserver);
formHistoryStartup.observe(null, "profile-after-change", null);

function getDBVersion(dbfile) {
    var ss = Cc["@mozilla.org/storage/service;1"].
             getService(Ci.mozIStorageService);
    var dbConnection = ss.openDatabase(dbfile);
    var version = dbConnection.schemaVersion;
    dbConnection.close();

    return version;
}

const isGUID = /[A-Za-z0-9\+\/]{16}/;


function searchEntries(terms, params, iter) {
  FormHistory.search(terms, params, { onSuccess: function (results) { iter.send(results); },
                                      onFailure: function (error) {
                                        do_throw("Error occurred searching form history: " + error);
                                      }
                                    });
}



function countEntries(name, value, then) {
  var obj = {};
  if (name !== null)
    obj.fieldname = name;
  if (value !== null)
    obj.value = value;

  FormHistory.count(obj, { onSuccess: function (num) then(num),
                           onFailure: function (error) {
                             do_throw("Error occurred searching form history: " + error);
                           }
                         });
}


function updateEntry(op, name, value, then) {
  var obj = { op: op };
  if (name !== null)
    obj.fieldname = name;
  if (value !== null)
    obj.value = value;
  updateFormHistory(obj, then);
}


function addEntry(name, value, then) {
  let now = Date.now() * 1000;
  updateFormHistory({ op: "add", fieldname: name, value: value, timesUsed: 1,
                      firstUsed: now, lastUsed: now }, then);
}


function updateFormHistory(changes, then) {
  FormHistory.update(changes, { onSuccess: then,
                                onFailure: function (error) {
                                  do_throw("Error occurred updating form history: " + error);
                                }
                              });
}







function do_log_info(aMessage) {
  print("TEST-INFO | " + _TEST_FILE + " | " + aMessage);
}
