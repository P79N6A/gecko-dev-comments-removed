





































var inspection = {}; Components.utils.import('resource://mozmill/modules/inspection.js', inspection);
var utils = {}; Components.utils.import('resource://mozmill/modules/utils.js', utils);
var objects = {}; Components.utils.import('resource://mozmill/stdlib/objects.js', objects);
var arrays = {}; Components.utils.import('resource://mozmill/stdlib/arrays.js', arrays);
var events = {}; Components.utils.import('resource://mozmill/modules/events.js', events);

var controller = {};  Components.utils.import('resource://mozmill/modules/controller.js', controller);



var currentRecorderArray = [];

var getEventSet = function (eArray) {
  var inSet = function (a, c) {
    for each(x in a) {
      if (x.evt.timeStamp == c.evt.timeStamp && c.evt.type == x.evt.type) {
        return true;
      }
    }
    return false;
  }
  
  var returnArray = [];
  for each(e in eArray) {
    
    if (!inSet(returnArray, e)) {
      returnArray.push(e);
    }
  }
  return returnArray;
}

var recorderMethodCases = {
  'click': function (x) {return 'click('+x['inspected'].elementText+')';},
  
  
  
  
  'keypress': function (x) {
    if (x['evt'].charCode == 0){
      return 'keypress(' + x['inspected'].elementText + ',' + x['evt'].keyCode + ', {ctrlKey:' +x['evt'].ctrlKey 
              + ', altKey:'+ x['evt'].altKey + ',shiftKey:' + x['evt'].shiftKey + ', metaKey:' + x['evt'].metaKey + '})';
    }
    else {
      return 'keypress(' + x['inspected'].elementText + ',"' + String.fromCharCode(x['evt'].charCode) + '", {ctrlKey:' +x['evt'].ctrlKey 
              + ', altKey:'+ x['evt'].altKey + ',shiftKey:' + x['evt'].shiftKey + ', metaKey:' + x['evt'].metaKey + '})'; 
    }
   },
  'change': function (x) {return 'type('+x['inspected'].elementText+',"'+x['evt'].target.value+'")';},
  'dblclick': function (x) { return 'doubleClick('+x['inspected'].elementText+')';},
}

var cleanupEventsArray = function (recorder_array) {
  var indexesForRemoval = [];
  var type_indexes = [x['evt'].type for each(x in recorder_array)];
  
  
  if (arrays.inArray(type_indexes, 'change')) {
    var offset = 0;
    while (arrays.indexOf(type_indexes, 'change', offset) != -1) {
      var eIndex = arrays.indexOf(type_indexes, 'change', offset);
      var e = recorder_array[eIndex];
      if (e['evt'].target.value != undefined && arrays.compare(e['evt'].target.value, 
        [String.fromCharCode(x['evt'].charCode) for 
        each(x in recorder_array.slice(eIndex - (e['evt'].target.value.length + 1) ,eIndex - 1))
        ])) {
          var i = eIndex - (e['evt'].target.value.length + 1)
          while (i < eIndex) {
            indexesForRemoval.push(i); i++;            
          }
        } else if (e['evt'].target.value != undefined && arrays.compare(e['evt'].target.value, 
        [String.fromCharCode(x['evt'].charCode) for 
        each(x in recorder_array.slice(eIndex - (e['evt'].target.value.length) ,eIndex - 1))
        ])) {
          var i = eIndex - (e['evt'].target.value.length )
          while (i < eIndex) {
            indexesForRemoval.push(i); i++;
          }
        }
      var offset = arrays.indexOf(type_indexes, 'change', offset) + 1;
    }
  }
  
  
  for (i in recorder_array) {
    var x = recorder_array[i];
    if (x['evt'].type ==  "change") {
      if (x['evt'].target.value == '') {
        indexesForRemoval.push(i);
      }
    }
  }
  
  
  try {
    var i = 1;
    while (recorder_array[recorder_array.length - i]['inspected'].controllerText == 'new mozmill.controller.MozMillController(mozmill.utils.getWindowByTitle("MozMill IDE"))') {
      i++;
      if (recorder_array[recorder_array.length - i]['evt'].charCode == 96) {
        indexesForRemoval.push(recorder_array.length - i);
      }   
    }
  } catch(err){}
  
  
  for (i in recorder_array) {
    var inspected = recorder_array[i]['inspected'];
    if (inspected.controllerText == 'new mozmill.controller.MozMillController(mozmill.utils.getWindowByTitle("MozMill IDE"))') {
      indexesForRemoval.push(i);
    }
  }
  
  var narray = [];
  for (i in recorder_array) {
    if (!arrays.inArray(indexesForRemoval, i)) {
      narray.push(recorder_array[i]);
    }
  }
  
  return narray;
}

