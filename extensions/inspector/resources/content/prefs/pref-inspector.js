

































function Startup()
{
  SidebarPrefs_initialize();
  enableBlinkPrefs(document.getElementById("inspector.blink.on").value);
}

function enableBlinkPrefs(aTruth)
{
  



  let els = {
    lbElBorderColor: "cprElBorderColor",
    lbElBorderWidth: "txfElBorderWidth",
    lbElDuration: "txfElDuration",
    lbElSpeed: "txfElSpeed",
    "": "cbElInvert"
  };

  for (let [label, control] in Iterator(els)) {
    let controlElem = document.getElementById(control);

    
    if (aTruth && !isPrefLocked(controlElem)) {
      controlElem.removeAttribute("disabled");
      if (label)
        document.getElementById(label).removeAttribute("disabled");
    } else {
      controlElem.setAttribute("disabled", true);
      if (label)
        document.getElementById(label).setAttribute("disabled", true);
    }
  }
}

function isPrefLocked(elem)
{
  if (!elem.hasAttribute("preference"))
    return false;

  return document.getElementById(elem.getAttribute("preference")).locked;
}
