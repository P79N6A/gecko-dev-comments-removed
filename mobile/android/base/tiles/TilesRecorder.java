



package org.mozilla.gecko.tiles;

import java.util.List;

import org.json.JSONArray;
import org.json.JSONObject;
import org.json.JSONException;

import org.mozilla.gecko.GeckoAppShell;
import org.mozilla.gecko.GeckoEvent;
import org.mozilla.gecko.Tab;

import android.util.Log;

public class TilesRecorder {
    public static final String ACTION_CLICK = "click";

    private static final String LOG_TAG = "GeckoTilesRecorder";
    private static final String EVENT_TILES_CLICK = "Tiles:Click";

    public void recordAction(Tab tab, String action, int index, List<Tile> tiles) {
        final Tile clickedTile = tiles.get(index);

        if (tab == null || clickedTile == null) {
            throw new IllegalArgumentException("Tab and tile cannot be null");
        }

        if (clickedTile.id == -1) {
            
            return;
        }

        try {
            final JSONArray tilesJSON = new JSONArray();
            int clickedTileIndex = -1;
            int currentTileIndex = 0;

            for (int i = 0; i < tiles.size(); i++) {
                final Tile tile = tiles.get(i);
                if (tile == null) {
                    
                    continue;
                }

                
                
                tilesJSON.put(jsonForTile(tile, currentTileIndex, i));

                
                
                
                if (clickedTile == tile) {
                    clickedTileIndex = currentTileIndex;
                }

                currentTileIndex++;
            }

            if (clickedTileIndex == -1) {
                throw new IllegalStateException("Clicked tile index not set");
            }

            final JSONObject payload = new JSONObject();
            payload.put(action, clickedTileIndex);
            payload.put("tiles", tilesJSON);

            final JSONObject data = new JSONObject();
            data.put("tabId", tab.getId());
            data.put("payload", payload.toString());

            GeckoAppShell.sendEventToGecko(GeckoEvent.createBroadcastEvent(EVENT_TILES_CLICK, data.toString()));
        } catch (JSONException e) {
            Log.e(LOG_TAG, "JSON error", e);
        }
    }

    private JSONObject jsonForTile(Tile tile, int tileIndex, int viewIndex) throws JSONException {
        final JSONObject tileJSON = new JSONObject();

        
        if (tile.id != -1) {
            tileJSON.put("id", tile.id);
        }

        
        if (tile.pinned) {
            tileJSON.put("pin", true);
        }

        
        
        
        if (tileIndex != viewIndex) {
            tileJSON.put("pos", viewIndex);
        }

        return tileJSON;
    }
}
