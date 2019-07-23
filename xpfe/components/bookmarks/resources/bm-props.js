





































var gFields;



var gProperties;


var gBookmarkID;

function Init()
{
  initServices();
  initBMService();

  
  gFields     = ["name", "url", "shortcut", "description"];

  
  
  gProperties = [NC_NS + "Name",
                 NC_NS + "URL",
                 NC_NS + "ShortcutURL",
                 NC_NS + "Description"];

  gBookmarkID = window.arguments[0];

  var i;
  var resource = RDF.GetResource(gBookmarkID);
  
  

  for (i = 0; i < gFields.length; ++i) {
    var field = document.getElementById(gFields[i]);

    var value = BMDS.GetTarget(resource, RDF.GetResource(gProperties[i]), true);

    if (value)
      value = value.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;

    if (value) 
      field.value = value;
  }

  var nameNode = document.getElementById("name");
  document.title = document.title.replace(/\*\*bm_title\*\*/gi, nameNode.value);

  
  var isContainerFlag = RDFCU.IsContainer(BMDS, resource);
  if (!isContainerFlag) {
    
    
    
    
  }

  var isSeparator = BookmarksUtils.resolveType(resource) == "BookmarkSeparator";

  if (isContainerFlag || isSeparator) {
    
    document.getElementById("locationrow").setAttribute("hidden", "true");
    document.getElementById("shortcutrow").setAttribute("hidden", "true");
    if (isSeparator) {
      document.getElementById("descriptionrow").setAttribute("hidden", "true");
    }
  }

  var showScheduling = false;
  var url = BMDS.GetTarget(resource, RDF.GetResource(gProperties[1]), true);
  if (url) {
    url = url.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
    if (url.substr(0, 7).toLowerCase() == "http://" ||
        url.substr(0, 8).toLowerCase() == "https://") {
      showScheduling = true;
    }
  }

  if (!showScheduling) {
    
    document.getElementById("scheduling").setAttribute("hidden", "true");
  } else {
    
    var scheduleArc = RDF.GetResource("http://home.netscape.com/WEB-rdf#Schedule");
    value = BMDS.GetTarget(resource, scheduleArc, true);
  
    if (value) {
      value = value.QueryInterface(Components.interfaces.nsIRDFLiteral).Value;
  
      if (value) {
        var values = value.split("|");
        if (values.length == 4) {
          
          var days = values[0];
          var dayNode = document.getElementById("dayRange");
          var dayItems = dayNode.childNodes[0].childNodes;
          for (i=0; i < dayItems.length; ++i) {
            if (dayItems[i].getAttribute("value") == days) {
              dayNode.selectedItem = dayItems[i];
              break;
            }
          }
          
          dayRangeChange(dayNode);
  
          
          var hours = values[1].split("-");
          var startHour = "";
          var endHour = "";
  
          if (hours.length == 2) {
            startHour = hours[0];
            endHour = hours[1];
          }
  
          
          var startHourNode = document.getElementById("startHourRange");
          var startHourItems = startHourNode.childNodes[0].childNodes;
          for (i=0; i < startHourItems.length; ++i) {
            if (startHourItems[i].getAttribute("value") == startHour) {
              startHourNode.selectedItem = startHourItems[i];
              break;
            }
          }
  
          
          var endHourNode = document.getElementById("endHourRange");
          var endHourItems = endHourNode.childNodes[0].childNodes;
          for (i=0; i < endHourItems.length; ++i) {
            if (endHourItems[i].getAttribute("value") == endHour) {
              endHourNode.selectedItem = endHourItems[i];
              break;
            }
          }
  
          
          var duration = values[2];
          var durationNode = document.getElementById("duration");
          durationNode.value = duration;
  
          
          var method = values[3];
          if (method.indexOf("icon") >= 0)
            document.getElementById("bookmarkIcon").checked = true;
  
          if (method.indexOf("sound") >= 0)
            document.getElementById("playSound").checked = true;
  
          if (method.indexOf("alert") >= 0)
            document.getElementById("showAlert").checked = true;
  
          if (method.indexOf("open") >= 0)
            document.getElementById("openWindow").checked = true;
        }
      }
    }
  }

  sizeToContent();
  
  
  nameNode.focus();
  nameNode.select();

}


