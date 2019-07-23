






































function Startup()
{
  gDialog.urlInput = document.getElementById("urlInput");
  gDialog.targetInput = document.getElementById("targetInput");
  gDialog.altInput = document.getElementById("altInput");
  gDialog.commonInput = document.getElementById("commonInput");

  gDialog.hsHref = window.arguments[0].getAttribute("hsHref");
  if (gDialog.hsHref != '')
    gDialog.urlInput.value = gDialog.hsHref;

  gDialog.hsAlt = window.arguments[0].getAttribute("hsAlt");
  if (gDialog.hsAlt != '')
    gDialog.altInput.value = gDialog.hsAlt;

  gDialog.hsTarget = window.arguments[0].getAttribute("hsTarget");
  if (gDialog.hsTarget != ''){
    gDialog.targetInput.value = gDialog.hsTarget;
    len = gDialog.commonInput.length;
    for (i=0; i<len; i++){
      if (gDialog.hsTarget == gDialog.commonInput.options[i].value)
        gDialog.commonInput.options[i].selected = "true";
    }
  }

  SetTextboxFocus(gDialog.urlInput);

  SetWindowLocation();
}

function onAccept()
{
  dump(window.arguments[0].id+"\n");
  window.arguments[0].setAttribute("hsHref", gDialog.urlInput.value);
  window.arguments[0].setAttribute("hsAlt", gDialog.altInput.value);
  window.arguments[0].setAttribute("hsTarget", gDialog.targetInput.value);

  SaveWindowLocation();

  window.close();
}

function changeTarget() {
  gDialog.targetInput.value=gDialog.commonInput.value;
}

function chooseFile()
{
  

  fileName = GetLocalFileURL("html");
  if (fileName && fileName != "") {
    gDialog.urlInput.value = fileName;
  }

  
  SetTextboxFocus(gDialog.urlInput);
}
