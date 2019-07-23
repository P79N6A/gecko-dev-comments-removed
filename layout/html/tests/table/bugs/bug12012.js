






































function StartCodeTime ( ) {
	CodeTimeInitial = new Date();
	}

function EndCodeTime ( str ) {
	var CodeTimeFinal = new Date();
	var diff = CodeTimeFinal.getTime() - CodeTimeInitial.getTime();
	dump("Timing " + str + " took " + diff + " milliseconds.\n");
	}





haveRead = false;
currentTime = new Date(); 
monthShowing = new Date();
nowShowing = new Date();

month_names_internal = new Array("Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec");



rdf_ns="http://when.com/1999/07/03-appts-rdf#";




var RDFRelFileName = 'bug12012.rdf';

dump("Getting window.location.href...\n");

var baseURL = window.location.href;
dump("...done.\n");



	

if (baseURL.indexOf("file:") == 0) {
	baseURL = baseURL.substring(5);
	while (baseURL.charAt(0) == '/') {
		baseURL = baseURL.substring(1);
	}
	baseURL = "file:///" + baseURL;
}


baseURL = baseURL.substring(0, baseURL.lastIndexOf("/") + 1);


RDFFileName = baseURL + RDFRelFileName;
appts_resourcename = RDFFileName + "#WhenComAppointmentsRoot";


RDFService = Components.classes['@mozilla.org/rdf/rdf-service;1'].getService();
RDFService = RDFService.QueryInterface(Components.interfaces.nsIRDFService);





function ascendToTR (node) {
	var rv = node;
	while ( rv.nodeName.toLowerCase() != "tr" ) {
		rv = rv.parentNode;
	}
	return rv;
}

function handleRClick ( event ) {
	
	var currentNode = ascendToTR(event.target);
	EditApptById(currentNode.getAttribute("id"));
}





function gotoURL( url ) {
	window.content.location = url;
	}

function getAddDiv() {
	return document.documentElement.firstChild.nextSibling.nextSibling.firstChild.firstChild;
	}

function getScheduleDiv() {
	return document.documentElement.firstChild.nextSibling.nextSibling.firstChild.firstChild.nextSibling;
	}

function getPickDiv() {
	return document.documentElement.firstChild.nextSibling.nextSibling.firstChild.firstChild.nextSibling.nextSibling;
	}

function buttonDisabled ( number ) {
	
	
	

	var tabTR = document.getElementById("tabsTR");
	tabTR.childNodes.item(0).className = "tab";
	tabTR.childNodes.item(1).className = "tab";
	tabTR.childNodes.item(2).className = "tab";
	tabTR.childNodes.item(number).className = "tab disable";

	
	
	
	

	
	
	tabTR.parentNode.style.border="medium solid black";
	tabTR.parentNode.style.border="none";
	
}

function divActive( number ) {
	
	
	

	buttonDisabled(number)

	
	var AddDiv = getAddDiv();
	var ScheduleDiv = getScheduleDiv();
	var PickDiv = getPickDiv()

	if ( number == 0 ) {
		ScheduleDiv.style.display = "none";
		PickDiv.style.display = "none";
		AddDiv.style.display = "block";
	} else if ( number == 1 ) {
		AddDiv.style.display = "none";
		PickDiv.style.display = "none";
		ScheduleDiv.style.display = "block";
		ShowSchedule();
	} else if ( number == 2 ) {
		AddDiv.style.display = "none";
		ScheduleDiv.style.display = "none";
		PickDiv.style.display = "block";
		ShowPick();
	}
}

function editButtonsActive( number ) {
	
	

	var adddiv = getAddDiv();
	var addbdiv = GetElementWithIdIn("addbuttons", adddiv);
	var edbdiv = GetElementWithIdIn("editbuttons", adddiv);

	if (number == 0) {
		edbdiv.style.display = "none";
		addbdiv.style.display = "block";
	} else if (number == 1) {
		addbdiv.style.display = "none";
		edbdiv.style.display = "block";
	}

}

function nextDay() {
	nowShowing.setDate(nowShowing.getDate() + 1);
	ShowSchedule();
}

function nextWeek() {
	nowShowing.setDate(nowShowing.getDate() + 7);
	ShowSchedule();
}

function prevDay() {
	nowShowing.setDate(nowShowing.getDate() - 1);
	ShowSchedule();
}

function prevWeek() {
	nowShowing.setDate(nowShowing.getDate() - 7);
	ShowSchedule();
}

