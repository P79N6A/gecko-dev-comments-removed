














package com.squareup.picasso;

import android.graphics.Bitmap;
import android.net.Uri;
import java.io.IOException;
import java.io.InputStream;


public interface Downloader {
  










  Response load(Uri uri, boolean localCacheOnly) throws IOException;

  
  class ResponseException extends IOException {
    public ResponseException(String message) {
      super(message);
    }
  }

  
  class Response {
    final InputStream stream;
    final Bitmap bitmap;
    final boolean cached;

    





    public Response(Bitmap bitmap, boolean loadedFromCache) {
      if (bitmap == null) {
        throw new IllegalArgumentException("Bitmap may not be null.");
      }
      this.stream = null;
      this.bitmap = bitmap;
      this.cached = loadedFromCache;
    }

    





    public Response(InputStream stream, boolean loadedFromCache) {
      if (stream == null) {
        throw new IllegalArgumentException("Stream may not be null.");
      }
      this.stream = stream;
      this.bitmap = null;
      this.cached = loadedFromCache;
    }

    




    public InputStream getInputStream() {
      return stream;
    }

    




    public Bitmap getBitmap() {
      return bitmap;
    }
  }
}
