const Ci = Components.interfaces;
const Cc = Components.classes;

let pb = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);



const kPrefName = 'intl.accept_languages'; 
                                           
let initialValue = null;

function check_child_pref_info_eq(continuation) {
    sendCommand(
        'var pb = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);\n'+
        
        'pb.getCharPref("'+ kPrefName +'")+ "," +'+
        'pb.prefHasUserValue("'+ kPrefName +'");',
        function (info) {
            let [ value, isUser ] = info.split(',');
            do_check_eq(pb.getCharPref(kPrefName), value);
            do_check_eq(pb.prefHasUserValue(kPrefName), isUser == "true");
            continuation();
        });
}

function run_test() {
    
    do_test_pending();

    try {
        if (pb.getCharPref('dom.ipc.processPrelauch.enabled')) {
            dump('WARNING: Content process may already have launched, so this test may not be meaningful.');
        }
    } catch(e) { }

    initialValue = pb.getCharPref(kPrefName);

    test_user_setting();
}

function test_user_setting() {
    
    
    
    pb.setCharPref(kPrefName, 'i-imaginarylanguage');
    
    
    
    check_child_pref_info_eq(function () {
            do_check_eq(pb.prefHasUserValue(kPrefName), true);

            test_cleared_is_default();
        });
}

function test_cleared_is_default() {
    pb.clearUserPref(kPrefName);
    
    
    
    check_child_pref_info_eq(function () {
            do_check_eq(pb.prefHasUserValue(kPrefName), false);

            clean_up();
        });
}

function clean_up() {
    pb.setCharPref(kPrefName, initialValue);
    
    
    
    check_child_pref_info_eq(function () {
            do_test_finished();
        });
}