function today() {
	nowShowing = new Date();
	ShowSchedule();
	}



function nextMonth() {
	monthShowing.setMonth(monthShowing.getMonth() + 1);
	ShowPick();
}

function prevMonth() {
	monthShowing.setMonth(monthShowing.getMonth() - 1);
	ShowPick();
}

function nextYear() {
	monthShowing.setFullYear(monthShowing.getFullYear() + 1);
	ShowPick();
}

function prevYear() {
	monthShowing.setFullYear(monthShowing.getFullYear() - 1);
	ShowPick();
}

function thisMonth() {
	monthShowing = new Date();
	ShowPick();
	}

function pickDate( year, month, date ) {
	
	nowShowing = new Date(year, month - 1, date);
	divActive(1); 
}








function GetElementWithIdIn ( id, ancestor ) {

	if ( ancestor.getAttribute("id") == id ) {
		return ancestor;
	} else {  
		for (var i = 0; i < ancestor.childNodes.length ; i++ ) {
			
			
			if ( ancestor.childNodes.item(i).nodeType == Node.ELEMENT_NODE ) {
				var rv = GetElementWithIdIn(id, ancestor.childNodes.item(i));
				if (rv != null) {
					return rv; 
				}
			}
		}
		return null;
	}
}





function ApptToString () {
	return "[" + String(this.date) + "," + this.title + "]";
}

Appointment.prototype.toString = ApptToString;

function Appointment ( date, title, desc, dur, id ) { 
	this.date = date; 
	this.title = title; 
	this.desc = desc; 
	this.dur = dur; 
	this.id = id; 



	
	}

function compareAppt ( x , y ) { 
	
	
	return x.date.getTime() - y.date.getTime();
}





function getOutputTBody() {
	
	
	

	

	
	
	return getScheduleDiv().firstChild.nextSibling.firstChild.nextSibling;
	
	
}

function getOutputForDate() {
 
	return getScheduleDiv().firstChild.firstChild.firstChild.nextSibling.nextSibling;
}

function timeString( date ) {
	var minutes = date.getMinutes();
	if (minutes < 10 ) {
		minutes = "0" + minutes;
	}
	var hours = date.getHours();
	if (hours < 12) {
		var suffix = "A";
	} else {
		var suffix = "P";
	}
	if (hours == 0) {
		hours = 12;
	} else if (hours > 12) {
		hours -= 12;
	}
	return hours + ":" + minutes + suffix;
}

function dateString( date ) {

	
	

	return month_names_internal[date.getMonth()] +  
		" " + date.getDate() + ", " + date.getFullYear();
}

function getAttr(rdf_datasource,service,attr_name) {
	var attr = rdf_datasource.GetTarget(service,
					 RDFService.GetResource(rdf_ns + attr_name),
					 true);
	if (attr)
		attr = attr.QueryInterface(
					Components.interfaces.nsIRDFLiteral);
	if (attr)
			attr = attr.Value;
	return attr;
}

function ReadAndSort () {
	if ( ! haveRead ) {
		
		var enumerator = appts_container.GetElements();

		
		allAppts = new Array();
	
		try {
			while (enumerator.hasMoreElements()) {
				var service = enumerator.getNext().QueryInterface(
												Components.interfaces.nsIRDFResource);
		
				
				var title = getAttr(rdf_datasource, service, 'title');
		
				
				var descrip = getAttr(rdf_datasource, service, 'notes');
		
				
				var year = getAttr(rdf_datasource, service, 'year');
				var month = getAttr(rdf_datasource, service, 'month');
				var date = getAttr(rdf_datasource, service, 'date');
				var hour = getAttr(rdf_datasource, service, 'hour');
				var minute = getAttr(rdf_datasource, service, 'minute');

				
				var theid = service.Value;
				
				theid = theid.substring(theid.lastIndexOf("#") + 1);

				var duration = getAttr(rdf_datasource, service, 'duration');
				
				var apptDate = new Date(year, month - 1, date, hour, minute);

				allAppts[allAppts.length] = new Appointment( apptDate, title, descrip, duration, theid );
		}
		} catch (ex) {
			window.alert("Caught exception [[" + ex + "]]\n");
		}
	haveRead = true;
	}

	var todaysAppts = new Array();

	for ( var i = 0 ; i < allAppts.length ; i++ ) {
		var appt = allAppts[i];
		if ( (appt.date.getFullYear() == nowShowing.getFullYear())
			&& (appt.date.getMonth() == nowShowing.getMonth())
			&& (appt.date.getDate() == nowShowing.getDate()) ) {

		todaysAppts[todaysAppts.length] = appt;
		}
	}

	
	
	todaysAppts.sort( compareAppt );

	return todaysAppts;
	}

