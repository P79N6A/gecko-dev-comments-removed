











































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;




function test_generateQI_string_names()
{
    var x = {
        QueryInterface: XPCOMUtils.generateQI([
            Components.interfaces.nsIClassInfo,
            "nsIDOMNode"
        ])
    };

    try {
        x.QueryInterface(Components.interfaces.nsIClassInfo);
    } catch(e) {
        do_throw("Should QI to nsIClassInfo");
    }
    try {
        x.QueryInterface(Components.interfaces.nsIDOMNode);
    } catch(e) {
        do_throw("Should QI to nsIDOMNode");
    }
    try {
        x.QueryInterface(Components.interfaces.nsIDOMDocument);
        do_throw("QI should not have succeeded!");
    } catch(e) {}
}

function test_defineLazyGetter()
{
    let accessCount = 0;
    let obj = { };
    const TEST_VALUE = "test value";
    XPCOMUtils.defineLazyGetter(obj, "foo", function() {
        accessCount++;
        return TEST_VALUE;
    });
    do_check_eq(accessCount, 0);

    
    do_check_eq(obj.foo, TEST_VALUE);
    do_check_eq(accessCount, 1);

    
    
    do_check_eq(obj.foo, TEST_VALUE);
    do_check_eq(accessCount, 1);
}

function test_defineLazyServiceGetter()
{
    let obj = { };
    XPCOMUtils.defineLazyServiceGetter(obj, "service",
                                       "@mozilla.org/consoleservice;1",
                                       "nsIConsoleService");
    let service = Cc["@mozilla.org/consoleservice;1"].
                  getService(Ci.nsIConsoleService);

    
    
    for (let prop in obj.service)
        do_check_true(prop in service);
    for (let prop in service)
        do_check_true(prop in obj.service);
}




let tests = [
    test_generateQI_string_names,
    test_defineLazyGetter,
    test_defineLazyServiceGetter,
];

function run_test()
{
    tests.forEach(function(test) {
        print("Running next test: " + test.name);
        test();
    });
}
