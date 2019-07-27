
function resolutionFromFilename(filename)
{
    resolution = filename.replace(/^.*[^0-9]([0-9]+x[0-9]+)[^0-9].*$/, "$1");
    if (resolution != filename) {
        return resolution;
    }
    return "";
}

function appendBuffer(test, sourceBuffer, data)
{
    test.expectEvent(sourceBuffer, "update");
    test.expectEvent(sourceBuffer, "updateend");
    sourceBuffer.appendBuffer(data);
}

function mediaSourceConfigChangeTest(directory, idA, idB, description)
{
    var manifestFilenameA = directory + "/test-" + idA + "-manifest.json";
    var manifestFilenameB = directory + "/test-" + idB + "-manifest.json";
    mediasource_test(function(test, mediaElement, mediaSource)
    {
        mediaElement.pause();
        test.failOnEvent(mediaElement, 'error');
        var expectResizeEvents = resolutionFromFilename(manifestFilenameA) != resolutionFromFilename(manifestFilenameB);
        var expectedResizeEventCount = 0;

        MediaSourceUtil.fetchManifestAndData(test, manifestFilenameA, function(typeA, dataA)
        {
            MediaSourceUtil.fetchManifestAndData(test, manifestFilenameB, function(typeB, dataB)
            {
                assert_equals(typeA, typeB, "Media format types match");

                var sourceBuffer = mediaSource.addSourceBuffer(typeA);

                appendBuffer(test, sourceBuffer, dataA);
                ++expectedResizeEventCount;

                test.waitForExpectedEvents(function()
                {
                    
                    sourceBuffer.timestampOffset = 0.5;
                    appendBuffer(test, sourceBuffer, dataB);
                    ++expectedResizeEventCount;
                });

                test.waitForExpectedEvents(function()
                {
                    
                    sourceBuffer.timestampOffset = 1;
                    appendBuffer(test, sourceBuffer, dataA);
                    ++expectedResizeEventCount;
                });

                test.waitForExpectedEvents(function()
                {
                    
                    sourceBuffer.timestampOffset = 1.5;
                    appendBuffer(test, sourceBuffer, dataB);
                    ++expectedResizeEventCount;
                });

                test.waitForExpectedEvents(function()
                {
                    assert_false(sourceBuffer.updating, "updating");
                    assert_greater_than(mediaSource.duration, 2, "duration");

                    
                    mediaSource.duration = 2;

                    assert_true(sourceBuffer.updating, "updating");
                    test.expectEvent(sourceBuffer, 'updatestart', 'sourceBuffer');
                    test.expectEvent(sourceBuffer, 'update', 'sourceBuffer');
                    test.expectEvent(sourceBuffer, 'updateend', 'sourceBuffer');
		});

		test.waitForExpectedEvents(function()
                {
                    assert_false(sourceBuffer.updating, "updating");

                    mediaSource.endOfStream();

                    assert_false(sourceBuffer.updating, "updating");

                    if (expectResizeEvents) {
                        for (var i = 0; i < expectedResizeEventCount; ++i) {
                            test.expectEvent(mediaElement, "resize");
                        }
                    }
                    test.expectEvent(mediaElement, "ended");
                    mediaElement.play();
                });

                test.waitForExpectedEvents(function() {
                    test.done();
                });
            });
        });
    }, description);
};
