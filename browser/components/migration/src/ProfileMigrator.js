



"use strict";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource:///modules/MigrationUtils.jsm");

function ProfileMigrator() {
}

ProfileMigrator.prototype = {
  migrate: MigrationUtils.startupMigration.bind(MigrationUtils),
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIProfileMigrator]),
  classDescription: "Profile Migrator",
  contractID: "@mozilla.org/toolkit/profile-migrator;1",
  classID: Components.ID("6F8BB968-C14F-4D6F-9733-6C6737B35DCE")
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([ProfileMigrator]);
