





































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
  else {
    this.dxOff();
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
  if (this.on){
    this.dxOff();
    $("#inspectDialog").dialog().parents(".ui-dialog:first").find(".ui-dialog-buttonpane button")[2].innerHTML = "Start";
  }
  else{
    this.dxOn();
    $("#inspectDialog").dialog().parents(".ui-dialog:first").find(".ui-dialog-buttonpane button")[2].innerHTML = "Stop";
  }
}



DomInspectorConnector.prototype.dxOn = function() {
  this.on = true;
  $("#inspectDialog").dialog().parents(".ui-dialog:first").find(".ui-dialog-buttonpane button")[2].innerHTML = "Stop";
  document.getElementById('eventsOut').value = "";
  
  
  
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
  $("#inspectDialog").dialog().parents(".ui-dialog:first").find(".ui-dialog-buttonpane button")[2].innerHTML = "Start";
  

  
  if (this.lastEvent){
    this.lastEvent.target.style.border = "";
  }
  
  
  var clickMethod = "dblclick";
  if (document.getElementById('inspectSingle').checked){
    clickMethod = 'click';
  }
  
  
  var copyOutputBox = $('copyout');
  
  
  
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
  MozMilldx.dxToggle();
  e.target.style.border = "";
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
   else {
     window.document.getElementById('eventsOut').value += "-----\n";
     window.document.getElementById('eventsOut').value += "Shift Key: "+ e.shiftKey + "\n";
     window.document.getElementById('eventsOut').value += "Control Key: "+ e.ctrlKey + "\n";
     window.document.getElementById('eventsOut').value += "Alt Key: "+ e.altKey + "\n";
     window.document.getElementById('eventsOut').value += "Meta Key: "+ e.metaKey + "\n\n";
     
     
     
     
     var ctrlString = "";
     ctrlString += MozMilldx.evtDispatch(e);
     ctrlString += "\nController: controller.keypress(element,"+e.charCode+",";
     ctrlString += "{";
     if (e.ctrlKey){
       ctrlString += "ctrlKey:"+e.ctrlKey.toString()+",";
     }
     if (e.altKey){
       ctrlString += "altKey:"+e.altKey.toString()+",";
     }
     if (e.shiftKey){
       ctrlString += "shiftKey:"+e.shiftKey.toString()+",";
     }
     if (e.metaKey){
       ctrlString += "metaKey:"+e.metaKey.toString();
     }
     ctrlString += "});\n";
     ctrlString = ctrlString.replace(/undefined/g, "false");         
     document.getElementById('eventsOut').value += ctrlString;
   }
}

DomInspectorConnector.prototype.dxRecursiveBind = function(frame, clickMethod) {
  
  this.dxRecursiveUnBind(frame, clickMethod);
  
  frame.addEventListener('mouseover', this.evtDispatch, true);
  frame.addEventListener('mouseout', this.evtDispatch, true);
  frame.addEventListener(clickMethod, this.getFoc, true);
  frame.addEventListener('keypress', this.clipCopy, true);
  
  
  var iframeCount = frame.window.frames.length;
  var iframeArray = frame.window.frames;

  for (var i = 0; i < iframeCount; i++)
  {
      try {
        iframeArray[i].addEventListener('mouseover', this.evtDispatch, true);
        iframeArray[i].addEventListener('mouseout', this.evtDispatch, true);
        iframeArray[i].addEventListener(clickMethod, this.getFoc, true);
        iframeArray[i].addEventListener('keypress', this.clipCopy, true);
        

        this.dxRecursiveBind(iframeArray[i], clickMethod);
      }
      catch(error) {
          
          

      }
  }
}

DomInspectorConnector.prototype.dxRecursiveUnBind = function(frame, clickMethod) {
  frame.removeEventListener('mouseover', this.evtDispatch, true);
  frame.removeEventListener('mouseout', this.evtDispatch, true);
  frame.removeEventListener(clickMethod, this.getFoc, true);
  frame.removeEventListener('keypress', this.clipCopy, true);
  
  
  var iframeCount = frame.window.frames.length;
  var iframeArray = frame.window.frames;

  for (var i = 0; i < iframeCount; i++)
  {
      try {
        iframeArray[i].removeEventListener('mouseover', this.evtDispatch, true);
        iframeArray[i].removeEventListener('mouseout', this.evtDispatch, true);
        iframeArray[i].removeEventListener(clickMethod, this.getFoc, true);
        iframeArray[i].removeEventListener('keypress', this.clipCopy, true);

        this.dxRecursiveUnBind(iframeArray[i], clickMethod);
      }
      catch(error) {
          
          
      }
  }
}

var MozMilldx = new DomInspectorConnector();


var enableDX = function () {
  MozMilldx.dxOn();
}
var disableDX = function () {
  MozMilldx.dxOff();
}