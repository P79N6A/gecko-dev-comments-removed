


























var debugHint = function(id) { return "(append ?"+id+" to debug) "; };
var testSingleId;
if (window.location.search) {
    testSingleId = parseInt(window.location.search.substring(1));
    debugHint = function(id) { return ""; };
}

function testPlaceholderWithHeight(placeholder,
                                   placeholderHeightAttr) {
    var testContainer = document.querySelector('#testContainer');
    var outerWidth = testContainer.getBoundingClientRect().width;
    var outerHeight = testContainer.getBoundingClientRect().height;

    SVGSizing.doCombinationTest(
        [["placeholder", [ placeholder ]],
         ["containerWidthStyle", [null, "400px"]],
         ["containerHeightStyle", [null, "400px"]],
         ["placeholderWidthAttr", [null, "100", "50%"]],
         ["placeholderHeightAttr", [placeholderHeightAttr]],
         ["svgViewBoxAttr", [ null, "0 0 100 200" ]],
         ["svgWidthAttr", [ null, "200", "25%" ]],
         ["svgHeightAttr", [ null, "200", "25%" ]]],
        function (config, id, cont) {
            var testData = new SVGSizing.TestData(config);
            var t = async_test(testData.name);
            var expectedRect =
                    testData.computeInlineReplacedSize(outerWidth, outerHeight);
            var placeholder = testData.buildSVGOrPlaceholder();
            var container =
                    testData.buildContainer(placeholder);

            var checkSize = function() {
                var placeholderRect =
                        placeholder.getBoundingClientRect();

                try {
                    assert_equals(placeholderRect.width,
                                  expectedRect.width,
                                  debugHint(id) + "Wrong width");
                    assert_equals(placeholderRect.height,
                                  expectedRect.height,
                                  debugHint(id) + "Wrong height");
                } finally {
                    testContainer.removeChild(container);
                    if (testSingleId)
                        document.body.removeChild(testContainer);
                    cont(id+1);
                }
                t.done();
            };

            if (!config.placeholder) {
                testContainer.appendChild(container);
                test(checkSize, testData.name);
            } else {
                t.step(function() {
                    placeholder.addEventListener('load', function() {
                        
                        
                        
                        setTimeout(t.step_func(checkSize), 0);
                    });
                    testContainer.appendChild(container);
                });
            }
            if (testSingleId == id)
                testData.buildDemo(expectedRect, id);
        }, testSingleId);
}
