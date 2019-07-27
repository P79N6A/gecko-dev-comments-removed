var All_Pointer_Events = [
        "pointerdown",
        "pointerup",
        "pointercancel",
        "pointermove",
        "pointerover",
        "pointerout",
        "pointerenter",
        "pointerleave",
        "gotpointercapture",
        "lostpointercapture"];



function check_PointerEvent(event) {
    test(function () {
        assert_true(event instanceof PointerEvent, "event is a PointerEvent event");
    }, event.type + " event is a PointerEvent event");


    
    
    
    
    
    var idl_type_check = {
        "long": function (v) { return typeof v === "number" && Math.round(v) === v; },
        "float": function (v) { return typeof v === "number"; },
        "string": function (v) { return typeof v === "string"; },
        "boolean": function (v) { return typeof v === "boolean" }
    };
    [
        ["readonly", "long", "pointerId"],
        ["readonly", "float", "width"],
        ["readonly", "float", "height"],
        ["readonly", "float", "pressure"],
        ["readonly", "long", "tiltX"],
        ["readonly", "long", "tiltY"],
        ["readonly", "string", "pointerType"],
        ["readonly", "boolean", "isPrimary"]
    ].forEach(function (attr) {
        var readonly = attr[0];
        var type = attr[1];
        var name = attr[2];


        
        test(function () {
            assert_true(name in event, name + " attribute in " + event.type + " event");
        }, event.type + "." + name + " attribute exists");


        
        if (readonly === "readonly") {
            test(function () {
                assert_readonly(event.type, name, event.type + "." + name + " cannot be changed");
            }, event.type + "." + name + " is readonly");
        }


        
        test(function () {
            assert_true(idl_type_check[type](event[name]), name + " attribute of type " + type);
        }, event.type + "." + name + " IDL type " + type + " (JS type was " + typeof event[name] + ")");
    });


    
    
    test(function () {
        
        assert_greater_than_equal(event.pressure, 0, "pressure is greater than or equal to 0");
        assert_less_than_equal(event.pressure, 1, "pressure is less than or equal to 1");


        
        if (event.pointerType === "mouse") {
            if (event.buttons === 0) {
                assert_equals(event.pressure, 0, "pressure is 0 for mouse with no buttons pressed");
            } else {
                assert_equals(event.pressure, 0.5, "pressure is 0.5 for mouse with a button pressed");
            }
        }
    }, event.type + ".pressure value is valid");


    
    if (event.pointerType === "mouse") {
        
        test(function () {
            assert_equals(event.tiltX, 0, event.type + ".tiltX is 0 for mouse");
            assert_equals(event.tiltY, 0, event.type + ".tiltY is 0 for mouse");
            assert_true(event.isPrimary, event.type + ".isPrimary is true for mouse");
        }, event.type + " properties for pointerType = mouse");
        
    }
}

function showPointerTypes() {
    var complete_notice = document.getElementById("complete-notice");
    var pointertype_log = document.getElementById("pointertype-log");
    var pointertypes = Object.keys(detected_pointertypes);
    pointertype_log.innerHTML = pointertypes.length ?
        pointertypes.join(",") : "(none)";
    complete_notice.style.display = "block";
}

function log(msg, el) {
    if (++count > 10){
      count = 0;
      el.innerHTML = ' ';
    }
    el.innerHTML = msg + '; ' + el.innerHTML;
}

 function failOnScroll() {
    assert_true(false,
    "scroll received while shouldn't");
}

function updateDescriptionNextStep() {
    document.getElementById('desc').innerHTML = "Test Description: Try to scroll text RIGHT.";
}

function updateDescriptionComplete() {
    document.getElementById('desc').innerHTML = "Test Description: Test complete";
}

function updateDescriptionSecondStepTouchActionElement(target, scrollReturnInterval) {
    window.setTimeout(function() {
    objectScroller(target, 'up', 0);}
    , scrollReturnInterval);
    document.getElementById('desc').innerHTML = "Test Description: Try to scroll element RIGHT moving your outside of the red border";
}

function updateDescriptionThirdStepTouchActionElement(target, scrollReturnInterval) {
    window.setTimeout(function() {
    objectScroller(target, 'left', 0);}
    , scrollReturnInterval);
    document.getElementById('desc').innerHTML = "Test Description: Try to scroll element DOWN then RIGHT starting your touch inside of the element. Then tap complete button";
}

function updateDescriptionFourthStepTouchActionElement(target, scrollReturnInterval) {
    document.getElementById('desc').innerHTML = "Test Description: Try to scroll element RIGHT starting your touch inside of the element";
}

function objectScroller(target, direction, value) {
    if (direction == 'up') {
        target.scrollTop = 0;
    } else if (direction == 'left') {
        target.scrollLeft = 0;
    }
}

function sPointerCapture(e) {
    try {
        if(target0.setPointerCapture)
            target0.setPointerCapture(e.pointerId);
        else
            test(function() {
                assert_equals(typeof(target0.setPointerCapture), "function", "target0 should have function setPointerCapture");
            }, "target0 should have function setPointerCapture");
    }
    catch(e) {
        console.log("catch exception: " + e);
        test(function() {
            assert_true(false, "Exception in function setPointerCapture");
        }, "Exception in function setPointerCapture");
    }
}

function rPointerCapture(e) {
    try {
        captureButton.value = 'Set Capture';
        if(target0.releasePointerCapture)
            target0.releasePointerCapture(e.pointerId);
        else
            test(function() {
                assert_equals(typeof(target0.releasePointerCapture), "function", "target0 should have function releasePointerCapture");
            }, "target0 should have function releasePointerCapture");
    }
    catch(e) {
        console.log("catch exception: " + e);
        test(function() {
            assert_true(false, "Exception in function releasePointerCapture");
        }, "Exception in function releasePointerCapture");
    }
}
