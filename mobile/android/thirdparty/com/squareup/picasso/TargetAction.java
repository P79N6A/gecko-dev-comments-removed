














package com.squareup.picasso;

import android.graphics.Bitmap;

final class TargetAction extends Action<Target> {

  TargetAction(Picasso picasso, Target target, Request data, boolean skipCache, String key) {
    super(picasso, target, data, skipCache, false, 0, null, key);
  }

  @Override void complete(Bitmap result, Picasso.LoadedFrom from) {
    if (result == null) {
      throw new AssertionError(
          String.format("Attempted to complete action with no result!\n%s", this));
    }
    Target target = getTarget();
    if (target != null) {
      target.onBitmapLoaded(result, from);
      if (result.isRecycled()) {
        throw new IllegalStateException("Target callback must not recycle bitmap!");
      }
    }
  }

  @Override void error() {
    Target target = getTarget();
    if (target != null) {
      target.onBitmapFailed(errorDrawable);
    }
  }
}
