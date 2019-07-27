var ReflectionHarness = {};


ReflectionHarness.passed = document.getElementById("passed");
ReflectionHarness.failed = document.getElementById("failed");








ReflectionHarness.catchUnexpectedExceptions = true;







ReflectionHarness.stringRep = function(val) {
	if (val === null) {
		
		return "null";
	}
	
	
	if (val === -0 && 1/val === -Infinity) {
		return "-0";
	}
	switch (typeof val) {
		case "string":
			for (var i = 0; i < 32; i++) {
				var replace = "\\";
				switch (i) {
					case 0: replace += "0"; break;
					case 1: replace += "x01"; break;
					case 2: replace += "x02"; break;
					case 3: replace += "x03"; break;
					case 4: replace += "x04"; break;
					case 5: replace += "x05"; break;
					case 6: replace += "x06"; break;
					case 7: replace += "x07"; break;
					case 8: replace += "b"; break;
					case 9: replace += "t"; break;
					case 10: replace += "n"; break;
					case 11: replace += "v"; break;
					case 12: replace += "f"; break;
					case 13: replace += "r"; break;
					case 14: replace += "x0e"; break;
					case 15: replace += "x0f"; break;
					case 16: replace += "x10"; break;
					case 17: replace += "x11"; break;
					case 18: replace += "x12"; break;
					case 19: replace += "x13"; break;
					case 20: replace += "x14"; break;
					case 21: replace += "x15"; break;
					case 22: replace += "x16"; break;
					case 23: replace += "x17"; break;
					case 24: replace += "x18"; break;
					case 25: replace += "x19"; break;
					case 26: replace += "x1a"; break;
					case 27: replace += "x1b"; break;
					case 28: replace += "x1c"; break;
					case 29: replace += "x1d"; break;
					case 30: replace += "x1e"; break;
					case 31: replace += "x1f"; break;
				}
				val = val.replace(String.fromCharCode(i), replace);
			}
			return '"' + val.replace('"', '\\"') + '"';
		case "boolean":
		case "undefined":
		case "number":
			return val + "";
		default:
			return typeof val + ' "' + val + '"';
	}
}





ReflectionHarness.currentTestInfo = {};






ReflectionHarness.testWrapper = function(fn) {
	fn();
}









ReflectionHarness.test = function(expected, actual, description) {
	
	if (expected === 0 && actual === 0 && 1/expected === 1/actual) {
		this.increment(this.passed);
		return true;
	} else if (expected === actual) {
		this.increment(this.passed);
		return true;
	} else {
		this.increment(this.failed);
		this.reportFailure(description + ' (expected ' + this.stringRep(actual) + ', got ' + this.stringRep(expected) + ')');
		return false;
	}
}

ReflectionHarness.run = function(fun, description) {
	try {
		fun();
	} catch (err) {
		ReflectionHarness.failure(description);
	}
}








ReflectionHarness.testException = function(exceptionName, fn, description) {
	try {
		fn();
	} catch (e) {
		if (e instanceof DOMException && e.code == DOMException[exceptionName]) {
			this.increment(this.passed);
			return true;
		}
	}
	this.increment(this.failed);
	this.reportFailure(description);
	return false;
}




ReflectionHarness.getTypeDescription = function() {
	var domNode = this.currentTestInfo.domObj.tagName.toLowerCase();
	var idlNode = this.currentTestInfo.idlObj.nodeName.toLowerCase();
	var domName = this.currentTestInfo.domName;
	var idlName = this.currentTestInfo.idlName;
	var comment = this.currentTestInfo.data.comment;
	var typeDesc = idlNode + "." + idlName;
	if (!comment && (domNode != idlNode || domName != idlName)) {
		comment = "<" + domNode + " " + domName + ">";
	}
	if (comment) {
		typeDesc += " (" + comment + ")";
	}
	return typeDesc;
}







