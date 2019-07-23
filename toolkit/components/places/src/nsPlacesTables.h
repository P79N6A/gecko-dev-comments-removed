






































#ifndef __nsPlacesTables_h__
#define __nsPlacesTables_h__

#define CREATE_MOZ_PLACES_BASE(__name, __temporary) NS_LITERAL_CSTRING( \
  "CREATE " __temporary " TABLE " __name " ( " \
    "  id INTEGER PRIMARY KEY" \
    ", url LONGVARCHAR" \
    ", title LONGVARCHAR" \
    ", rev_host LONGVARCHAR" \
    ", visit_count INTEGER DEFAULT 0" \
    ", hidden INTEGER DEFAULT 0 NOT NULL" \
    ", typed INTEGER DEFAULT 0 NOT NULL" \
    ", favicon_id INTEGER" \
    ", frecency INTEGER DEFAULT -1 NOT NULL" \
    ", last_visit_date INTEGER " \
  ")" \
)
#define CREATE_MOZ_PLACES CREATE_MOZ_PLACES_BASE("moz_places", "")
#define CREATE_MOZ_PLACES_TEMP CREATE_MOZ_PLACES_BASE("moz_places_temp", "TEMP")
#define MOZ_PLACES_COLUMNS \
  "id, url, title, rev_host, visit_count, hidden, typed, favicon_id, " \
  "frecency, last_visit_date"
#define CREATE_MOZ_PLACES_VIEW NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY VIEW moz_places_view AS " \
  "SELECT " MOZ_PLACES_COLUMNS " FROM moz_places_temp " \
  "UNION ALL " \
  "SELECT " MOZ_PLACES_COLUMNS " FROM moz_places " \
  "WHERE id NOT IN (SELECT id FROM moz_places_temp) " \
)


#define CREATE_MOZ_HISTORYVISITS_BASE(__name, __temporary) NS_LITERAL_CSTRING( \
  "CREATE " __temporary " TABLE " __name " (" \
    "  id INTEGER PRIMARY KEY" \
    ", from_visit INTEGER" \
    ", place_id INTEGER" \
    ", visit_date INTEGER" \
    ", visit_type INTEGER" \
    ", session INTEGER" \
  ")" \
)
#define CREATE_MOZ_HISTORYVISITS \
  CREATE_MOZ_HISTORYVISITS_BASE("moz_historyvisits", "")
#define CREATE_MOZ_HISTORYVISITS_TEMP \
  CREATE_MOZ_HISTORYVISITS_BASE("moz_historyvisits_temp", "TEMP")
#define MOZ_HISTORYVISITS_COLUMNS \
  "id, from_visit, place_id, visit_date, visit_type, session"
#define CREATE_MOZ_HISTORYVISITS_VIEW NS_LITERAL_CSTRING( \
  "CREATE TEMPORARY VIEW moz_historyvisits_view AS " \
  "SELECT " MOZ_HISTORYVISITS_COLUMNS " FROM moz_historyvisits_temp " \
  "UNION ALL " \
  "SELECT " MOZ_HISTORYVISITS_COLUMNS " FROM moz_historyvisits " \
  "WHERE id NOT IN (SELECT id FROM moz_historyvisits_temp) " \
)

#define CREATE_MOZ_INPUTHISTORY NS_LITERAL_CSTRING( \
  "CREATE TABLE moz_inputhistory (" \
    "  place_id INTEGER NOT NULL" \
    ", input LONGVARCHAR NOT NULL" \
    ", use_count INTEGER" \
    ", PRIMARY KEY (place_id, input)" \
  ")" \
)

#define CREATE_MOZ_ANNOS NS_LITERAL_CSTRING( \
  "CREATE TABLE moz_annos (" \
    "  id INTEGER PRIMARY KEY" \
    ", place_id INTEGER NOT NULL" \
    ", anno_attribute_id INTEGER" \
    ", mime_type VARCHAR(32) DEFAULT NULL" \
    ", content LONGVARCHAR" \
    ", flags INTEGER DEFAULT 0" \
    ", expiration INTEGER DEFAULT 0" \
    ", type INTEGER DEFAULT 0" \
    ", dateAdded INTEGER DEFAULT 0" \
    ", lastModified INTEGER DEFAULT 0" \
  ")" \
)

#define CREATE_MOZ_ANNO_ATTRIBUTES NS_LITERAL_CSTRING( \
  "CREATE TABLE moz_anno_attributes (" \
    "  id INTEGER PRIMARY KEY" \
    ", name VARCHAR(32) UNIQUE NOT NULL" \
  ")" \
)

#define CREATE_MOZ_ITEMS_ANNOS NS_LITERAL_CSTRING( \
  "CREATE TABLE moz_items_annos (" \
    "  id INTEGER PRIMARY KEY" \
    ", item_id INTEGER NOT NULL" \
    ", anno_attribute_id INTEGER" \
    ", mime_type VARCHAR(32) DEFAULT NULL" \
    ", content LONGVARCHAR" \
    ", flags INTEGER DEFAULT 0" \
    ", expiration INTEGER DEFAULT 0" \
    ", type INTEGER DEFAULT 0" \
    ", dateAdded INTEGER DEFAULT 0" \
    ", lastModified INTEGER DEFAULT 0" \
  ")" \
)

#define CREATE_MOZ_FAVICONS NS_LITERAL_CSTRING( \
  "CREATE TABLE moz_favicons (" \
    "  id INTEGER PRIMARY KEY" \
    ", url LONGVARCHAR UNIQUE" \
    ", data BLOB" \
    ", mime_type VARCHAR(32)" \
    ", expiration LONG" \
  ")" \
)

#define CREATE_MOZ_BOOKMARKS NS_LITERAL_CSTRING( \
  "CREATE TABLE moz_bookmarks (" \
    "  id INTEGER PRIMARY KEY" \
    ", type INTEGER" \
    ", fk INTEGER DEFAULT NULL" /* place_id */ \
    ", parent INTEGER" \
    ", position INTEGER" \
    ", title LONGVARCHAR" \
    ", keyword_id INTEGER" \
    ", folder_type TEXT" \
    ", dateAdded INTEGER" \
    ", lastModified INTEGER" \
  ")" \
)

#define CREATE_MOZ_BOOKMARKS_ROOTS NS_LITERAL_CSTRING( \
  "CREATE TABLE moz_bookmarks_roots (" \
    "  root_name VARCHAR(16) UNIQUE" \
    ", folder_id INTEGER" \
  ")" \
)

#define CREATE_MOZ_KEYWORDS NS_LITERAL_CSTRING( \
  "CREATE TABLE moz_keywords (" \
    "  id INTEGER PRIMARY KEY AUTOINCREMENT" \
    ", keyword TEXT UNIQUE" \
  ")" \
)

#endif 
