





































var inspection = {}; Components.utils.import('resource://mozmill/modules/inspection.js', inspection);
var utils = {}; Components.utils.import('resource://mozmill/modules/utils.js', utils);

var DomInspectorConnector = function() {
  this.lastEvent = null;
  this.lastTime = null;
  this.on = false;
}
DomInspectorConnector.prototype.grab = function(){
  var disp = $('dxDisplay').textContent;
  var dispArr = disp.split(': ');
  $('editorInput').value += 'new elementslib.'+dispArr[0].toUpperCase()+"('"+dispArr[1]+"')\n";
}

DomInspectorConnector.prototype.changeClick = function(e) {
  if (this.on){
    this.dxOff()
    this.dxOn();
  }
}

DomInspectorConnector.prototype.evtDispatch = function(e) {
  
  
  
  var currentTime = new Date();
  var newTime = currentTime.getTime();
  
  if (this.lastTime != null){
    var timeDiff = newTime - this.lastTime;
    this.lastTime = newTime;
        
    if (timeDiff < 2){
      this.lastEvent = e;
      return;
    }
  } else { this.lastTime = newTime; }
  
  
  try { var i = inspection.inspectElement(e); }
  catch(err){ return; }
  
  var dxC = i.controllerText;
  var dxE = i.elementText;
  var dxV = String(i.validation);

  document.getElementById('dxController').innerHTML = dxC;
  document.getElementById('dxValidation').innerHTML = dxV;
  document.getElementById('dxElement').innerHTML = dxE;

  return dxE;
}
DomInspectorConnector.prototype.dxToggle = function(){
  if (this.on)
    this.dxOff();
  else
    this.dxOn();
}



DomInspectorConnector.prototype.dxOn = function() {
  this.on = true;
  $("#dxToggle").text("Stop");

  
  var clickMethod = "dblclick";
  if (document.getElementById('inspectSingle').checked){
    clickMethod = 'click';
  }

  var enumerator = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Components.interfaces.nsIWindowMediator)
                     .getEnumerator("");
  while(enumerator.hasMoreElements()) {
    var win = enumerator.getNext();
    if (win.document.title != 'MozMill IDE'){
      this.dxRecursiveBind(win, clickMethod);
    }
  }

  var observerService =
    Components.classes["@mozilla.org/observer-service;1"]
      .getService(Components.interfaces.nsIObserverService);

  observerService.addObserver(this.observer, "toplevel-window-ready", false);
};


DomInspectorConnector.prototype.observer = {
  observe: function(subject,topic,data){
    var clickMethod = "dblclick";
    if ($('inspectSingle').selected){
      clickMethod = 'click';
    }
    
    MozMilldx.dxRecursiveBind(subject, clickMethod);
  }
};

DomInspectorConnector.prototype.dxOff = function() {
  this.on = false;
  $("#dxToggle").text("Start");
  $("#dxCopy").show();

  
  if (this.lastEvent){
    this.lastEvent.target.style.outline = "";
  }
  
  for each(win in utils.getWindows()) {
    this.dxRecursiveUnBind(win, 'click');
  }
  
  for each(win in utils.getWindows()) {
    this.dxRecursiveUnBind(win, 'dblclick');
  }
  
  var observerService =
    Components.classes["@mozilla.org/observer-service;1"]
      .getService(Components.interfaces.nsIObserverService);

  try { 
    observerService.removeObserver(this.observer, "toplevel-window-ready");
  } catch(err){}
};

DomInspectorConnector.prototype.getFoc = function(e){
  MozMilldx.dxOff();
  e.target.style.outline = "";
  e.stopPropagation();
  e.preventDefault();
  window.focus();
}

DomInspectorConnector.prototype.inspectorToClipboard = function(){
  copyToClipboard($('#dxController')[0].innerHTML +'\n'+$('#dxElement')[0].innerHTML);
};


DomInspectorConnector.prototype.clipCopy = function(e){
   if (e == true){
     copyToClipboard($('#dxElement')[0].innerHTML + ' '+$('#dxValidation')[0].innerHTML + ' ' + $('#dxController')[0].innerHTML);
   }
   else if (e.altKey && e.shiftKey && (e.charCode == 199)){
     copyToClipboard($('#dxElement')[0].innerHTML + ' '+$('#dxValidation')[0].innerHTML + ' ' + $('#dxController')[0].innerHTML);
   }
}


DomInspectorConnector.prototype.dxRecursiveBind = function(frame, clickMethod) {
  
  frame.addEventListener('mouseover', this.evtDispatch, true);
  frame.addEventListener('mouseout', this.evtDispatch, true);
  frame.addEventListener(clickMethod, this.getFoc, true);
  frame.addEventListener('keypress', this.clipCopy, true);
  
  
  var iframeCount = frame.window.frames.length;
  var iframeArray = frame.window.frames;

  for (var i = 0; i < iframeCount; i++)
    this.dxRecursiveBind(iframeArray[i], clickMethod);
}


DomInspectorConnector.prototype.dxRecursiveUnBind = function(frame, clickMethod) {
  try {
    frame.removeEventListener('mouseover', this.evtDispatch, true);
    frame.removeEventListener('mouseout', this.evtDispatch, true);
    frame.removeEventListener(clickMethod, this.getFoc, true);
    frame.removeEventListener('keypress', this.clipCopy, true);
  }
  catch(e) {
    
  }
  
  var iframeCount = frame.window.frames.length;
  var iframeArray = frame.window.frames;

  for (var i = 0; i < iframeCount; i++)
    this.dxRecursiveUnBind(iframeArray[i], clickMethod);
}

var MozMilldx = new DomInspectorConnector();


var enableDX = function () {
  MozMilldx.dxOn();
}
var disableDX = function () {
  MozMilldx.dxOff();
}
