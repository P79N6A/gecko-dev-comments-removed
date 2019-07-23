













































function bugAccess()
{
  var onloadBugList;
  var onloadBugSpecs;
  var onloadBugSearch;
  var onloadBugSave;
  var onloadLogin;

  this.getBugSpecs = function(inputId,inUrl,onloadFunc)
  {
    if(onloadFunc!=undefined)
    {
      onloadBugSpecs = onloadFunc;
    }else
      alert("ERROR: getBugList called without function parameter");
    var url = inUrl+"show_bug.cgi?ctype=xml&id=";
    
    url += inputId;

    
    var xmlHttp=new XMLHttpRequest();
    xmlHttp.open("GET", url, true);


    var req=new XMLHttpRequest();
    req.open("GET", url, true);
    var callback = this.parseBugSpecs;
    req.onreadystatechange = function (aEvt) {
    if (req.readyState == 4) {
      if(req.status == 200) {

       callback(req);
    }else
           alert("Error loading page\n");
    }
    };
    req.send(null);


    
    xmlHttp.send(null);

    
  }
  
  this.parseBugSpecs = function(e)
  {
    var bugInfo = new Object();
    
    var xml = e.responseXML;
  

    if(xml.getElementsByTagName("bug")[0].getAttribute("error")!=null)
    {
      alert("The requested bug was: "+ xml.getElementsByTagName("bug")[0].getAttribute("error"));
      bugInfo["id"] = "";
      bugInfo["title"] = "";
      bugInfo["status"] = "";
      bugInfo["info"] = "";

    }else
    {
      bugInfo["id"] = xml.getElementsByTagName("bug_id")[0].firstChild.data;
      bugInfo["title"] = xml.getElementsByTagName("short_desc")[0].firstChild.data;
      bugInfo["status"] = xml.getElementsByTagName("bug_status")[0].firstChild.data
      if (xml.getElementsByTagName("resolution").length)
        bugInfo["status"] += "--" + xml.getElementsByTagName("resolution")[0].firstChild.data;
      bugInfo["info"] = xml.getElementsByTagName("short_desc")[0].firstChild.data + "\n\n"
                + xml.getElementsByTagName("thetext")[0].firstChild.data;
    }

    onloadBugSpecs(bugInfo);
  }

  this.setBugListOnloadFunc = function(onloadFunc){onloadBugList = onloadFunc;}

  this.setBugSearchOnloadFunc = function(onloadFunc){onloadBugSearch = onloadFunc;}

  
  this.getBugList = function(inURL, params, mode)
  {
    var xmlHttp=new XMLHttpRequest();
    xmlHttp.open("GET", inURL+"/buglist.cgi?"+params, true);

    dump("searching: " + inURL+"/buglist.cgi?"+params);
    
    xmlHttp.send(null);
    if(mode == 1)
      xmlHttp.onload=this.parseBugList;
    else if(mode == 0)
      xmlHttp.onload=this.parseBugSearch;
  }

  
  this.parseBugList = function(e)
  {
    var bugData = parseList(e.target.responseText);
    onloadBugList(bugData);
  }

  this.parseBugSearch = function(e)
  {
    var bugData = parseList(e.target.responseText);
    onloadBugSearch(bugData);
  }

  this.writeBugToBugzilla = function(inUrl,bugSpec,fn)
  {
    onloadBugSave=fn;

  var names = new Array(bugSpecs.getFieldTotal());
  names[0]="id";
  names[1]="product";
  names[2]="component";
  names[3]="status";
  names[4]="resolution";
  names[5]="assigned_to";
  names[6]="rep_platform";
  names[7]="op_sys";
  names[8]="version";
  names[9]="priority";
  names[10]="bug_severity";
  names[11]="target_milestone";
  names[12]="reporter";
  names[13]="qa_contact";
  names[14]="bug_file_loc";
  names[15]="short_desc";

  var fieldCount = bugSpecs.getFieldTotal();
  var varString="";

  for(i=0;i<fieldCount-1;i++)
  {
    if(names[i]!="reporter")
      varString+=names[i]+"="+bugSpec.getSpec(i)+"&";
  }

  varString+="longdesclength=&knob="+bugSpec.getKnob();


  var xmlHttp=new XMLHttpRequest();

  xmlHttp.open("POST", inUrl+"/process_bug.cgi",true);

  xmlHttp.setRequestHeader("Method", "POST "+inUrl+"/process_bug.cgi"+" HTTP/1.1");
  xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded");


  xmlHttp.send(varString);
  
  xmlHttp.onload = this.writeBugToBugzillaReturnStatus;
}

  this.writeBugToBugzillaReturnStatus = function(e)
  {
    if (e.target.readyState == 4)
     {

      if (e.target.status == 200)
       {
        var start;
         
        if(e.target.responseText.indexOf('name="Bugzilla_login">')!=-1)
        {
          
          alert("Your not logged in to this instance of bugzilla");
        }else if(e.target.responseText.indexOf('Bug processed')!=-1)
        {
          var success = "Changes Made";

          alert(success);
        }else if((start = e.target.responseText.indexOf('<td bgcolor="#ff0000">'))!=-1)
        {
          
          var error = "Error: "+e.target.responseText.substring(start,e.target.responseText.indexOf('</td>'));
          error = error.replace(/\n|\t|/g,"");
          error = error.replace(/<[^>]+>/g,"");
          error = getCleanText(error);
          alert(error);
        }

       }else
       {
        alert("Connection failed! possible network error");
       }

      onloadBugSave();

     }else
     {
      
      alert("load phase: "+e.target.readyState);
    }
  }

  this.loginToBugzilla = function(inUrl, user, passwd, fn)
  {
    onloadLogin = fn;

    var varString = "&Bugzilla_login="+user+"&Bugzilla_password="+passwd;

    var xmlHttp=new XMLHttpRequest();

    xmlHttp.open("POST", inUrl+"/index.cgi?GoAheadAndLogIn=1",true);

    xmlHttp.setRequestHeader("Method", "POST "+inUrl+"/index.cgi"+" HTTP/1.1");
    xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded");

    xmlHttp.send(varString);
    xmlHttp.onload = this.loginToBugzillaStatus;

  }

  this.loginToBugzillaStatus = function(e)
  {
    
    var rc=0;

    var start;
    if(e.target.responseText.indexOf('href="relogin.cgi"')!=-1)
    {
      
      rc=1;
      alert("Login successful");

    }else if(e.target.responseText.toLowerCase().indexOf('invalid username or password')!=-1)
    {
      rc=-1;
      alert("Bugzilla reported that your user name or password was invalid please verify");
    }

    onloadLogin(rc);

  }

  }

  
  
  function parseList(xmlHttp)
  {
  var beginLoc=0;
  var endLoc=0;
  var id;
  var summary;

  bugData=new Array();
  
  
  
  
  
  
  
  
  
  endLoc = xmlHttp.search(/<table class=\"bz_buglist sortable\"/);
  if (endLoc <= 0) {
    
    
    
    alert("ERROR: No Bugs Found.");
    return bugData;
  }

  for(var i = 0;(beginLoc = xmlHttp.indexOf('show_bug.cgi?id',endLoc)) != -1; i++)
  {
    endLoc = xmlHttp.indexOf('"',beginLoc);
    beginLoc = xmlHttp.indexOf('=',beginLoc);

    id=xmlHttp.substring(beginLoc+1, endLoc);


    
    var curLoc=endLoc;

    endLoc = xmlHttp.indexOf('</tr>',endLoc);
    endLoc = xmlHttp.lastIndexOf('</td>',endLoc);
    beginLoc = xmlHttp.lastIndexOf("<td >",endLoc);
    if(beginLoc==-1)
    {
      beginLoc = xmlHttp.lastIndexOf("<td>",endLoc);
    }

    summary = xmlHttp.substring(beginLoc, endLoc);

    summary = getCleanText(summary);
    bugData[i] = new Array(2);
    bugData[i][0] = id;
    bugData[i][1] = summary;
    
  }
  
  return bugData;
}
