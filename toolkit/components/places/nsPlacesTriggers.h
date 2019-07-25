






































#include "nsPlacesTables.h"

#ifndef __nsPlacesTriggers_h__
#define __nsPlacesTriggers_h__








#define EXCLUDED_VISIT_TYPES "0, 4, 7, 8"





#define CREATE_HISTORYVISITS_AFTERINSERT_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMP TRIGGER moz_historyvisits_afterinsert_v2_trigger " \
  "AFTER INSERT ON moz_historyvisits FOR EACH ROW " \
  "BEGIN " \
    "UPDATE moz_places SET " \
      "visit_count = visit_count + (SELECT NEW.visit_type NOT IN (" EXCLUDED_VISIT_TYPES ")), "\
      "last_visit_date = MAX(IFNULL(last_visit_date, 0), NEW.visit_date) " \
    "WHERE id = NEW.place_id;" \
  "END" \
)

#define CREATE_HISTORYVISITS_AFTERDELETE_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMP TRIGGER moz_historyvisits_afterdelete_v2_trigger " \
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




#define CREATE_PLACES_AFTERINSERT_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMP TRIGGER moz_places_afterinsert_trigger " \
  "AFTER INSERT ON moz_places FOR EACH ROW " \
  "WHEN LENGTH(NEW.rev_host) > 1 " \
  "BEGIN " \
    "INSERT OR REPLACE INTO moz_hosts (id, host, frecency) " \
    "VALUES (" \
      "(SELECT id FROM moz_hosts WHERE host = fixup_url(get_unreversed_host(NEW.rev_host))), " \
      "fixup_url(get_unreversed_host(NEW.rev_host)), " \
      "MAX((SELECT frecency FROM moz_hosts WHERE host = fixup_url(get_unreversed_host(NEW.rev_host))), NEW.frecency) " \
    "); " \
  "END" \
)

#define CREATE_PLACES_AFTERDELETE_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMP TRIGGER moz_places_afterdelete_trigger " \
  "AFTER DELETE ON moz_places FOR EACH ROW " \
  "BEGIN " \
    "DELETE FROM moz_hosts " \
    "WHERE host = fixup_url(get_unreversed_host(OLD.rev_host)) " \
      "AND NOT EXISTS(SELECT 1 FROM moz_places WHERE rev_host = OLD.rev_host); " \
  "END" \
)





#define CREATE_PLACES_AFTERUPDATE_TRIGGER NS_LITERAL_CSTRING( \
  "CREATE TEMP TRIGGER moz_places_afterupdate_frecency_trigger " \
  "AFTER UPDATE OF frecency ON moz_places FOR EACH ROW " \
  "WHEN NEW.frecency >= 0 " \
    "AND ABS(" \
      "IFNULL((NEW.frecency - OLD.frecency) / CAST(NEW.frecency AS REAL), " \
             "(NEW.frecency - OLD.frecency))" \
    ") > .05 " \
  "BEGIN " \
    "UPDATE moz_hosts " \
    "SET frecency = (SELECT MAX(frecency) FROM moz_places " \
                    "WHERE rev_host = NEW.rev_host " \
                       "OR rev_host = NEW.rev_host || 'www.') " \
    "WHERE host = fixup_url(get_unreversed_host(NEW.rev_host)); " \
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
