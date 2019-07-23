





































var gParser = new DOMParser;
var gProc = new XSLTProcessor;
var gTimeout;

function Test(aTitle, aSourceURL, aStyleURL, aNumber, aObserver)
{
    this.mTitle = aTitle;
    this.mObserver = aObserver;
    this.mTotal = aNumber;
    this.mDone = 0;
    var xmlcontent = loadFile(aSourceURL);
    var xslcontent = loadFile(aStyleURL);
    this.mSource = gParser.parseFromString(xmlcontent, 'application/xml');
    this.mStyle = gParser.parseFromString(xslcontent, 'application/xml');
}

function runTest(aTitle, aSourceURL, aStyleURL, aNumber, aObserver)
{
    test = new Test(aTitle, aSourceURL, aStyleURL, aNumber,
                        aObserver);
    gTimeout = setTimeout(onNextTransform, 100, test, 0);
}

function onNextTransform(aTest, aNumber)
{
    res = document.implementation.createDocument('', '', null);
    var startTime = Date.now();
    gProc.transformDocument(aTest.mSource, aTest.mStyle, res, null);
    var endTime = Date.now();
    aNumber++;
    var progress = aNumber / aTest.mTotal * 100;
    if (aTest.mObserver) {
        aTest.mObserver.progress(aTest.mTitle, endTime - startTime,
                                 progress);
    }
    if (aNumber < aTest.mTotal) {
        gTimeout = setTimeout(onNextTransform, 100, aTest, aNumber);
    } else if (aTest.mObserver) {
        aTest.mObserver.done(aTest.mTitle);
    }
}
