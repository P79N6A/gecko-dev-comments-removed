
var currentTest;
var gIsRefImageLoaded = false;
const gShouldOutputDebugInfo = false;

function pollForSuccess()
{
  if (!currentTest.isTestFinished) {
    if (!currentTest.reusingReferenceImage || (currentTest.reusingReferenceImage
        && gRefImageLoaded)) {
      currentTest.checkImage();
    }

    setTimeout(pollForSuccess, currentTest.pollFreq);
  }
};

function referencePoller()
{
  currentTest.takeReferenceSnapshot();
}

function reuseImageCallback()
{
  gIsRefImageLoaded = true;
}

function failTest()
{
  if (currentTest.isTestFinished || currentTest.closeFunc) {
    return;
  }

  ok(false, "timing out after " + currentTest.timeout + "ms.  "
     + "Animated image still doesn't look correct, after poll #"
     + currentTest.pollCounter);
  currentTest.wereFailures = true;

  if (currentTest.currentSnapshotDataURI) {
    currentTest.outputDebugInfo("Snapshot #" + currentTest.pollCounter,
                                "snapNum" + currentTest.pollCounter,
                                currentTest.currentSnapshotDataURI);
  }

  currentTest.enableDisplay(document.getElementById(currentTest.debugElementId));

  currentTest.cleanUpAndFinish();
};





























function AnimationTest(pollFreq, timeout, referenceElementId, imageElementId,
                       debugElementId, cleanId, srcAttr, xulTest, closeFunc)
{
  
  
  clearImageCache();

  this.wereFailures = false;
  this.pollFreq = pollFreq;
  this.timeout = timeout;
  this.imageElementId = imageElementId;
  this.referenceElementId = referenceElementId;

  if (!document.getElementById(referenceElementId)) {
    
    
    
    this.reusingImageAsReference = true;
  }

  this.srcAttr = srcAttr;
  this.debugElementId = debugElementId;
  this.referenceSnapshot = ""; 
  this.pollCounter = 0;
  this.isTestFinished = false;
  this.numRefsTaken = 0;
  this.blankWaitTime = 0;

  this.cleanId = cleanId ? cleanId : '';
  this.xulTest = xulTest ? xulTest : '';
  this.closeFunc = closeFunc ? closeFunc : '';
};

AnimationTest.prototype.preloadImage = function()
{
  if (this.srcAttr) {
    this.myImage = new Image();
    this.myImage.onload = function() { currentTest.continueTest(); };
    this.myImage.src = this.srcAttr;
  } else {
    this.continueTest();
  }
};

AnimationTest.prototype.outputDebugInfo = function(message, id, dataUri)
{
  if (!gShouldOutputDebugInfo) {
    return;
  }
  var debugElement = document.getElementById(this.debugElementId);
  var newDataUriElement = document.createElement("a");
  newDataUriElement.setAttribute("id", id);
  newDataUriElement.setAttribute("href", dataUri);
  newDataUriElement.appendChild(document.createTextNode(message));
  debugElement.appendChild(newDataUriElement);
  var brElement = document.createElement("br");
  debugElement.appendChild(brElement);
  todo(false, "Debug (" + id + "): " + message + " " + dataUri);
};

AnimationTest.prototype.isFinished = function()
{
  return this.isTestFinished;
};

AnimationTest.prototype.takeCleanSnapshot = function()
{
  var cleanElement;
  if (this.cleanId) {
    cleanElement = document.getElementById(this.cleanId);
  }

  
  if (cleanElement) {
    this.enableDisplay(cleanElement);
  }

  
  this.cleanSnapshot = snapshotWindow(window, false);

  
  if (cleanElement) {
    this.disableDisplay(cleanElement);
  }

  var dataString1 = "Clean Snapshot";
  this.outputDebugInfo(dataString1, 'cleanSnap',
                       this.cleanSnapshot.toDataURL());
};

AnimationTest.prototype.takeBlankSnapshot = function()
{
  
  this.blankSnapshot = snapshotWindow(window, false);

  var dataString1 = "Initial Blank Snapshot";
  this.outputDebugInfo(dataString1, 'blank1Snap',
                       this.blankSnapshot.toDataURL());
};








AnimationTest.prototype.beginTest = function()
{
  SimpleTest.waitForExplicitFinish();

  currentTest = this;
  this.preloadImage();
};






AnimationTest.prototype.continueTest = function()
{
  
  
  setTimeout(failTest, this.timeout);

  if (!this.reusingImageAsReference) {
    this.disableDisplay(document.getElementById(this.imageElementId));
  }

  this.takeReferenceSnapshot();
  this.setupPolledImage();
  SimpleTest.executeSoon(pollForSuccess);
};

