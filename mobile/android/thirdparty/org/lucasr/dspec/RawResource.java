















package org.lucasr.dspec;

import android.content.Context;
import android.content.res.Resources;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.StringWriter;

class RawResource {
    public static JSONObject getAsJSON(Resources resources, int id) throws IOException {
        InputStreamReader reader = null;

        try {
            final InputStream is = resources.openRawResource(id);
            if (is == null) {
                return null;
            }

            reader = new InputStreamReader(is, "UTF-8");

            final char[] buffer = new char[1024];
            final StringWriter s = new StringWriter();

            int n;
            while ((n = reader.read(buffer, 0, buffer.length)) != -1) {
                s.write(buffer, 0, n);
            }

            return new JSONObject(s.toString());
        } catch (JSONException e) {
            throw new IllegalStateException("Invalid design spec JSON resource", e);
        } finally {
            if (reader != null) {
                reader.close();
            }
        }
    }
}