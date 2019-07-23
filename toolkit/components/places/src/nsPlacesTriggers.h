





































#ifndef __nsPlacesTriggers_h__
#define __nsPlacesTriggers_h__





#define CREATE_VISIT_COUNT_INSERT_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TRIGGER moz_historyvisits_afterinsert_v1_trigger " \
  "AFTER INSERT ON moz_historyvisits FOR EACH ROW " \
  "WHEN NEW.visit_type NOT IN (0, 4, 7) " /* invalid, EMBED, DOWNLOAD */ \
  "BEGIN " \
    "UPDATE moz_places " \
    "SET visit_count = visit_count + 1 " \
    "WHERE moz_places.id = NEW.place_id; " \
  "END" \
)






#define CREATE_VISIT_COUNT_DELETE_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TRIGGER moz_historyvisits_afterdelete_v1_trigger " \
  "AFTER DELETE ON moz_historyvisits FOR EACH ROW " \
  "WHEN OLD.visit_type NOT IN (0, 4, 7) " /* invalid, EMBED, DOWNLOAD */ \
  "BEGIN " \
    "UPDATE moz_places " \
    "SET visit_count = visit_count - 1 " \
    "WHERE moz_places.id = OLD.place_id " \
    "AND visit_count > 0; " \
  "END" \
)






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

#endif 
