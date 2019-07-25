





































var frame = {}; Components.utils.import('resource://mozmill/modules/frame.js', frame);




function openNewWindow(){
  window.open('');
}

function getFileName(path){
  var splt = "/"
  if (navigator.platform.indexOf("Win") != -1){
    splt = "\\";
  }
  var pathArr = path.split(splt);
  return pathArr[pathArr.length-1]
}

function testFinished(){
  document.getElementById('runningStatus').textContent = 'Test Finished, See Output Tab...';
  $("#tabs").tabs().tabs("select", 1);
  window.focus();
}

function openFile(){
  $("#tabs").tabs().tabs("select", 0);
  var openObj = utils.openFile(window);
  if (openObj){
    editAreaLoader.openFile('editorInput', {text:'',title:getFileName(openObj.path),id:openObj.path});
    editAreaLoader.setValue('editorInput', openObj.data);    
  }
}

function saveAsFile() {
  var openFn = utils.saveAsFile(window);  
  if (openFn){
    
    var oldFn = window.openFn;
    window.openFn = openFn;
    var data = utils.getFile(window.openFn);
    editAreaLoader.openFile('editorInput', {text:data,title:getFileName(window.openFn),id:window.openFn});
    
    editAreaLoader.closeFile('editorInput', oldFn);
    return true;
  }
  return false;
}

function saveFile() {
  try { 
    var node = window.frames['frame_editorInput'].document.getElementById('tab_file_'+encodeURIComponent(window.openFn));
    node.getElementsByTagName("strong")[0].style.display = "none";
  }
  catch(err){}
  
  return utils.saveFile(window);
}

function changeEditor() {
  if (window.openFn) {
    
    $('saveMenu').removeAttribute("disabled");
  } else { 
    
    }
}

function closeFile() {
  
  
  var all = editAreaLoader.getAllFiles('editorInput');
  var count = 0;

  for (x in all){
    count++;
  }
  if (count == 1){
    var data = editAreaLoader.getValue('editorInput');
    if (data == ""){
      window.close();
      return;
    }
  }
  
  var really = confirm("Are you sure you want to close this file?");
  if (really == true) {
    editAreaLoader.closeFile('editorInput', window.openFn);
  }
}

function runFile(){
  $('runningStatus').textContent = 'Running File...';
  var nsIFilePicker = Components.interfaces.nsIFilePicker;
  var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
  fp.init(window, "Select a File", nsIFilePicker.modeOpen);
  fp.appendFilter("JavaScript Files","*.js");
  var res = fp.show();
  if (res == nsIFilePicker.returnOK){
    $("#testDialog").dialog("close");
    $("#tabs").tabs("select", 1);
    frame.runTestFile(fp.file.path, true);
  }
  
  testFinished();
}

function runDirectory(){
  $('runningStatus').textContent = 'Running File...';
  var nsIFilePicker = Components.interfaces.nsIFilePicker;
  var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
  fp.init(window, "Select a Directory", nsIFilePicker.modeGetFolder);
  var res = fp.show();
  if (res == nsIFilePicker.returnOK){
    $("#testDialog").dialog("close");
    $("#tabs").tabs("select", 1);
    frame.runTestDirectory(fp.file.path, true);
  }
  testFinished();
}






function runEditor(){
  saveFile();
  
  var doRun = function(){
    document.getElementById('runningStatus').textContent = 'Running Test...';
    
    
    $("#testDialog").dialog("close");
    frame.runTestFile(window.openFn, true);
    testFinished();
  }
    doRun();

}

function newFile(){
  $("#tabs").tabs().tabs("select", 0);
  window.openFn = utils.tempfile().path;
  editAreaLoader.openFile('editorInput', {text:'',title:getFileName(window.openFn),id:window.openFn});
}

function genBoiler(){
    $("#tabs").tabs().tabs("select", 0);
    window.openFn = utils.tempfile().path;
    editAreaLoader.openFile('editorInput', {text:'',title:getFileName(window.openFn),id:window.openFn});
    utils.genBoiler(window);
    var node = window.frames['frame_editorInput'].document.getElementById('tab_file_'+encodeURIComponent(window.openFn));
    node.getElementsByTagName("strong")[0].style.display = "inline";
}

function toggleHideEditor(){
  
  if ($('#frame_editorInput')[0].style.visibility != "hidden"){
    $('#frame_editorInput')[0].style.visibility = "hidden";
    $('#EditAreaArroundInfos_editorInput')[0].style.visibility = "hidden";

    var of = document.createElement('div');
    of.innerHTML = "<b>Open File</b>: " + window.openFn;
    of.id = "ofdiv";
    $('#tabs-1 > p')[0].insertBefore(of, $('#tabs-1 > p')[0].childNodes[0]);
    
  } else{
    $('#ofdiv')[0].parentNode.removeChild($('#ofdiv')[0]);
    $('#frame_editorInput')[0].style.visibility = "visible";
    $('#EditAreaArroundInfos_editorInput')[0].style.visibility = "visible";
  }
}

function swapTabs(tab){
  $('editorTab').style.display = 'none';
  $('outputTab').style.display = 'none';
  $('eventsTab').style.display = 'none';
  $('shellTab').style.display = 'none';
  
  $('editorHead').style.background = '#aaa';
  $('outputHead').style.background = '#aaa';
  $('eventsHead').style.background = '#aaa';
  $('shellHead').style.background = '#aaa';
  
  $(tab+'Tab').style.display = 'block';
  $(tab+'Head').style.background = 'white';
}

function logicalClear(){
  $('#resOut')[0].innerHTML = '';
}

function accessOutput(){
  
  var n = $('resOut');
  var txt = '';
  for (var c = 0; c < n.childNodes.length; c++){
    if (n.childNodes[c].textContent){
      txt += n.childNodes[c].textContent + '\n';  
    }
    else{
      txt += n.childNodes[c].value + '\n';
    }
  }
  if (txt == undefined){ return; }
  copyToClipboard(txt);
}

var copyToClipboard = function(str){
  const gClipboardHelper = Components.classes["@mozilla.org/widget/clipboardhelper;1"] .getService(Components.interfaces.nsIClipboardHelper); 
  gClipboardHelper.copyString(str);
}