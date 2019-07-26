



package org.mozilla.gecko.fxa.activities;

import org.mozilla.gecko.background.common.log.Logger;

import android.app.Activity;
import android.content.Intent;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;

public abstract class FxAccountAbstractActivity extends Activity {
  private static final String LOG_TAG = FxAccountAbstractActivity.class.getSimpleName();

  protected void launchActivity(Class<? extends Activity> activityClass) {
    Intent intent = new Intent(this, activityClass);
    
    
    intent.setFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
    startActivity(intent);
  }

  protected void redirectToActivity(Class<? extends Activity> activityClass) {
    launchActivity(activityClass);
    finish();
  }

  






  public View ensureFindViewById(View v, int id, String description) {
    View view;
    if (v != null) {
      view = v.findViewById(id);
    } else {
      view = findViewById(id);
    }
    if (view == null) {
      String message = "Could not find view " + description + ".";
      Logger.error(LOG_TAG, message);
      throw new RuntimeException(message);
    }
    return view;
  }

  public void linkifyTextViews(View view, int[] textViews) {
    for (int id : textViews) {
      TextView textView;
      if (view != null) {
        textView = (TextView) view.findViewById(id);
      } else {
        textView = (TextView) findViewById(id);
      }
      if (textView == null) {
        Logger.warn(LOG_TAG, "Could not process links for view with id " + id + ".");
        continue;
      }
      textView.setMovementMethod(LinkMovementMethod.getInstance());
      textView.setText(Html.fromHtml(textView.getText().toString()));
    }
  }

  protected void launchActivityOnClick(final View view, final Class<? extends Activity> activityClass) {
    view.setOnClickListener(new OnClickListener() {
      @Override
      public void onClick(View v) {
        FxAccountAbstractActivity.this.launchActivity(activityClass);
      }
    });
  }
}
