





































 const FIREFOX_ID = "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}";
 
 var litmus = {
	baseURL : qaPref.getPref(qaPref.prefBase+".litmus.url", "char"),
	
	getTestcase : function(testcase_id, callback) {
        litmus.getLitmusJson(testcase_id, callback, "testcase_id=");
	},
	getSubgroup : function(subgroupID, callback) {
        litmus.getLitmusJson(subgroupID, callback, "subgroup_id=");
    },
    getTestgroup : function(testgroupID, callback) {
        litmus.getLitmusJson(testgroupID, callback, "testgroup_id=");
    },
    getTestrun : function(testrunID, callback) {
        litmus.getLitmusJson(testrunID, callback, "test_run_id=");
    },
    getTestruns : function(callback) {
        var s = new Sysconfig(); 
        var branch = encodeURIComponent(s.branch);
        litmus.getLitmusJson("&product_name=Firefox&branch_name=" + branch,
                        callback, "test_runs_by_branch_product_name=1");
    },
    getLitmusJson : function(ID, callback, prefix) {
        var url = litmus.baseURL+'json.cgi?' + prefix + ID;
        var d = loadJSONDoc(url);
		d.addBoth(function (res) { 
			d.deferred = null;  
			return res; 
		});
		d.addCallback(callback);
		d.addErrback(function (err) { 
            if (err instanceof CancelledError) { 
                return; 
            }
            alert(err);
        });
    },
    
    
    
    lastTestRunSummary : "",
    lastTestGroupSummary : "",
    lastSubgroupObject: null,    
    lastTestcaseObject: null,    
    lastTestcaseIndex: null,
    
    preDialogSubgroupObject: null,  
    preDialogTestcaseObject: null,
    
    storeSubgroup : function(subgroup) {
        litmus.lastSubgroupObject = subgroup;
    },
    storeTestcase : function(testcase) {
        litmus.lastTestcaseObject = testcase;
    },
    
    handleDialog : function() {
        if ($("qa-testrun-label").value != "") {
            litmus.lastTestRunSummary = $("qa-testrun-label").value;
            litmus.lastTestGroupSummary = $("qa-testgroup-label").value;
            lastTestcaseIndex = $("testlist").selectedIndex;
        }
        litmus.preDialogueSubgroupObject = litmus.lastSubgroupObject;
        litmus.preDialogTestcaseObject = litmus.lastTestcaseObject;
        var newWindow = window.openDialog('chrome://qa/content/tabs/selecttests.xul', '_blank', 'chrome,all,dialog=yes',
                                          litmus.readStateFromPref, litmus.handleDialogCancel);
    },
    
    handleDialogCancel : function() {
        if (litmus.lastTestRunSummary == "") return;
        
        
        
        $("qa-testrun-label").value = litmus.lastTestRunSummary;
        $("qa-testgroup-label").value = litmus.lastTestGroupSummary;
        litmus.lastSubgroupObject = litmus.preDialogueSubgroupObject;
        litmus.lastTestcaseObject = litmus.preDialogTestcaseObject;
        
        if (litmus.lastSubgroupObject != null)
            litmus.populatePreviewBox(litmus.lastSubgroupObject.testcases);
        if (litmus.lastTestcaseObject != null) {
            litmus.populateTestcase(litmus.lastTestcaseObject);
        }
        
        $("testlist").setAttribute("suppressonselect", "true");
        $("testlist").selectedIndex = lastTestcaseIndex;
        $("testlist").setAttribute("suppressonselect", "false");
        
        
        litmus.writeStateToPref(litmus.lastTestRunSummary, litmus.lastTestGroupSummary,
                                litmus.lastSubgroupObject.subgroup_id, lastTestcaseIndex);
    },
    
	validateLogin : function(uname, passwd, callback) {
	    var req = doSimpleXMLHttpRequest(litmus.baseURL+'json.cgi', {
	       validate_login: 1,
	       username: uname,
	       password: passwd
	    });
	    req.addErrback(callback);
	    req.addCallback(callback);
	},
	createAccount : function() {
	    alert("XXX: not implemented");
	},
	postResultXML : function(xml, callback, errback) {
		var req = doSimpleXMLHttpRequest(litmus.baseURL+'process_test.cgi', 
										    { data: xml });
		req.addErrback(errback);
		req.addCallback(function(resp) {
			
			
			if ((/ok/i).exec(resp.responseText)) {
				callback(resp);
			} else {
				errback(resp);
			}
		});
	},
    
    currentSubgroupID: null,	
    
    writeStateToPref : function(testrunSummary, testgroupSummary, subgroupID, index) {
        qaPref.setPref(qaPref.prefBase + ".currentTestcase.testrunSummary", testrunSummary, "char");
        qaPref.setPref(qaPref.prefBase + ".currentTestcase.testgroupSummary", testgroupSummary, "char");
        qaPref.setPref(qaPref.prefBase + ".currentTestcase.subgroupId", subgroupID, "int");
        qaPref.setPref(qaPref.prefBase + ".currentTestcase.testcaseIndex", index, "int");
    },
    readStateFromPref : function() {
        $("qa-testrun-label").value = qaPref.getPref(qaPref.prefBase + ".currentTestcase.testrunSummary", "char");
        $("qa-testgroup-label").value = qaPref.getPref(qaPref.prefBase + ".currentTestcase.testgroupSummary", "char");
        litmus.currentSubgroupID = qaPref.getPref(qaPref.prefBase + ".currentTestcase.subgroupId", "int");
        
        if (litmus.currentSubgroupID != 0)
            litmus.getSubgroup(litmus.currentSubgroupID,litmus.statePopulateFields);
    },
    checkRadioButtons : function() {
        var menu = document.getElementById('testlist');
        if (menu.selectedIndex == -1) return;
        var disable = menu.selectedItem.firstChild.getAttribute("checked");
        document.getElementById("qa-testcase-result").disabled = disable;
    },
    
    prevButton : function() {
        $("testlist").selectedIndex--;
        
    },
	nextButton: function() {
		
		if ($('qa-testcase-result').selectedItem) {
			litmus.submitResult();
		}
        $("testlist").selectedIndex++;
		
	},
    handleSelect : function() {
        if ($("testlist").selectedIndex == -1)
            return;
        
        
          
        
        
        litmus.selectCurrentTestCase();
    },
    selectCurrentTestCase : function() {
        
        
        litmus.getTestcase($("testlist").selectedItem.value, litmus.populateTestcase);
    },
	populatePreviewBox : function(testcases) {
        
        var menu = document.getElementById('testlist');
        if (!menu) return;
        
        while (menu.firstChild) {  
            menu.removeChild(menu.firstChild);
        };
    
        for (var i = 0; i < testcases.length; i++) {
            var row = document.createElement("listitem");
            row.value = testcases[i].testcase_id;
            var checkbox = document.createElement("listcell");
            checkbox.setAttribute("label", "");
            checkbox.setAttribute("type", "checkbox");
            checkbox.setAttribute("disabled", "true");
            var name = document.createElement("listcell");
            name.setAttribute("label", "#" + testcases[i].testcase_id + " -- " + testcases[i].summary);
            name.setAttribute("crop", "end");
            name.setAttribute("maxwidth", "175");
            row.appendChild(checkbox);
            row.appendChild(name);
            menu.appendChild(row);
        }
    },
    populateTestcase : function(testcase) {
        litmus.lastTestcaseObject = testcase;
        if (testcase == undefined) {
                return;
            }
        document.getElementById('qa-testcase-id').value = 
            qaMain.bundle.getString("qa.extension.testcase.head")+testcase.testcase_id;
		document.getElementById('qa-testcase-summary').value = testcase.summary;
	
        qaTools.writeSafeHTML('qa-testcase-steps', testcase.steps_formatted);
        qaTools.writeSafeHTML('qa-testcase-expected', testcase.expected_results_formatted);
        
        qaTools.linkTargetsToBlank($('qa-testcase-steps'));
        qaTools.linkTargetsToBlank($('qa-testcase-expected'));
        
        litmus.checkRadioButtons();
    },
	populateFields : function(subgroup) {
        litmus.lastSubgroupObject = subgroup;
		litmus.populatePreviewBox(subgroup.testcases);
        $("testlist").selectedIndex = 0;
        litmus.selectCurrentTestCase();
	},
    statePopulateFields : function(subgroup) {  
        litmus.lastSubgroupObject = subgroup;
        litmus.populatePreviewBox(subgroup.testcases);
        
        $("testlist").selectedIndex = qaPref.getPref(qaPref.prefBase + ".currentTestcase.testcaseIndex", "int");
        litmus.selectCurrentTestCase();
    },
	submitResult : function() {
		var rs;
		var item = $('qa-testcase-result').selectedItem;
		if (item.id == "qa-testcase-pass") {
			rs = 'Pass';
		} else if (item.id == "qa-testcase-fail") {
			rs = 'Fail';
		} else if (item.id == "qa-testcase-unclearBroken") {
			rs = 'Test unclear/broken';
		} else {
			
			return false;
		}

        var menu = document.getElementById('testlist');
        
		var l = new LitmusResults({username: qaPref.litmus.getUsername(), 
								  password: qaPref.litmus.getPassword(),
								  server: litmus.baseURL});
		l.sysconfig(new Sysconfig());
		
		l.addResult(new Result({
			testid: menu.selectedItem.value,
			resultstatus: rs,
			exitstatus: 'Exited Normally',
			duration: 0,
			comment: $('qa-testcase-comment').value,
			isAutomatedResult: 0
		}));
		
		var callback = function(resp) {
			alert("yay");
		};
		
		var errback = function(resp) {
			alert(resp.responseText);
		};
		
		litmus.postResultXML(l.toXML(), callback, errback);        
        var item = menu.selectedItem;
        item.firstChild.setAttribute("checked", "true");
        return false;   
    },
 };


