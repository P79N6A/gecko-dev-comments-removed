






































var gAdvancedJSPane = {

  _exceptionsParams: {
    moveresize: {
      blockVisible: false,
      sessionVisible: false,
      allowVisible: true,
      prefilledHost: "",
      permissionType: "moveresize"
    }
  },

  _showExceptions: function(aPermissionType) {
    var bundlePreferences = document.getElementById("preferencesBundle");
    var params = this._exceptionsParams[aPermissionType];
    params.windowTitle = bundlePreferences.getString(aPermissionType + "permissionstitle");
    params.introText = bundlePreferences.getString(aPermissionType + "permissionstext");
    document.documentElement.openWindow("Browser:Permissions",
                                        "chrome://browser/content/preferences/permissions.xul",
                                        "", params);
  },

  showRaiseExceptions: function() {
    this._showExceptions("moveresize");
  }
};