ReflectionHarness.reportFailure = function(description) {
	var typeDesc = this.getTypeDescription();
	var idlName = this.currentTestInfo.idlName;
	var comment = this.currentTestInfo.data.comment;
	typeDesc = typeDesc.replace("&", "&amp;").replace("<", "&lt;");
	description = description.replace("&", "&amp;").replace("<", "&lt;");

	var type = this.currentTestInfo.data.type;

	
	
	if (description.search('^typeof IDL attribute \\(expected ".*", got "undefined"\\)$') != -1) {
		type = "undefined";
	}

	var done = false;
	var ul = document.getElementById("errors-" + type.replace(" ", "-"));
	if (ul === null) {
		ul = document.createElement("ul");
		ul.id = "errors-" + type.replace(" ", "-");
		var div = document.getElementById("errors");
		p = document.createElement("p");
		if (type == "undefined") {
			div.parentNode.insertBefore(ul, div.nextSibling);
			p.innerHTML = "These IDL attributes were of undefined type, presumably representing unimplemented features (cordoned off into a separate section for tidiness):";
		} else {
			div.appendChild(ul);
			p.innerHTML = "Errors for type " + type + ":";
		}
		ul.parentNode.insertBefore(p, ul);
	} else if (type != "undefined") {
		var existingErrors = ul.getElementsByClassName("desc");
		for (var i = 0; i < existingErrors.length; i++) {
			if (existingErrors[i].innerHTML == description) {
				var typeSpan = existingErrors[i].parentNode.getElementsByClassName("type")[0];
				
				
				
				
				
				var types = typeSpan.innerHTML.split(", ");
				var count = 0;
				for (var i = 0; i < types.length; i++) {
					if (types[i].search("^\\([0-9]* elements\\)\\." + idlName + "$") != -1) {
						types[i] = "(" + (1 + parseInt(/[0-9]+/.exec(types[i])[0])) + " elements)." + idlName;
						typeSpan.innerHTML = types.join(", ");
						return;
					} else if (types[i].search("\\." + idlName + "$") != -1) {
						count++;
					}
				}
				if (comment || count < 10) {
					
					
					typeSpan.innerHTML += ", " + typeDesc;
				} else {
					var filteredTypes = types.filter(function(type) { return type.search("\\." + idlName + "$") == -1; });
					if (filteredTypes.length) {
						typeSpan.innerHTML = filteredTypes.join(", ") + ", ";
					} else {
						typeSpan.innerHTML = "";
					}
					typeSpan.innerHTML += "(" + (types.length - filteredTypes.length) + " elements)." + idlName;
				}
				return;
			}
		}
	}

	if (type == "undefined") {
		ul.innerHTML += "<li>" + typeDesc;
	} else {
		ul.innerHTML += "<li><span class=\"type\">" + typeDesc + "</span>: <span class=\"desc\">" + description + "</span>";
	}
}








ReflectionHarness.failure = function(message) {
	this.increment(this.failed);
	this.reportFailure(message);
}







ReflectionHarness.success = function() {
	this.increment(this.passed);
}







ReflectionHarness.increment = function(el) {
	el.innerHTML = parseInt(el.innerHTML) + 1;
	var percent = document.getElementById("percent");
	var passed = document.getElementById("passed");
	var failed = document.getElementById("failed");
	percent.innerHTML = (parseInt(passed.innerHTML)/(parseInt(passed.innerHTML) + parseInt(failed.innerHTML))*100).toPrecision(3);
}








ReflectionHarness.maskErrors = function(regex) {
	var uls = document.getElementsByTagName("ul");
	for (var i = 0; i < uls.length; i++) {
		var lis = uls[i].children;
		for (var j = 0; j < lis.length; j++) {
			if (regex !== "" && lis[j].innerHTML.match(regex)) {
				lis[j].style.display = "none";
			} else {
				lis[j].style.display = "list-item";
			}
		}
	}
}





var elements = {};

var extraTests = [];





function mergeElements(src) {
	for (var key in src) {
		if (!src.hasOwnProperty(key)) {
			
			continue;
		}

		if (key in elements) {
			elements[key] = elements[key].concat(src[key]);
		} else {
			elements[key] = src[key];
		}
	}
}
