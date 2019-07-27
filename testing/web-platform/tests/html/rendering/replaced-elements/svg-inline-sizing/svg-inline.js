






















var debugHint = function(id) { return "(append ?"+id+" to debug) "; };
var testSingleId;
if (window.location.search) {
    testSingleId = window.location.search.substring(1);
    debugHint = function(id) { return ""; };
}

var testContainer = document.querySelector('#testContainer');
var outerWidth = testContainer.getBoundingClientRect().width;
var outerHeight = testContainer.getBoundingClientRect().height;

SVGSizing.doCombinationTest(
    [["placeholder", [ null ]],
     ["containerWidthStyle", [null, "400px"]],
     ["containerHeightStyle", [null, "400px"]],
     ["svgViewBoxAttr", [ null, "0 0 100 200" ]],
     ["svgWidthStyle", [ null, "100px", "50%" ]],
     ["svgHeightStyle", [ null, "100px", "50%" ]],
     ["svgWidthAttr", [ null, "200", "25%" ]],
     ["svgHeightAttr", [ null, "200", "25%" ]]],
    function(config, id, cont) {
        var testData = new SVGSizing.TestData(config);

        var expectedRect =
                testData.computeInlineReplacedSize(outerWidth, outerHeight);
        var svgElement = testData.buildSVGOrPlaceholder();
        var container =
                testData.buildContainer(svgElement);

        var checkSize = function() {
            var svgRect =
                    svgElement.getBoundingClientRect();

            try {
                assert_equals(svgRect.width,
                              expectedRect.width,
                              debugHint(id) + "Wrong width");
                assert_equals(svgRect.height,
                              expectedRect.height,
                              debugHint(id) + "Wrong height");
            } finally {
                testContainer.removeChild(container);
                if (testSingleId)
                    document.body.removeChild(testContainer);
                cont(id+1);
            }
        };

        testContainer.appendChild(container);
        test(checkSize, testData.name);

        if (testSingleId == id) {
            testData.buildDemo(expectedRect, id);
        }
    }, testSingleId);
