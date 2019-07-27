












var VISIBILITY_STATES =
{
    HIDDEN: "hidden",
    VISIBLE: "visible"
};

var feature_check = false;








function pv_test(func, msg, doc)
{
    if (!doc)
    {
        doc = document;
    }

    
    
    if (!feature_check)
    {
        feature_check = true;

        var hiddenVal = doc.hidden;
        var visStateVal = doc.visibilityState;

        
        test(function()
        {
            assert_true(hiddenVal !== undefined && hiddenVal != null,
                        "document.hidden is defined and not null.");},
                        "document.hidden is defined and not null.");

        test(function()
        {
            assert_true(visStateVal !== undefined && hiddenVal != null,
                        "document.visibilityState is defined and not null.");},
                        "document.visibilityState is defined and not null.");
    
    }

    if (func)
    {
        test(func, msg);
    }
}


function test_feature_exists(doc, msg)
{
    if (!msg)
    {
        msg = "";
    }
    var hiddenMsg = "document.hidden is defined" + msg + ".";
    var stateMsg = "document.visibilityState is defined" + msg + ".";
    pv_test(function(){assert_true(document.hidden !== undefined, hiddenMsg);}, hiddenMsg, doc);
    pv_test(function(){assert_true(document.visibilityState !== undefined, stateMsg);}, stateMsg, doc);
}





function test_true(value, msg)
{
    pv_test(function() { assert_true(value, msg); }, msg);
}

function test_equals(value, equals, msg)
{
    pv_test(function() { assert_equals(value, equals, msg); }, msg);
}





function add_async_result(test_obj, pass_state)
{
    
    test_obj.step(function() { assert_true(pass_state) });

    
    test_obj.done();
}

function add_async_result_assert(test_obj, func)
{
    
    test_obj.step(func);

    
    test_obj.done();
}

var open_link;
function TabSwitch()
{
    
    open_link = window.open('', '_blank');
    setTimeout(function() { open_link.close(); }, 2000);
}
