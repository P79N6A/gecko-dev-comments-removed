














package com.squareup.picasso;

import android.graphics.Bitmap;








public interface Cache {
  
  Bitmap get(String key);

  
  void set(String key, Bitmap bitmap);

  
  int size();

  
  int maxSize();

  
  void clear();

  
  Cache NONE = new Cache() {
    @Override public Bitmap get(String key) {
      return null;
    }

    @Override public void set(String key, Bitmap bitmap) {
      
    }

    @Override public int size() {
      return 0;
    }

    @Override public int maxSize() {
      return 0;
    }

    @Override public void clear() {
    }
  };
}