var getRecordedScript = function (recorder_array) {
  var setup = {};
  var test = [];
  
  var recorder_array = cleanupEventsArray(getEventSet(recorder_array));
  
  for each(x in recorder_array) {
    var inspected = x['inspected'];
    if (!setup[inspected.controllerText]) {
      if (objects.getLength(setup) > 0) {
        var ext = String(objects.getLength(setup) + 1);
      } else {
        var ext = '';
      }
      setup[inspected.controllerText] = 'controller'+ext
    }
    if (recorderMethodCases[x['evt'].type] == undefined) {
      alert("Don't have a case for event type: "+x['evt'].type)
    }
    var methodString = recorderMethodCases[x['evt'].type](x).replace(inspected.documentString, inspected.documentString.replace('controller.', setup[inspected.controllerText]+'.'))

    if (x['evt'].button == 2){
      methodString = methodString.replace("click", "rightClick");
    }
    test.push(setup[inspected.controllerText]  + '.' + methodString + ';');
  }
  
  var rscript = ['var setupModule = function(module) {',];
  for (i in setup) {
    rscript.push("  "+setup[i]+' = '+i+';')
  }
  rscript.push('}')
  rscript.push('')
  rscript.push('var testRecorded = function () {')
  for each(t in test){
    rscript.push('  '+t);
  }
  rscript.push('}')
  return rscript.join('\n');
}

var RecorderConnector = function() {
  this.lastEvent = null;
  this.ison = false;
}

RecorderConnector.prototype.toggle = function(){
  if (this.ison)
    this.off();
  else
    this.on();
};

RecorderConnector.prototype.dispatch = function(evt){
  currentRecorderArray.push({'evt':evt, 'inspected':inspection.inspectElement(evt)});
  var value = editor.getContent();
  value += (evt.type + ':: ' + evt.timeStamp + '\n');
  editor.setContent(value);
}


RecorderConnector.prototype.bindListeners = function(frame) {
  frame.addEventListener('click', this.dispatch, false);
  frame.addEventListener('dblclick', this.dispatch, false);
  frame.addEventListener('change', this.dispatch, false);
  frame.addEventListener('keypress', this.dispatch, false);

  var iframeCount = frame.window.frames.length;
  var iframeArray = frame.window.frames;

  for (var i = 0; i < iframeCount; i++)
    this.bindListeners(iframeArray[i]);
}


RecorderConnector.prototype.unbindListeners = function(frame) {
  try {
    frame.removeEventListener('click', this.dispatch, false);
    frame.removeEventListener('dblclick', this.dispatch, false);
    frame.removeEventListener('change', this.dispatch, false);
    frame.removeEventListener('keypress', this.dispatch, false);
  }
  catch(error) {}
  
  var iframeCount = frame.window.frames.length;
  var iframeArray = frame.window.frames;

  for (var i = 0; i < iframeCount; i++)
    this.unbindListeners(iframeArray[i]);
}


RecorderConnector.prototype.observer = {
  observe: function(subject,topic,data){
    var defer = function(){
      controller.waitForEval("subject.documentLoaded == true", 10000, 100, subject)
      MozMillrec.bindListeners(subject);
    }
    window.setTimeout(defer, 500);
  }
};

RecorderConnector.prototype.on = function() {
  this.ison = true;

  
  $("#recordToggle").text("Stop Recording");
  $("#recordToggle").addClass("ui-state-highlight");
  $("#recordToggle").addClass("ui-priority-primary");

  newFile();
  
  for each(win in utils.getWindows()) {
    if (win.document.title != "MozMill IDE"){
      this.bindListeners(win);
    }
  }

  var mmWindows = utils.getWindows('navigator:browser');
  if (mmWindows.length != 0){
    mmWindows[0].focus();
  }
  
  var observerService =
    Components.classes["@mozilla.org/observer-service;1"]
      .getService(Components.interfaces.nsIObserverService);
  
  
  observerService.addObserver(this.observer, "toplevel-window-ready", false);
  
  currentRecorderArray = [];
};

RecorderConnector.prototype.off = function() {
  this.ison = false;

  
  $("#recordToggle").text("Record");
  $("#recordToggle").removeClass("ui-state-highlight");
  $("#recordToggle").removeClass("ui-priority-primary");

  
  for each(win in utils.getWindows()) {
    this.unbindListeners(win);
  }
  var r = getRecordedScript(currentRecorderArray);

  editor.setContent(r);
  currentRecorderArray = [];
  
  var observerService =
    Components.classes["@mozilla.org/observer-service;1"]
      .getService(Components.interfaces.nsIObserverService);
  try { observerService.removeObserver(this.observer, "toplevel-window-ready"); }
  catch(err){}
};

var MozMillrec = new RecorderConnector();


var enableRec = function () {
  MozMillrec.on();
}
var disableRec = function () {
  MozMillrec.off();
}
