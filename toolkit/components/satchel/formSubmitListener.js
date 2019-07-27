



(function(){

var Cc = Components.classes;
var Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

var satchelFormListener = {
    QueryInterface : XPCOMUtils.generateQI([Ci.nsIFormSubmitObserver,
                                            Ci.nsIDOMEventListener,
                                            Ci.nsIObserver,
                                            Ci.nsISupportsWeakReference]),

    debug          : true,
    enabled        : true,
    saveHttpsForms : true,

    init : function() {
        Services.obs.addObserver(this, "earlyformsubmit", false);
        Services.prefs.addObserver("browser.formfill.", this, false);
        this.updatePrefs();
        addEventListener("unload", this, false);
    },

    updatePrefs : function () {
        this.debug          = Services.prefs.getBoolPref("browser.formfill.debug");
        this.enabled        = Services.prefs.getBoolPref("browser.formfill.enable");
        this.saveHttpsForms = Services.prefs.getBoolPref("browser.formfill.saveHttpsForms");
    },

    
    
    isValidCCNumber : function (ccNumber) {
        
        ccNumber = ccNumber.replace(/[\-\s]/g, '');

        let len = ccNumber.length;
        if (len != 9 && len != 15 && len != 16)
            return false;

        if (!/^\d+$/.test(ccNumber))
            return false;

        let total = 0;
        for (let i = 0; i < len; i++) {
            let ch = parseInt(ccNumber[len - i - 1]);
            if (i % 2 == 1) {
                
                ch *= 2;
                if (ch > 9)
                    ch -= 9;
            }
            total += ch;
        }
        return total % 10 == 0;
    },

    log : function (message) {
        if (!this.debug)
            return;
        dump("satchelFormListener: " + message + "\n");
        Services.console.logStringMessage("satchelFormListener: " + message);
    },

    

    handleEvent: function(e) {
        switch (e.type) {
            case "unload":
                Services.obs.removeObserver(this, "earlyformsubmit");
                Services.prefs.removeObserver("browser.formfill.", this);
                break;

            default:
                this.log("Oops! Unexpected event: " + e.type);
                break;
        }
    },

    

    observe : function (subject, topic, data) {
        if (topic == "nsPref:changed")
            this.updatePrefs();
        else
            this.log("Oops! Unexpected notification: " + topic);
    },

    

    notify : function(form, domWin, actionURI, cancelSubmit) {
        try {
            
            
            
            if (domWin.top != content)
                return;
            if (!this.enabled)
                return;

            if (PrivateBrowsingUtils.isContentWindowPrivate(domWin))
                return;

            this.log("Form submit observer notified.");

            if (!this.saveHttpsForms) {
                if (actionURI.schemeIs("https"))
                    return;
                if (form.ownerDocument.documentURIObject.schemeIs("https"))
                    return;
            }

            if (form.hasAttribute("autocomplete") &&
                form.getAttribute("autocomplete").toLowerCase() == "off")
                return;

            let entries = [];
            for (let i = 0; i < form.elements.length; i++) {
                let input = form.elements[i];
                if (!(input instanceof Ci.nsIDOMHTMLInputElement))
                    continue;

                
                if (!input.mozIsTextField(true))
                    continue;

                
                

                
                if (input.hasAttribute("autocomplete") &&
                    input.getAttribute("autocomplete").toLowerCase() == "off")
                    continue;

                let value = input.value.trim();

                
                if (!value || value == input.defaultValue.trim())
                    continue;

                
                if (this.isValidCCNumber(value)) {
                    this.log("skipping saving a credit card number");
                    continue;
                }

                let name = input.name || input.id;
                if (!name)
                    continue;

                if (name == 'searchbar-history') {
                    this.log('addEntry for input name "' + name + '" is denied')
                    continue;
                }

                
                if (name.length > 200 || value.length > 200) {
                    this.log("skipping input that has a name/value too large");
                    continue;
                }

                
                if (entries.length >= 100) {
                    this.log("not saving any more entries for this form.");
                    break;
                }

                entries.push({ name: name, value: value });
            }

            if (entries.length) {
                this.log("sending entries to parent process for form " + form.id);
                sendAsyncMessage("FormHistory:FormSubmitEntries", entries);
            }
        }
        catch (e) {
            this.log("notify failed: " + e);
        }
    }
};

satchelFormListener.init();

})();
