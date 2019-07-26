














package com.squareup.picasso;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.widget.ImageView;
import java.io.IOException;

import static com.squareup.picasso.BitmapHunter.forRequest;
import static com.squareup.picasso.Picasso.LoadedFrom.MEMORY;
import static com.squareup.picasso.Utils.checkNotMain;
import static com.squareup.picasso.Utils.createKey;


@SuppressWarnings("UnusedDeclaration") 
public class RequestCreator {
  private final Picasso picasso;
  private final Request.Builder data;

  private boolean skipMemoryCache;
  private boolean noFade;
  private boolean deferred;
  private int placeholderResId;
  private Drawable placeholderDrawable;
  private int errorResId;
  private Drawable errorDrawable;

  RequestCreator(Picasso picasso, Uri uri, int resourceId) {
    if (picasso.shutdown) {
      throw new IllegalStateException(
          "Picasso instance already shut down. Cannot submit new requests.");
    }
    this.picasso = picasso;
    this.data = new Request.Builder(uri, resourceId);
  }

  




  public RequestCreator placeholder(int placeholderResId) {
    if (placeholderResId == 0) {
      throw new IllegalArgumentException("Placeholder image resource invalid.");
    }
    if (placeholderDrawable != null) {
      throw new IllegalStateException("Placeholder image already set.");
    }
    this.placeholderResId = placeholderResId;
    return this;
  }

  







  public RequestCreator placeholder(Drawable placeholderDrawable) {
    if (placeholderResId != 0) {
      throw new IllegalStateException("Placeholder image already set.");
    }
    this.placeholderDrawable = placeholderDrawable;
    return this;
  }

  
  public RequestCreator error(int errorResId) {
    if (errorResId == 0) {
      throw new IllegalArgumentException("Error image resource invalid.");
    }
    if (errorDrawable != null) {
      throw new IllegalStateException("Error image already set.");
    }
    this.errorResId = errorResId;
    return this;
  }

  
  public RequestCreator error(Drawable errorDrawable) {
    if (errorDrawable == null) {
      throw new IllegalArgumentException("Error image may not be null.");
    }
    if (errorResId != 0) {
      throw new IllegalStateException("Error image already set.");
    }
    this.errorDrawable = errorDrawable;
    return this;
  }

  





  public RequestCreator fit() {
    deferred = true;
    return this;
  }

  
  RequestCreator unfit() {
    deferred = false;
    return this;
  }

  
  public RequestCreator resizeDimen(int targetWidthResId, int targetHeightResId) {
    Resources resources = picasso.context.getResources();
    int targetWidth = resources.getDimensionPixelSize(targetWidthResId);
    int targetHeight = resources.getDimensionPixelSize(targetHeightResId);
    return resize(targetWidth, targetHeight);
  }

  
  public RequestCreator resize(int targetWidth, int targetHeight) {
    data.resize(targetWidth, targetHeight);
    return this;
  }

  




  public RequestCreator centerCrop() {
    data.centerCrop();
    return this;
  }

  



  public RequestCreator centerInside() {
    data.centerInside();
    return this;
  }

  
  public RequestCreator rotate(float degrees) {
    data.rotate(degrees);
    return this;
  }

  
  public RequestCreator rotate(float degrees, float pivotX, float pivotY) {
    data.rotate(degrees, pivotX, pivotY);
    return this;
  }

  




  
  public RequestCreator transform(Transformation transformation) {
    data.transform(transformation);
    return this;
  }

  




  public RequestCreator skipMemoryCache() {
    skipMemoryCache = true;
    return this;
  }

  
  public RequestCreator noFade() {
    noFade = true;
    return this;
  }

  
  public Bitmap get() throws IOException {
    checkNotMain();
    if (deferred) {
      throw new IllegalStateException("Fit cannot be used with get.");
    }
    if (!data.hasImage()) {
      return null;
    }

    Request finalData = picasso.transformRequest(data.build());
    String key = createKey(finalData);

    Action action = new GetAction(picasso, finalData, skipMemoryCache, key);
    return forRequest(picasso.context, picasso, picasso.dispatcher, picasso.cache, picasso.stats,
        action, picasso.dispatcher.downloader).hunt();
  }

  



  public void fetch() {
    if (deferred) {
      throw new IllegalStateException("Fit cannot be used with fetch.");
    }
    if (data.hasImage()) {
      Request finalData = picasso.transformRequest(data.build());
      String key = createKey(finalData);

      Action action = new FetchAction(picasso, finalData, skipMemoryCache, key);
      picasso.enqueueAndSubmit(action);
    }
  }

  




































  public void into(Target target) {
    if (target == null) {
      throw new IllegalArgumentException("Target must not be null.");
    }
    if (deferred) {
      throw new IllegalStateException("Fit cannot be used with a Target.");
    }

    Drawable drawable =
        placeholderResId != 0 ? picasso.context.getResources().getDrawable(placeholderResId)
            : placeholderDrawable;

    if (!data.hasImage()) {
      picasso.cancelRequest(target);
      target.onPrepareLoad(drawable);
      return;
    }

    Request finalData = picasso.transformRequest(data.build());
    String requestKey = createKey(finalData);

    if (!skipMemoryCache) {
      Bitmap bitmap = picasso.quickMemoryCacheCheck(requestKey);
      if (bitmap != null) {
        picasso.cancelRequest(target);
        target.onBitmapLoaded(bitmap, MEMORY);
        return;
      }
    }

    target.onPrepareLoad(drawable);

    Action action = new TargetAction(picasso, target, finalData, skipMemoryCache, requestKey);
    picasso.enqueueAndSubmit(action);
  }

  





  public void into(ImageView target) {
    into(target, null);
  }

  








  public void into(ImageView target, Callback callback) {
    if (target == null) {
      throw new IllegalArgumentException("Target must not be null.");
    }

    if (!data.hasImage()) {
      picasso.cancelRequest(target);
      PicassoDrawable.setPlaceholder(target, placeholderResId, placeholderDrawable);
      return;
    }

    if (deferred) {
      if (data.hasSize()) {
        throw new IllegalStateException("Fit cannot be used with resize.");
      }
      int measuredWidth = target.getMeasuredWidth();
      int measuredHeight = target.getMeasuredHeight();
      if (measuredWidth == 0 || measuredHeight == 0) {
        PicassoDrawable.setPlaceholder(target, placeholderResId, placeholderDrawable);
        picasso.defer(target, new DeferredRequestCreator(this, target, callback));
        return;
      }
      data.resize(measuredWidth, measuredHeight);
    }

    Request finalData = picasso.transformRequest(data.build());
    String requestKey = createKey(finalData);

    if (!skipMemoryCache) {
      Bitmap bitmap = picasso.quickMemoryCacheCheck(requestKey);
      if (bitmap != null) {
        picasso.cancelRequest(target);
        PicassoDrawable.setBitmap(target, picasso.context, bitmap, MEMORY, noFade,
            picasso.debugging);
        if (callback != null) {
          callback.onSuccess();
        }
        return;
      }
    }

    PicassoDrawable.setPlaceholder(target, placeholderResId, placeholderDrawable);

    Action action =
        new ImageViewAction(picasso, target, finalData, skipMemoryCache, noFade, errorResId,
            errorDrawable, requestKey, callback);

    picasso.enqueueAndSubmit(action);
  }
}