function ShowSchedule() {
	
	var outTB = getOutputTBody();
	var outDate = getOutputForDate();

	
	while (outTB.hasChildNodes() ) {
		outTB.removeChild(outTB.firstChild);
	}
	while (outDate.hasChildNodes() ) {
		outDate.removeChild(outDate.firstChild);
	}

	
	outDate.appendChild(document.createTextNode(dateString(nowShowing)));

	

	
	var newrow = outTB.insertRow(outTB.rows.length);

	
	

	var todaysAppts = ReadAndSort();

	for ( var i = 0 ; i < todaysAppts.length ; i++ ) {
		var appt = todaysAppts[i];

		

		var newrow = outTB.insertRow(outTB.rows.length);

		
		newrow.setAttribute("id", appt.id);
		
		newrow.addEventListener("click", handleRClick, false, false);

		
		var titlecell = newrow.insertCell(1);
		var timecell = newrow.insertCell(0);
		timecell.appendChild(document.createTextNode(timeString(appt.date)));
		titlecell.appendChild(document.createTextNode(appt.title));
	}
}







function monthString( date ) {
	
	
	return month_names_internal[date.getMonth()] +  
		" " + date.getFullYear();
}

function ShowPick() {
	var pickDiv = getPickDiv(); 

	var outCal = GetElementWithIdIn("calendar", pickDiv);
	var outDate = GetElementWithIdIn("MonthOutput", pickDiv);



	
	while (outCal.hasChildNodes() ) {
		outCal.removeChild(outCal.firstChild);
	}
	while (outDate.hasChildNodes() ) {
		outDate.removeChild(outDate.firstChild);
	}



	
	outDate.appendChild(document.createTextNode(monthString(monthShowing)));


	


	
	var myrow = outCal.insertRow(0);


	
	


	var curDay = new Date(monthShowing); 
	curDay.setDate(1); 



	myrow = outCal.insertRow(outCal.rows.length);
	var junkcell = myrow.insertCell(0); 
	for (var i = 0 ; i < curDay.getDay() ; i++ ) {
		myrow.insertCell(myrow.cells.length);
		}



	var mycell;
	var targMonth = monthShowing.getMonth(); 
	
	var testNowShowing = (
		( monthShowing.getFullYear() == nowShowing.getFullYear()) &&
		( monthShowing.getMonth() == nowShowing.getMonth()) );
	var testCurrentTime = (
		( monthShowing.getFullYear() == currentTime.getFullYear()) &&
		( monthShowing.getMonth() == currentTime.getMonth()) );

	while ( curDay.getMonth() == targMonth ) {
		if ( ( curDay.getDay() == 0) && (curDay.getDate() != 1) ) {
			
			
			myrow = outCal.insertRow(outCal.rows.length);
			junkcell = myrow.insertCell(0); 
		}
		mycell = myrow.insertCell( myrow.cells.length );
		
		mycell.appendChild( document.createTextNode( curDay.getDate() ) );
		
		mycell.setAttribute("onclick", "pickDate(" + curDay.getFullYear()
		                               + "," + (curDay.getMonth() + 1)
									   + "," + curDay.getDate()
									   + ")");
		
		var classNm = "caldate";
		if ( testNowShowing && ( curDay.getDate() == nowShowing.getDate())) {
			classNm += " nowshowing";
			}
		if ( testCurrentTime && ( curDay.getDate() == currentTime.getDate())) {
			classNm += " today";
			}
		mycell.setAttribute("class", classNm);
		
		curDay.setDate(curDay.getDate() + 1);
	}

}






function makeAssert ( source, data, name ) {
	var target = RDFService.GetLiteral(data);
	var arc = RDFService.GetResource(rdf_ns + name);
	rdf_datasource.Assert(source, arc, target, true);
}

