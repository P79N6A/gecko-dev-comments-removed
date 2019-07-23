





































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function pluginBindings() { }

pluginBindings.prototype = {
    
    
    classDescription: "plugin bindings",
    classID:          Components.ID("12663f3a-a311-4606-83eb-b6b9108dcc36"),
    contractID:       "@mozilla.org/plugin-bindings;1",
    QueryInterface: XPCOMUtils.generateQI([]),

    _xpcom_categories: [{ category: "agent-style-sheets",
                          entry:    "pluginfinder xbl binding",
                          value:    "chrome://mozapps/content/plugins/pluginFinderBinding.css"},
                        { category: "agent-style-sheets",
                          entry:    "pluginproblem xbl binding",
                          value:    "chrome://mozapps/content/plugins/pluginProblemBinding.css"}]
};

var NSGetModule = XPCOMUtils.generateNSGetModule([pluginBindings]);
