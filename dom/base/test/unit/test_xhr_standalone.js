






function run_test()
{
    var xhr = Components.classes['@mozilla.org/xmlextras/xmlhttprequest;1'].
          createInstance(Components.interfaces.nsIXMLHttpRequest);
    xhr.open('GET', 'data:,', false);
    var exceptionThrown = false;
    try {
        xhr.responseType = '';
        xhr.withCredentials = false;
    } catch (e) {
        exceptionThrown = true;
    }
    do_check_eq(false, exceptionThrown);
}