function SendBackToSource( appt ) {

	

	
	
	
	
	

	
	
	var sourceNode
		= RDFService.GetResource(RDFFileName +"#" + appt.id);

	
	

	makeAssert( sourceNode, appt.date.getFullYear(), 'year');
	makeAssert( sourceNode, appt.date.getMonth() + 1, 'month'); 
	makeAssert( sourceNode, appt.date.getDate(), 'date');
	makeAssert( sourceNode, appt.date.getHours(), 'hour');
	makeAssert( sourceNode, appt.date.getMinutes(), 'minute');
	makeAssert( sourceNode, appt.title, 'title');
	makeAssert( sourceNode, appt.desc, 'notes');
	makeAssert( sourceNode, appt.dur, 'duration');

	

	appts_container.AppendElement( sourceNode );

	

	try {
		rdf_remote_datasource.Flush(); 
	} catch (ex) {
		window.alert("Caught exception [[" + ex + "]] in Flush().\n");
	}

}

function Add( appt ) {

	
	allAppts[allAppts.length] = appt;

	
	SendBackToSource( appt );

}

function ResetAddFormTo( year, month, date, hrs, min, durhrs, durmin, title, notes) {
	var addDiv = getAddDiv(); 

	var yearselect = GetElementWithIdIn("addyear", addDiv );
	var monthselect = GetElementWithIdIn("addmonth", addDiv );
	var dayselect = GetElementWithIdIn("addday", addDiv );
	var hourselect = GetElementWithIdIn("addtimehrs", addDiv );
	var minselect = GetElementWithIdIn("addtimemin", addDiv );
	var durhourselect = GetElementWithIdIn("adddurhrs", addDiv );
	var durminselect = GetElementWithIdIn("adddurmin", addDiv );

	var titleInput = GetElementWithIdIn("addtitle", addDiv );
	var notesInput = GetElementWithIdIn("addnotes", addDiv );

	if (yearselect != null) {  
		
		
		yearselect.selectedIndex =
			year - yearselect.options.item(0).value;
	} else {
		window.alert("Hit a null.");
	}

	if (monthselect != null) {
		
    	monthselect.selectedIndex = month - 1;
	} else {
		window.alert("Hit a null.");
	}

	if (dayselect != null) {
		
		dayselect.selectedIndex = date - 1;
	} else {
		window.alert("Hit a null.");
	}

	if (hourselect != null) {
		
		hourselect.selectedIndex = hrs ;
	} else {
		window.alert("Hit a null.");
	}

	if (minselect != null) {
		minselect.selectedIndex = Math.round(min / 5);
	} else {
		window.alert("Hit a null.");
	}

	if (durhourselect != null) {
		durhourselect.selectedIndex = durhrs;
	} else {
		window.alert("Hit a null.");
	}

	if (durminselect != null) {
		durminselect.selectedIndex = Math.round(durmin / 5);
	} else {
		window.alert("Hit a null.");
	}

	if (titleInput != null) {
		if ( title == null) {
			title = "";
		}
		titleInput.value = title;
	} else {
		window.alert("Hit a null.");
	}

	if (notesInput != null) {
		if ( notes == null) {
			notes = "";
		}
		notesInput.value = notes;
	} else {
		window.alert("Hit a null.");
	}

}

function ResetAddForm() {
	editButtonsActive(0);
	ResetAddFormTo( nowShowing.getFullYear(),
	                nowShowing.getMonth() + 1,
                    nowShowing.getDate(),
					nowShowing.getHours() + 1, 
					0, 
					0, 
					0, 
					"", 
					""); 
}

function SubmitAddForm() {
	var addDiv = getAddDiv(); 

	var title = GetElementWithIdIn("addtitle", addDiv );
	var years = GetElementWithIdIn("addyear", addDiv );
	var months = GetElementWithIdIn("addmonth", addDiv );
	var days = GetElementWithIdIn("addday", addDiv );
	var timehrs = GetElementWithIdIn("addtimehrs", addDiv );
	var timemin = GetElementWithIdIn("addtimemin", addDiv );
	var durhrs = GetElementWithIdIn("adddurhrs", addDiv );
	var durmin = GetElementWithIdIn("adddurmin", addDiv );
	var notes = GetElementWithIdIn("addnotes", addDiv );

	if ( ( title == null ) ||
		( years == null ) ||
		( months == null ) ||
		( days == null ) ||
		( timehrs == null ) ||
		( timemin == null ) ||
		( durhrs == null ) ||
		( durmin == null ) ||
		( notes == null ) ) {
		window.alert("Something is null.  Addition failed.\n");
	} else {
		
		var apptid = "appt" + (new Date().getTime());
		Add( new Appointment( new Date ( years.value, months.value - 1,
		                                 days.value, timehrs.value,
										 timemin.value ),
		                      title.value,
							  notes.value,
							  (durhrs.value * 60) + (durmin.value * 1),
							  apptid));
		
		divActive(1);
	}
}