function Commit()
{
  var changed = false;

  
  
  
  for (var i = 0; i < gFields.length; ++i) {
    var field = document.getElementById(gFields[i]);

    if (field) {
      
      var newvalue = field.value;

      var oldvalue = BMDS.GetTarget(RDF.GetResource(gBookmarkID),
                                    RDF.GetResource(gProperties[i]), true);

      if (oldvalue)
        oldvalue = oldvalue.QueryInterface(Components.interfaces.nsIRDFLiteral);

      if (newvalue && gProperties[i] == (NC_NS + "ShortcutURL")) {
        
        newvalue = newvalue.toLowerCase();
      }
      else if (newvalue && gProperties[i] == (NC_NS + "URL")) {
        
        
        if (newvalue.indexOf(":") < 0)
          newvalue = "http://" + newvalue;
      }

      if (newvalue)
        newvalue = RDF.GetLiteral(newvalue);

      if (updateAttribute(gProperties[i], oldvalue, newvalue)) {
        changed = true;
      }
    }
  }

  
  
  var scheduling = document.getElementById("scheduling");
  var schedulingHidden = scheduling.getAttribute("hidden");
  if (schedulingHidden != "true") {
    var scheduleRes = "http://home.netscape.com/WEB-rdf#Schedule";
    oldvalue = BMDS.GetTarget(RDF.GetResource(gBookmarkID),
                              RDF.GetResource(scheduleRes), true);
    newvalue = "";
    var dayRangeNode = document.getElementById("dayRange");
    var dayRange = dayRangeNode.selectedItem.getAttribute("value");

    if (dayRange) {
      var startHourRangeNode = document.getElementById("startHourRange");
      var startHourRange = startHourRangeNode.selectedItem.getAttribute("value");

      var endHourRangeNode = document.getElementById("endHourRange");
      var endHourRange = endHourRangeNode.selectedItem.getAttribute("value");

      if (parseInt(startHourRange) > parseInt(endHourRange)) {
        var temp = startHourRange;
        startHourRange = endHourRange;
        endHourRange = temp;
      }

      var duration = document.getElementById("duration").value;
      if (!duration) {
        alert(BookmarksUtils.getLocaleString("pleaseEnterADuration"));
        return false;
      }

      var methods = [];
      if (document.getElementById("bookmarkIcon").checked)
        methods.push("icon");
      if (document.getElementById("playSound").checked)
        methods.push("sound");
      if (document.getElementById("showAlert").checked)
        methods.push("alert");
      if (document.getElementById("openWindow").checked)
        methods.push("open");

      if (methods.length == 0) {
        alert(BookmarksUtils.getLocaleString("pleaseSelectANotification"));
        return false;
      }

      var method = methods.join(); 

      newvalue = dayRange + "|" + startHourRange + "-" + endHourRange + "|" + duration + "|" + method;
    }

    if (newvalue)
      newvalue = RDF.GetLiteral(newvalue);

    if (updateAttribute(scheduleRes, oldvalue, newvalue))
      changed = true;
  }

  if (changed) {
    var remote = BMDS.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
    if (remote)
      remote.Flush();
  }

  window.close();
  return true;
}

function updateAttribute(prop, oldvalue, newvalue)
{
  var changed = false;

  if (prop && (oldvalue || newvalue) && oldvalue != newvalue) {

    if (oldvalue && !newvalue) {
      BMDS.Unassert(RDF.GetResource(gBookmarkID), 
                    RDF.GetResource(prop), oldvalue);
    }
    else if (!oldvalue && newvalue) {
      BMDS.Assert(RDF.GetResource(gBookmarkID),
                  RDF.GetResource(prop), newvalue, true);
    }
    else  {
      BMDS.Change(RDF.GetResource(gBookmarkID), 
                  RDF.GetResource(prop), oldvalue, newvalue);
    }

    changed = true;
  }

  return changed;
}

function setEndHourRange()
{
  
  var startHourRangeNode = document.getElementById("startHourRange");
  var startHourRange = startHourRangeNode.selectedItem.getAttribute("value");
  var startHourRangeInt = parseInt(startHourRange);

  var endHourRangeNode = document.getElementById("endHourRange");
  var endHourRange = endHourRangeNode.selectedItem.getAttribute("value");
  var endHourRangeInt = parseInt(endHourRange);

  var endHourItemNode = endHourRangeNode.firstChild.firstChild;

  var index = 0;

  
  for (; index < startHourRangeInt; ++index) {
    endHourItemNode.setAttribute("disabled", "true");
    endHourItemNode = endHourItemNode.nextSibling;
  }

  
  if (startHourRangeInt >= endHourRangeInt)
    endHourRangeNode.selectedItem = endHourItemNode;

  
  for (; index < 24; ++index) {
    endHourItemNode.removeAttribute("disabled");
    endHourItemNode = endHourItemNode.nextSibling;
  }
}

function dayRangeChange (aMenuList)
{
  var controls = ["startHourRange", "endHourRange", "duration", "bookmarkIcon", 
                  "showAlert", "openWindow", "playSound", "durationSubLabel", 
                  "durationLabel", "startHourRangeLabel", "endHourRangeLabel"];
  for (var i = 0; i < controls.length; ++i)
    document.getElementById(controls[i]).disabled = !aMenuList.value;
}
