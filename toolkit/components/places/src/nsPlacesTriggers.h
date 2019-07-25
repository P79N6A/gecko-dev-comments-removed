






































#include "nsPlacesTables.h"

#ifndef __nsPlacesTriggers_h__
#define __nsPlacesTriggers_h__








#define EXCLUDED_VISIT_TYPES "0, 4, 7, 8"





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





#define CREATE_HISTORYVISITS_AFTERINSERT_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TRIGGER moz_historyvisits_afterinsert_v2_trigger " \
  "AFTER INSERT ON moz_historyvisits FOR EACH ROW " \
  "BEGIN " \
    "UPDATE moz_places SET " \
      "visit_count = visit_count + (SELECT NEW.visit_type NOT IN (" EXCLUDED_VISIT_TYPES ")), "\
      "last_visit_date = MAX(IFNULL(last_visit_date, 0), NEW.visit_date) " \
    "WHERE id = NEW.place_id;" \
  "END" \
)

#define CREATE_HISTORYVISITS_AFTERDELETE_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TRIGGER moz_historyvisits_afterdelete_v2_trigger " \
  "AFTER DELETE ON moz_historyvisits FOR EACH ROW " \
  "BEGIN " \
    "UPDATE moz_places SET " \
      "visit_count = visit_count - (SELECT OLD.visit_type NOT IN (" EXCLUDED_VISIT_TYPES ")), "\
      "last_visit_date = (SELECT visit_date FROM moz_historyvisits " \
                         "WHERE place_id = OLD.place_id " \
                         "ORDER BY visit_date DESC LIMIT 1) " \
    "WHERE id = OLD.place_id;" \
  "END" \
)







#define CREATE_REMOVEOPENPAGE_CLEANUP_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY TRIGGER moz_openpages_temp_afterupdate_trigger " \
  "AFTER UPDATE OF open_count ON moz_openpages_temp FOR EACH ROW " \
  "WHEN NEW.open_count = 0 " \
  "BEGIN " \
    "DELETE FROM moz_openpages_temp " \
    "WHERE url = NEW.url;" \
  "END" \
)

#endif 
