


add_task(function* setup() {
  yield setupPlacesDatabase("places_v26.sqlite");
  
  let path = OS.Path.join(OS.Constants.Path.profileDir, DB_FILENAME);
  let db = yield Sqlite.openConnection({ path });
  
  yield db.execute(`INSERT INTO moz_places (url, guid)
                    VALUES ("http://test1.com/", "test1_______")
                         , ("http://test2.com/", "test2_______")
                   `);
  
  yield db.execute(`INSERT INTO moz_keywords (keyword)
                    VALUES ("kw1")
                         , ("kw2")
                         , ("kw3")
                   `);
  
  let now = Date.now() * 1000;
  let index = 0;
  yield db.execute(`INSERT INTO moz_bookmarks (type, fk, parent, position, dateAdded, lastModified, keyword_id, guid)
                    VALUES (1, (SELECT id FROM moz_places WHERE guid = 'test1_______'), 3, ${index++}, ${now}, ${now},
                             (SELECT id FROM moz_keywords WHERE keyword = 'kw1'), "bookmark1___")
                            /* same uri, different keyword */
                         , (1, (SELECT id FROM moz_places WHERE guid = 'test1_______'), 3, ${index++}, ${now}, ${now},
                             (SELECT id FROM moz_keywords WHERE keyword = 'kw2'), "bookmark2___")
                           /* different uri, same keyword as 1 */
                         , (1, (SELECT id FROM moz_places WHERE guid = 'test2_______'), 3, ${index++}, ${now}, ${now},
                             (SELECT id FROM moz_keywords WHERE keyword = 'kw1'), "bookmark3___")
                           /* same uri, same keyword as 1 */
                         , (1, (SELECT id FROM moz_places WHERE guid = 'test1_______'), 3, ${index++}, ${now}, ${now},
                             (SELECT id FROM moz_keywords WHERE keyword = 'kw1'), "bookmark4___")
                           /* same uri, same keyword as 2 */
                         , (1, (SELECT id FROM moz_places WHERE guid = 'test2_______'), 3, ${index++}, ${now}, ${now},
                             (SELECT id FROM moz_keywords WHERE keyword = 'kw2'), "bookmark5___")
                           /* different uri, same keyword as 1 */
                         , (1, (SELECT id FROM moz_places WHERE guid = 'test1_______'), 3, ${index++}, ${now}, ${now},
                             (SELECT id FROM moz_keywords WHERE keyword = 'kw3'), "bookmark6___")
                   `);
  
  yield db.execute(`INSERT INTO moz_anno_attributes (name)
                    VALUES ("bookmarkProperties/POSTData")`);
  yield db.execute(`INSERT INTO moz_items_annos(anno_attribute_id, item_id, content)
                    VALUES ((SELECT id FROM moz_anno_attributes where name = "bookmarkProperties/POSTData"),
                            (SELECT id FROM moz_bookmarks WHERE guid = "bookmark3___"), "postData1")
                         , ((SELECT id FROM moz_anno_attributes where name = "bookmarkProperties/POSTData"),
                            (SELECT id FROM moz_bookmarks WHERE guid = "bookmark5___"), "postData2")`);
  yield db.close();
});

add_task(function* database_is_valid() {
  Assert.equal(PlacesUtils.history.databaseStatus,
               PlacesUtils.history.DATABASE_STATUS_UPGRADED);

  let db = yield PlacesUtils.promiseDBConnection();
  Assert.equal((yield db.getSchemaVersion()), CURRENT_SCHEMA_VERSION);
});

add_task(function* test_keywords() {
  
  
  let [ url1, postData1 ] = PlacesUtils.getURLAndPostDataForKeyword("kw1");
  Assert.equal(url1, "http://test2.com/");
  Assert.equal(postData1, "postData1");
  let [ url2, postData2 ] = PlacesUtils.getURLAndPostDataForKeyword("kw2");
  Assert.equal(url2, "http://test2.com/");
  Assert.equal(postData2, "postData2");
  let [ url3, postData3 ] = PlacesUtils.getURLAndPostDataForKeyword("kw3");
  Assert.equal(url3, "http://test1.com/");

  Assert.equal((yield foreign_count("http://test1.com/")), 5); 
  Assert.equal((yield foreign_count("http://test2.com/")), 4); 
});