AnimationTest.prototype.setupPolledImage = function ()
{
  
  if (!this.reusingImageAsReference) {
    this.enableDisplay(document.getElementById(this.imageElementId));
    var currentSnapshot = snapshotWindow(window, false);
    var result = compareSnapshots(currentSnapshot,
                                  this.referenceSnapshot, true);

    this.currentSnapshotDataURI = currentSnapshot.toDataURL();

    if (result[0]) {
      
      ok(true, "Animated image looks correct, at poll #"
         + this.pollCounter);

      this.cleanUpAndFinish();
    }
  } else {
    if (!gIsRefImageLoaded) {
      this.myImage = new Image();
      this.myImage.onload = reuseImageCallback;
      document.getElementById(this.imageElementId).setAttribute('src',
        this.referenceElementId);
    }
  }
}

AnimationTest.prototype.checkImage = function ()
{
  if (this.isTestFinished) {
    return;
  }

  this.pollCounter++;

  
  
  if (!this.reusingImageAsReference) {
    this.enableDisplay(document.getElementById(this.imageElementId));
  }

  var currentSnapshot = snapshotWindow(window, false);
  var result = compareSnapshots(currentSnapshot, this.referenceSnapshot, true);

  this.currentSnapshotDataURI = currentSnapshot.toDataURL();

  if (result[0]) {
    
    ok(true, "Animated image looks correct, at poll #"
       + this.pollCounter);

    this.cleanUpAndFinish();
  }
};

AnimationTest.prototype.takeReferenceSnapshot = function ()
{
  this.numRefsTaken++;

  
  if (!this.cleanSnapshot) {
    this.takeCleanSnapshot();
  }

  
  if (!this.blankSnapshot) {
    this.takeBlankSnapshot();
  }

  if (this.reusingImageAsReference) {
    
    var referenceElem = document.getElementById(this.imageElementId);
    this.enableDisplay(referenceElem);

    this.referenceSnapshot = snapshotWindow(window, false);

    var snapResult = compareSnapshots(this.cleanSnapshot,
                                      this.referenceSnapshot, false);
    if (!snapResult[0]) {
      if (this.blankWaitTime > 2000) {
        
        
        this.wereFailures = true;
        ok(snapResult[0],
           "Reference snapshot shouldn't match clean (non-image) snapshot");
      } else {
        this.blankWaitTime += currentTest.pollFreq;
        
        setTimeout(referencePoller, currentTest.pollFreq);
        return;
      }
    }

    ok(snapResult[0],
       "Reference snapshot shouldn't match clean (non-image) snapshot");

    var dataString = "Reference Snapshot #" + this.numRefsTaken;
    this.outputDebugInfo(dataString, 'refSnapId',
                         this.referenceSnapshot.toDataURL());
  } else {
    
    this.disableDisplay(document.getElementById(this.imageElementId));

    
    var referenceDiv = document.getElementById(this.referenceElementId);
    this.enableDisplay(referenceDiv);

    this.referenceSnapshot = snapshotWindow(window, false);
    var snapResult = compareSnapshots(this.cleanSnapshot,
                                      this.referenceSnapshot, false);
    if (!snapResult[0]) {
      if (this.blankWaitTime > 2000) {
        
        
        this.wereFailures = true;
        ok(snapResult[0],
           "Reference snapshot shouldn't match clean (non-image) snapshot");
      } else {
        this.blankWaitTime += 20;
        
        setTimeout(referencePoller, 20);
        return;
      }
    }

    ok(snapResult[0],
       "Reference snapshot shouldn't match clean (non-image) snapshot");

    var dataString = "Reference Snapshot #" + this.numRefsTaken;
    this.outputDebugInfo(dataString, 'refSnapId',
                         this.referenceSnapshot.toDataURL());

    
    this.disableDisplay(referenceDiv);
    this.testBlankCameBack();
  }
};

AnimationTest.prototype.enableDisplay = function(element)
{
  if (!element) {
    return;
  }

  if (!this.xulTest) {
    element.style.display = '';
  } else {
    element.setAttribute('hidden', 'false');
  }
};

AnimationTest.prototype.disableDisplay = function(element)
{
  if (!element) {
    return;
  }

  if (!this.xulTest) {
    element.style.display = 'none';
  } else {
    element.setAttribute('hidden', 'true');
  }
};

AnimationTest.prototype.testBlankCameBack = function()
{
  var blankSnapshot2 = snapshotWindow(window, false);
  var result = compareSnapshots(this.blankSnapshot, blankSnapshot2, true);
  ok(result[0], "Reference image should disappear when it becomes display:none");

  if (!result[0]) {
    this.wereFailures = true;
    var dataString = "Second Blank Snapshot";
    this.outputDebugInfo(dataString, 'blank2SnapId', result[2]);
  }
};

AnimationTest.prototype.cleanUpAndFinish = function ()
{
  
  
  if (this.isTestFinished) {
    return;
  }

  this.isTestFinished = true;

  
  if (this.closeFunc) {
    this.closeFunc();
    return;
  }

  if (this.wereFailures) {
    document.getElementById(this.debugElementId).style.display = 'block';
  }

  SimpleTest.finish();
  document.getElementById(this.debugElementId).style.display = "";
};
