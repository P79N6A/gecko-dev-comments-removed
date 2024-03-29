














package com.squareup.picasso;

import android.content.Context;
import android.net.Uri;
import android.net.http.HttpResponseCache;
import android.os.Build;
import java.io.File;
import java.io.IOException;
import java.net.HttpURLConnection;
import java.net.URL;

import static com.squareup.picasso.Utils.parseResponseSourceHeader;






public class UrlConnectionDownloader implements Downloader {
  static final String RESPONSE_SOURCE = "X-Android-Response-Source";

  private static final Object lock = new Object();
  static volatile Object cache;

  private final Context context;

  public UrlConnectionDownloader(Context context) {
    this.context = context.getApplicationContext();
  }

  protected HttpURLConnection openConnection(Uri path) throws IOException {
    HttpURLConnection connection = (HttpURLConnection) new URL(path.toString()).openConnection();
    connection.setConnectTimeout(Utils.DEFAULT_CONNECT_TIMEOUT);
    connection.setReadTimeout(Utils.DEFAULT_READ_TIMEOUT);
    return connection;
  }

  @Override public Response load(Uri uri, boolean localCacheOnly) throws IOException {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.ICE_CREAM_SANDWICH) {
      installCacheIfNeeded(context);
    }

    HttpURLConnection connection = openConnection(uri);
    connection.setUseCaches(true);
    if (localCacheOnly) {
      connection.setRequestProperty("Cache-Control", "only-if-cached,max-age=" + Integer.MAX_VALUE);
    }

    int responseCode = connection.getResponseCode();
    if (responseCode >= 300) {
      connection.disconnect();
      throw new ResponseException(responseCode + " " + connection.getResponseMessage());
    }

    boolean fromCache = parseResponseSourceHeader(connection.getHeaderField(RESPONSE_SOURCE));

    return new Response(connection.getInputStream(), fromCache);
  }

  private static void installCacheIfNeeded(Context context) {
    
    if (cache == null) {
      try {
        synchronized (lock) {
          if (cache == null) {
            cache = ResponseCacheIcs.install(context);
          }
        }
      } catch (IOException ignored) {
      }
    }
  }

  private static class ResponseCacheIcs {
    static Object install(Context context) throws IOException {
      File cacheDir = Utils.createDefaultCacheDir(context);
      HttpResponseCache cache = HttpResponseCache.getInstalled();
      if (cache == null) {
        long maxSize = Utils.calculateDiskCacheSize(cacheDir);
        cache = HttpResponseCache.install(cacheDir, maxSize);
      }
      return cache;
    }
  }
}
