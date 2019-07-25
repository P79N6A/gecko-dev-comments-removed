






































#ifndef __nsPlacesIndexes_h__
#define __nsPlacesIndexes_h__

#define CREATE_PLACES_IDX(__name, __table, __columns, __type) \
  NS_LITERAL_CSTRING( \
    "CREATE " __type " INDEX IF NOT EXISTS " __table "_" __name \
      " ON " __table " (" __columns ")" \
  )




#define CREATE_IDX_MOZ_PLACES_TEMP_URL \
  CREATE_PLACES_IDX( \
    "url_uniqueindex", "moz_places_temp", "url", "UNIQUE" \
  )
#define CREATE_IDX_MOZ_PLACES_URL \
  CREATE_PLACES_IDX( \
    "url_uniqueindex", "moz_places", "url", "UNIQUE" \
  )

#define CREATE_IDX_MOZ_PLACES_TEMP_FAVICON \
  CREATE_PLACES_IDX( \
    "faviconindex", "moz_places_temp", "favicon_id", "" \
  )
#define CREATE_IDX_MOZ_PLACES_FAVICON \
  CREATE_PLACES_IDX( \
    "faviconindex", "moz_places", "favicon_id", "" \
  )

#define CREATE_IDX_MOZ_PLACES_TEMP_REVHOST \
  CREATE_PLACES_IDX( \
    "hostindex", "moz_places_temp", "rev_host", "" \
  )
#define CREATE_IDX_MOZ_PLACES_REVHOST \
  CREATE_PLACES_IDX( \
    "hostindex", "moz_places", "rev_host", "" \
  )

#define CREATE_IDX_MOZ_PLACES_TEMP_VISITCOUNT \
  CREATE_PLACES_IDX( \
    "visitcount", "moz_places_temp", "visit_count", "" \
  )
#define CREATE_IDX_MOZ_PLACES_VISITCOUNT \
  CREATE_PLACES_IDX( \
    "visitcount", "moz_places", "visit_count", "" \
  )

#define CREATE_IDX_MOZ_PLACES_TEMP_FRECENCY \
  CREATE_PLACES_IDX( \
    "frecencyindex", "moz_places_temp", "frecency", "" \
  )
#define CREATE_IDX_MOZ_PLACES_FRECENCY \
  CREATE_PLACES_IDX( \
    "frecencyindex", "moz_places", "frecency", "" \
  )

#define CREATE_IDX_MOZ_PLACES_TEMP_LASTVISITDATE \
  CREATE_PLACES_IDX( \
    "lastvisitdateindex", "moz_places_temp", "last_visit_date", "" \
  )
#define CREATE_IDX_MOZ_PLACES_LASTVISITDATE \
  CREATE_PLACES_IDX( \
    "lastvisitdateindex", "moz_places", "last_visit_date", "" \
  )





#define CREATE_IDX_MOZ_HISTORYVISITS_TEMP_PLACEDATE \
  CREATE_PLACES_IDX( \
    "placedateindex", "moz_historyvisits_temp", "place_id, visit_date", "" \
  )
#define CREATE_IDX_MOZ_HISTORYVISITS_PLACEDATE \
  CREATE_PLACES_IDX( \
    "placedateindex", "moz_historyvisits", "place_id, visit_date", "" \
  )

#define CREATE_IDX_MOZ_HISTORYVISITS_TEMP_FROMVISIT \
  CREATE_PLACES_IDX( \
    "fromindex", "moz_historyvisits_temp", "from_visit", "" \
  )
#define CREATE_IDX_MOZ_HISTORYVISITS_FROMVISIT \
  CREATE_PLACES_IDX( \
    "fromindex", "moz_historyvisits", "from_visit", "" \
  )

#define CREATE_IDX_MOZ_HISTORYVISITS_TEMP_VISITDATE \
  CREATE_PLACES_IDX( \
    "dateindex", "moz_historyvisits_temp", "visit_date", "" \
  )
#define CREATE_IDX_MOZ_HISTORYVISITS_VISITDATE \
  CREATE_PLACES_IDX( \
    "dateindex", "moz_historyvisits", "visit_date", "" \
  )





#define CREATE_IDX_MOZ_BOOKMARKS_PLACETYPE \
  CREATE_PLACES_IDX( \
    "itemindex", "moz_bookmarks", "fk, type", "" \
  )

#define CREATE_IDX_MOZ_BOOKMARKS_PARENTPOSITION \
  CREATE_PLACES_IDX( \
    "parentindex", "moz_bookmarks", "parent, position", "" \
  )

#define CREATE_IDX_MOZ_BOOKMARKS_PLACELASTMODIFIED \
  CREATE_PLACES_IDX( \
    "itemlastmodifiedindex", "moz_bookmarks", "fk, lastModified", "" \
  )





#define CREATE_IDX_MOZ_ANNOS_PLACEATTRIBUTE \
  CREATE_PLACES_IDX( \
    "placeattributeindex", "moz_annos", "place_id, anno_attribute_id", "UNIQUE" \
  )





#define CREATE_IDX_MOZ_ITEMSANNOS_PLACEATTRIBUTE \
  CREATE_PLACES_IDX( \
    "itemattributeindex", "moz_items_annos", "item_id, anno_attribute_id", "UNIQUE" \
  )


#endif 