function Sysconfig(aProduct, aPlatform, aOpsys, aBranch, aBuildid, aLocale) {
	this._load('product', aProduct);
	this._load('platform', aPlatform);
	this._load('opsys', aOpsys);
	this._load('branch', aBranch);
	this._load('buildid', aBuildid);
	this._load('locale', aLocale);
	this.populate();
}
 
Sysconfig.prototype = {
	product: null,
	platorm: null,
	opsys: null,
	branch: null,
	buildid: null,
	locale: null,
	
	
	
	
	
	_load: function(fieldname, setting) {
		if (this[fieldname]) { return }
		if (setting) {
			this[fieldname] = setting;
			return;
		} 
		var pref = qaPref.getPref(qaPref.prefBase+'.sysconfig.'+fieldname, 'char');
		if (pref) {
			this[fieldname] = pref;
			return;
		}
	},
	
	
	
	populate: function() {
		var appinfo = Components.classes["@mozilla.org/xre/app-info;1"]
                        .getService(Components.interfaces.nsIXULAppInfo);
                        
        
		this.buildid = appinfo.appBuildID;
		if (! this.buildid) { throw "buildid" }
        
        
        if (! this.product) {
        	if (appinfo.ID == FIREFOX_ID) {
        		this.product = 'Firefox';
        	}
        	if (! this.product) { throw "product" }
        }
        
        
        
        
		if ((/^3\.0/).exec(appinfo.version)) {
			this.branch = '3.0 Branch';
		} else if ((/^2\.0/).exec(appinfo.version)) {
			this.branch = '2.0 Branch';
		} else if ((/^1\.5\./).exec(appinfo.version)) {
			this.branch = '1.5 Branch';
		}
		if (! this.branch) { throw "branch" }
			
        
        if (! this.platform) {
			if ((/^Mac/).exec(navigator.platform)) {
				this.platform = 'Mac';
			} else if ((/^Win/).exec(navigator.platform)) {
				this.platform = 'Windows';
			} else if ((/^Linux/).exec(navigator.platform)) {
				this.platform = 'Linux';
			} else if ((/^Solaris/).exec(navigator.platform)) {
				this.platform = 'Solaris';
			}
			if (! this.platform) { throw "platform" }
        }
        
        if (this.platform == 'Windows') {
        	switch (navigator.oscpu) {
        		case 'Windows NT 5.1':
        			this.opsys = 'Windows XP';
        			break;
        		case 'Windows NT 5.2':
        			this.opsys = 'Windows XP';
        			break;
        		case 'Windows NT 6.0':
        			this.opsys = 'Windows Vista';
        			break;
        		case 'Windows NT 5.0':
        			this.opsys = 'Windows 2000';
        			break;
        		case 'Win 9x 4.90':
        			this.opsys = 'Windows ME';
        			break;
        		case 'Win98':
        			this.opsys = 'Windows 98';
        			break;
        	}
        } else if (this.platform == 'Linux') {
        	this.opsys = 'Linux';
        } else if (this.platform == 'Mac') {
        	
        	
        	this.opsys = 'Mac OS 10.4';
        }
        if (! this.opsys) {throw "opsys" }
        
        
		this.locale = navigator.language;
		if (!this.locale) { throw "locale" }
	},
};
