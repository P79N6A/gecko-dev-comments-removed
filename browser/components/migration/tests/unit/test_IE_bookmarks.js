



function run_test() {
  let migrator = newMigratorFor("ie");

  
  do_check_true(migrator.sourceExists);

  
  let availableSources = migrator.getMigrateData("FieldOfFlowers", false);
  do_check_true((availableSources & IMIGRATOR.BOOKMARKS) > 0);

  
  let startup = {
    doStartup: function () {},
    get directory() do_get_profile()
  }
  migrator.migrate(IMIGRATOR.BOOKMARKS, startup, "FieldOfFlowers");

  
  
  do_check_true(PlacesUtils.bookmarks
                           .getIdForItemAt(PlacesUtils.bookmarksMenuFolderId, 1) > 0);
  do_check_true(PlacesUtils.bookmarks
                           .getIdForItemAt(PlacesUtils.toolbarFolderId, 1) > 0);
}