function getApptById( id ) {
	for (var i = 0 ; i < allAppts.length ; i++ ) {
		if ( allAppts[i].id == id ) {
			return allAppts[i];
		}
	}
	return null;
}

function removeApptById ( id ) {
	for (var i = 0 ; i < allAppts.length ; i++ ) {
		if ( allAppts[i].id == id ) {
			
			allAppts[i] = allAppts[allAppts.length - 1];
			allAppts[allAppts.length - 1] = null;
			allAppts.length -= 1;
			return;
		}
	}
}

function EditApptById( theid ) {
	idEditing = theid; 

	var apptEditing = getApptById( idEditing );
	if ( apptEditing == null) {
		window.alert("Null appointment.  Something's wrong.\n");
		return;
		}
	divActive(0); 
	editButtonsActive(1);
	ResetAddFormTo( apptEditing.date.getFullYear(),
	                apptEditing.date.getMonth() + 1,
					apptEditing.date.getDate(),
					apptEditing.date.getHours(),
					apptEditing.date.getMinutes(),
					Math.floor(apptEditing.dur / 60),
					apptEditing.dur % 60,
					apptEditing.title,
					apptEditing.desc );
}

function getAttrLiteral(rdf_datasource,service,attr_name) {
	var attr = rdf_datasource.GetTarget(service,
					 RDFService.GetResource(rdf_ns + attr_name),
					 true);
	if (attr)
		attr = attr.QueryInterface(
					Components.interfaces.nsIRDFLiteral);
	return attr;
}

function makeUnAssert ( rdf_datasource, resource, name ) {
	var target = getAttrLiteral(rdf_datasource, resource, name);
	var data = getAttr(rdf_datasource, resource, name); 
	var arc = RDFService.GetResource(rdf_ns + name);
	rdf_datasource.Unassert(resource, arc, target);
}

function DeleteEditing() {
	
	removeApptById(idEditing);

	
	var sourceNode = RDFService.GetResource(RDFFileName +"#" + idEditing);
	appts_container.RemoveElement( sourceNode, true );

	
	makeUnAssert(rdf_datasource, sourceNode, 'year');
	makeUnAssert(rdf_datasource, sourceNode, 'month');
	makeUnAssert(rdf_datasource, sourceNode, 'date');
	makeUnAssert(rdf_datasource, sourceNode, 'hour');
	makeUnAssert(rdf_datasource, sourceNode, 'minute');
	makeUnAssert(rdf_datasource, sourceNode, 'title');
	makeUnAssert(rdf_datasource, sourceNode, 'notes');
	makeUnAssert(rdf_datasource, sourceNode, 'duration');

	try {
		rdf_remote_datasource.Flush(); 
	} catch (ex) {
		window.alert("Caught exception [[" + ex + "]] in Flush().\n");
	}

}

function EditFormSubmit() {
	DeleteEditing();
	SubmitAddForm();
	divActive(1);
}

function EditFormDelete() {
	DeleteEditing();
	divActive(1);
}

function EditFormCancel() {
	divActive(1);
}





function Init()
{
	

	
	
	
	
	try {
		
		
		
		rdf_datasource = Components.classes['@mozilla.org/rdf/datasource;1?name=xml-datasource'].createInstance();
		rdf_datasource = rdf_datasource.QueryInterface(Components.interfaces.nsIRDFDataSource);

		rdf_remote_datasource = rdf_datasource.QueryInterface(Components.interfaces.nsIRDFRemoteDataSource);
		rdf_remote_datasource.Init(RDFFileName); 

		dump("Reading datasource synchronously.\n");
		
		rdf_remote_datasource.Refresh(true);
	}
	catch (ex) {
		
		
		dump("Datasource already read.  Grabbing.\n");
		rdf_datasource = RDFService.GetDataSource(RDFFileName);
	}

	
	
	
	
	appts_container = Components.classes['@mozilla.org/rdf/container;1'].createInstance();
	appts_container = appts_container.QueryInterface(Components.interfaces.nsIRDFContainer);

	appts_resource = RDFService.GetResource(appts_resourcename);
	appts_container.Init(rdf_datasource, appts_resource);
}

function Boot() {

	if (document.getElementById("ApptsWindow")) {
dump("Calling init:\n");
		Init();

		
		

		
dump("Calling divActive(1):\n");
		divActive(1);
dump("Done initializing.\n");
		
	} else {
		setTimeout("Boot()", 0);
	}
}
