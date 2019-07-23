









function dumpToFile(aData) {
  const path = "/tmp";

  var outputFile = Cc["@mozilla.org/file/local;1"].
                   createInstance(Ci.nsILocalFile);
  outputFile.initWithPath(path);
  outputFile.append("testdump.png");

  var outputStream = Cc["@mozilla.org/network/file-output-stream;1"].
                     createInstance(Ci.nsIFileOutputStream);
  
  outputStream.init(outputFile, 0x02 | 0x08 | 0x20, 0644, null);

  var bos = Cc["@mozilla.org/binaryoutputstream;1"].
            createInstance(Ci.nsIBinaryOutputStream);
  bos.setOutputStream(outputStream);

  bos.writeByteArray(aData, aData.length);

  outputStream.close();
}






function readFileData(aFile) {
  var inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                    createInstance(Ci.nsIFileInputStream);
  
  inputStream.init(aFile, 0x01, -1, null);
  var size = inputStream.available();

  
  var bis = Cc["@mozilla.org/binaryinputstream;1"].
            createInstance(Ci.nsIBinaryInputStream);
  bis.setInputStream(inputStream);

  var bytes = bis.readByteArray(size);

  if (size != bytes.length)
      throw "Didn't read expected number of bytes";

  return bytes;
}









function setAndGetFaviconData(aFilename, aData, aMimeType) {
  var iconURI = uri("http://places.test/" + aFilename);
  try {
    iconsvc.setFaviconData(iconURI,
                           aData, aData.length, aMimeType,
                           Number.MAX_VALUE);
  } catch (ex) {}
  var dataURL = iconsvc.getFaviconDataAsDataURL(iconURI);
  try {
    iconsvc.setFaviconDataFromDataURL(iconURI, dataURL, Number.MAX_VALUE);
  } catch (ex) {}
  var mimeTypeOutparam = {};

  var outData = iconsvc.getFaviconData(iconURI,
                         mimeTypeOutparam, {});

  return [outData, mimeTypeOutparam.value];
}







function compareArrays(aArray1, aArray2) {
  do_check_eq(aArray1.length, aArray2.length);

  for (var i = 0; i < aArray1.length; i++)
      if (aArray1[i] != aArray2[i])
          throw "arrays differ at index " + i;
}



try {
  var iconsvc = Cc["@mozilla.org/browser/favicon-service;1"].
                getService(Ci.nsIFaviconService);

  
  
  
  
  
  var isWindows = ("@mozilla.org/windows-registry-key;1" in Cc);
} catch(ex) {
  do_throw("Could not get favicon service\n");
}


try {
  var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                getService(Ci.nsINavHistoryService);
  var bhist = histsvc.QueryInterface(Ci.nsIBrowserHistory);
} catch(ex) {
  do_throw("Could not get history services\n");
}


