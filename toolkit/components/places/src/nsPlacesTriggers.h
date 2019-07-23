





































#ifndef __nsPlacesTriggers_h__
#define __nsPlacesTriggers_h__






#define CREATE_KEYWORD_VALIDITY_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TRIGGER moz_bookmarks_beforedelete_v1_trigger " \
  "BEFORE DELETE ON moz_bookmarks FOR EACH ROW " \
  "WHEN OLD.keyword_id NOT NULL " \
  "BEGIN " \
    "DELETE FROM moz_keywords " \
    "WHERE id = OLD.keyword_id " \
    "AND NOT EXISTS ( " \
      "SELECT id " \
      "FROM moz_bookmarks " \
      "WHERE keyword_id = OLD.keyword_id " \
      "AND id <> OLD.id " \
      "LIMIT 1 " \
    ");" \
  "END" \
)






#define CREATE_PLACES_VIEW_INSERT_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY TRIGGER moz_places_view_insert_trigger " \
  "INSTEAD OF INSERT " \
  "ON moz_places_view " \
  "BEGIN " \
    "INSERT INTO moz_places_temp ( " \
      "id, url, title, rev_host, visit_count, hidden, typed, favicon_id, " \
      "frecency " \
    ") " \
    "VALUES (MAX((SELECT IFNULL(MAX(id), 0) FROM moz_places_temp), " \
                "(SELECT IFNULL(MAX(id), 0) FROM moz_places)) + 1, " \
            "NEW.url, NEW.title, NEW.rev_host, " \
            "IFNULL(NEW.visit_count, 0), " /* enforce having a value */ \
            "NEW.hidden, NEW.typed, NEW.favicon_id, NEW.frecency);" \
  "END" \
)






#define CREATE_PLACES_VIEW_DELETE_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY TRIGGER moz_places_view_delete_trigger " \
  "INSTEAD OF DELETE " \
  "ON moz_places_view " \
  "BEGIN " \
    "DELETE FROM moz_places_temp " \
    "WHERE id = OLD.id; " \
    "DELETE FROM moz_places " \
    "WHERE id = OLD.id; " \
  "END" \
)









#define CREATE_PLACES_VIEW_UPDATE_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY TRIGGER moz_places_view_update_trigger " \
  "INSTEAD OF UPDATE " \
  "ON moz_places_view " \
  "BEGIN " \
    "INSERT OR IGNORE INTO moz_places_temp " \
    "SELECT * " \
    "FROM moz_places " \
    "WHERE id = OLD.id; " \
    "UPDATE moz_places_temp " \
    "SET url = IFNULL(NEW.url, OLD.url), " \
        "title = IFNULL(NEW.title, OLD.title), " \
        "rev_host = IFNULL(NEW.rev_host, OLD.rev_host), " \
        "visit_count = IFNULL(NEW.visit_count, OLD.visit_count), " \
        "hidden = IFNULL(NEW.hidden, OLD.hidden), " \
        "typed = IFNULL(NEW.typed, OLD.typed), " \
        "favicon_id = IFNULL(NEW.favicon_id, OLD.favicon_id), " \
        "frecency = IFNULL(NEW.frecency, OLD.frecency) " \
    "WHERE id = OLD.id; " \
  "END" \
)









#define CREATE_HISTORYVISITS_VIEW_INSERT_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY TRIGGER moz_historyvisits_view_insert_trigger " \
  "INSTEAD OF INSERT " \
  "ON moz_historyvisits_view " \
  "BEGIN " \
    "INSERT INTO moz_historyvisits_temp ( " \
      "id, from_visit, place_id, visit_date, visit_type, session " \
    ") " \
    "VALUES (MAX((SELECT IFNULL(MAX(id), 0) FROM moz_historyvisits_temp), " \
                "(SELECT IFNULL(MAX(id), 0) FROM moz_historyvisits)) + 1, " \
            "NEW.from_visit, NEW.place_id, NEW.visit_date, NEW.visit_type, " \
            "NEW.session); " \
    "INSERT OR IGNORE INTO moz_places_temp " \
    "SELECT * " \
    "FROM moz_places " \
    "WHERE id = NEW.place_id " \
    "AND NEW.visit_type NOT IN (0, 4, 7); " \
    "UPDATE moz_places_temp " \
    "SET visit_count = visit_count + 1 " \
    "WHERE id = NEW.place_id " \
    "AND NEW.visit_type NOT IN (0, 4, 7); " /* invalid, EMBED, DOWNLOAD */ \
  "END" \
)









#define CREATE_HISTORYVISITS_VIEW_DELETE_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY TRIGGER moz_historyvisits_view_delete_trigger " \
  "INSTEAD OF DELETE " \
  "ON moz_historyvisits_view " \
  "BEGIN " \
    "DELETE FROM moz_historyvisits_temp " \
    "WHERE id = OLD.id; " \
    "DELETE FROM moz_historyvisits " \
    "WHERE id = OLD.id; " \
    "INSERT OR IGNORE INTO moz_places_temp " \
    "SELECT * " \
    "FROM moz_places " \
    "WHERE id = OLD.place_id " \
    "AND OLD.visit_type NOT IN (0, 4, 7); " \
    "UPDATE moz_places_temp " \
    "SET visit_count = visit_count - 1 " \
    "WHERE id = OLD.place_id " \
    "AND OLD.visit_type NOT IN (0, 4, 7); " /* invalid, EMBED, DOWNLOAD */ \
  "END" \
)









#define CREATE_HISTORYVISITS_VIEW_UPDATE_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY TRIGGER moz_historyvisits_view_update_trigger " \
  "INSTEAD OF UPDATE " \
  "ON moz_historyvisits_view " \
  "BEGIN " \
    "INSERT OR IGNORE INTO moz_historyvisits_temp " \
    "SELECT * " \
    "FROM moz_historyvisits " \
    "WHERE id = OLD.id; " \
    "UPDATE moz_historyvisits_temp " \
    "SET from_visit = IFNULL(NEW.from_visit, OLD.from_visit), " \
        "place_id = IFNULL(NEW.place_id, OLD.place_id), " \
        "visit_date = IFNULL(NEW.visit_date, OLD.visit_date), " \
        "visit_type = IFNULL(NEW.visit_type, OLD.visit_type), " \
        "session = IFNULL(NEW.session, OLD.session) " \
    "WHERE id = OLD.id; " \
  "END" \
)









#define CREATE_TEMP_SYNC_TRIGGER_BASE(__table) NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY TRIGGER " __table "_beforedelete_trigger " \
  "BEFORE DELETE ON " __table "_temp FOR EACH ROW " \
  "BEGIN " \
    "INSERT OR REPLACE INTO " __table " " \
    "SELECT * FROM " __table "_temp " \
    "WHERE id = OLD.id;" \
  "END" \
)
#define CREATE_MOZ_PLACES_SYNC_TRIGGER \
  CREATE_TEMP_SYNC_TRIGGER_BASE("moz_places")
#define CREATE_MOZ_HISTORYVISITS_SYNC_TRIGGER \
  CREATE_TEMP_SYNC_TRIGGER_BASE("moz_historyvisits")

#endif 