function run_test() {
try {


var testnum = 1;
var testdesc = "test storing a normal 16x16 icon";


var iconName = "favicon-normal16.png";
var inMimeType = "image/png";
var iconFile = do_get_file(iconName);

var inData = readFileData(iconFile);
do_check_eq(inData.length, 286);

var [outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


do_check_eq(inMimeType, outMimeType);
compareArrays(inData, outData);
                    


testnum++;
testdesc = "test storing a normal 32x32 icon";


iconName = "favicon-normal32.png";
inMimeType = "image/png";
iconFile = do_get_file(iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 344);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


do_check_eq(inMimeType, outMimeType);
compareArrays(inData, outData);



testnum++;
testdesc = "test storing an oversize 16x16 icon ";



iconName = "favicon-big16.ico";
inMimeType = "image/x-icon";
iconFile = do_get_file(iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 1406);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file("expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);


testnum++;
testdesc = "test storing an oversize 4x4 icon ";



iconName = "favicon-big4.jpg";
inMimeType = "image/jpeg";
iconFile = do_get_file(iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 4751);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file("expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);



testnum++;
testdesc = "test storing an oversize 32x32 icon ";



iconName = "favicon-big32.jpg";
inMimeType = "image/jpeg";
iconFile = do_get_file(iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 3494);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file("expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);

if (!isWindows)
  compareArrays(expectedData, outData);



testnum++;
testdesc = "test storing an oversize 48x48 icon ";





iconName = "favicon-big48.ico";
inMimeType = "image/x-icon";
iconFile = do_get_file(iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 56646);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file("expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);


testnum++;
testdesc = "test storing an oversize 64x64 icon ";



iconName = "favicon-big64.png";
inMimeType = "image/png";
iconFile = do_get_file(iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 10698);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file("expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);


testnum++;
testdesc = "test scaling an oversize 160x3 icon ";



iconName = "favicon-scale160x3.jpg";
inMimeType = "image/jpeg";
iconFile = do_get_file(iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 5095);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file("expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);


testnum++;
testdesc = "test scaling an oversize 3x160 icon ";



iconName = "favicon-scale3x160.jpg";
inMimeType = "image/jpeg";
iconFile = do_get_file(iconName);

inData = readFileData(iconFile);
do_check_eq(inData.length, 5059);

[outData, outMimeType] = setAndGetFaviconData(iconName, inData, inMimeType);


var expectedFile = do_get_file("expected-" + iconName + ".png");
var expectedData = readFileData(expectedFile);


do_check_eq("image/png", outMimeType);
compareArrays(expectedData, outData);



testnum++;
testdesc = "test set and get favicon ";


var icon1Name = "favicon-normal32.png";
var icon1MimeType = "image/png";
var icon1File = do_get_file(icon1Name);
var icon1Data = readFileData(icon1File);
do_check_eq(icon1Data.length, 344);
var icon1URI = uri("file:///./" + icon1Name);


var icon2Name = "favicon-normal16.png";
var icon2MimeType = "image/png";
var icon2File = do_get_file(icon2Name);
var icon2Data = readFileData(icon2File);
do_check_eq(icon2Data.length, 286);
var icon2URI = uri("file:///./" + icon2Name);

var page1URI = uri("http://foo.bar/");
var page2URI = uri("http://bar.foo/");
var page3URI = uri("http://foo.bar.moz/");


histsvc.addVisit(page1URI, Date.now() * 1000, null,
                 histsvc.TRANSITION_TYPED, false, 0);
histsvc.addVisit(page2URI, Date.now() * 1000, null,
                 histsvc.TRANSITION_TYPED, false, 0);
histsvc.addVisit(page3URI, Date.now() * 1000, null,
                 histsvc.TRANSITION_TYPED, false, 0);


try {
  iconsvc.setFaviconData(icon1URI, icon1Data, icon1Data.length,
                         icon1MimeType, Number.MAX_VALUE);
} catch (ex) {}
iconsvc.setFaviconUrlForPage(page1URI, icon1URI);
var savedIcon1URI = iconsvc.getFaviconForPage(page1URI);


try {
  iconsvc.setFaviconData(icon2URI, icon2Data, icon2Data.length,
                         icon2MimeType, Number.MAX_VALUE);
} catch (ex) {}
iconsvc.setFaviconUrlForPage(page2URI, icon2URI);
var savedIcon2URI = iconsvc.getFaviconForPage(page2URI);


try {
  iconsvc.setFaviconData(icon1URI, icon1Data, icon1Data.length,
                         icon1MimeType, Number.MAX_VALUE);
} catch (ex) {}
iconsvc.setFaviconUrlForPage(page3URI, icon1URI);
var savedIcon3URI = iconsvc.getFaviconForPage(page3URI);


var out1MimeType = {};
var out1Data = iconsvc.getFaviconData(savedIcon1URI, out1MimeType, {});
do_check_eq(icon1MimeType, out1MimeType.value);
compareArrays(icon1Data, out1Data);


var out2MimeType = {};
var out2Data = iconsvc.getFaviconData(savedIcon2URI, out2MimeType, {});
do_check_eq(icon2MimeType, out2MimeType.value);
compareArrays(icon2Data, out2Data);


var out3MimeType = {};
var out3Data = iconsvc.getFaviconData(savedIcon3URI, out3MimeType, {});
do_check_eq(icon1MimeType, out3MimeType.value);
compareArrays(icon1Data, out3Data);



testnum++;
testdesc = "test setAndLoadFaviconForPage ";


iconName = "favicon-normal32.png";
inMimeType = "image/png";
iconFile = do_get_file(iconName);
inData = readFileData(iconFile);
do_check_eq(inData.length, 344);
var pageURI = uri("http://foo.bar/");

var faviconURI = uri("file:///./" + iconName);

iconsvc.setAndLoadFaviconForPage(pageURI, faviconURI, true);

var savedFaviconURI = iconsvc.getFaviconForPage(pageURI);
outMimeType = {};
outData = iconsvc.getFaviconData(savedFaviconURI, outMimeType, {});


do_check_eq(inMimeType, outMimeType.value);
compareArrays(inData, outData);



testnum++;
testdesc = "test favicon links ";

do_check_eq(iconsvc.getFaviconImageForPage(pageURI).spec,
            iconsvc.getFaviconLinkForIcon(faviconURI).spec);



testnum++;
testdesc = "test failed favicon cache ";


iconName = "favicon-normal32.png";
faviconURI = uri("file:///./" + iconName);

iconsvc.addFailedFavicon(faviconURI);
do_check_true(iconsvc.isFailedFavicon(faviconURI));
iconsvc.removeFailedFavicon(faviconURI);
do_check_false(iconsvc.isFailedFavicon(faviconURI));




} catch (e) {
    throw "FAILED in test #" + testnum + " -- " + testdesc + ": " + e;
}
};
