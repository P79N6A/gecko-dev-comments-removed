









package org.webrtc.vieautotest;

import org.webrtc.vieautotest.R;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.view.SurfaceView;
import android.view.View;
import android.view.SurfaceHolder;
import android.widget.LinearLayout;
import android.opengl.GLSurfaceView;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;

public class ViEAutotest extends Activity
    implements
      AdapterView.OnItemSelectedListener,
      View.OnClickListener {

  private Thread testThread;
  private Spinner testSpinner;
  private Spinner subtestSpinner;
  private int testSelection;
  private int subTestSelection;

  
  private LinearLayout remoteSurface = null;
  private GLSurfaceView glSurfaceView = null;
  private SurfaceView surfaceView = null;

  private LinearLayout localSurface = null;
  private GLSurfaceView glLocalSurfaceView = null;
  private SurfaceView localSurfaceView = null;

  
  @Override
  public void onCreate(Bundle savedInstanceState) {

    Log.d("*WEBRTC*", "onCreate called");

    super.onCreate(savedInstanceState);
    setContentView(R.layout.main);

    
    final Button buttonStart = (Button) findViewById(R.id.Button01);
    buttonStart.setOnClickListener(this);

    
    testSpinner = (Spinner) findViewById(R.id.testSpinner);
    ArrayAdapter<CharSequence> adapter =
        ArrayAdapter.createFromResource(this, R.array.test_array,
                                        android.R.layout.simple_spinner_item);

    int resource = android.R.layout.simple_spinner_dropdown_item;
    adapter.setDropDownViewResource(resource);
    testSpinner.setAdapter(adapter);
    testSpinner.setOnItemSelectedListener(this);

    
    subtestSpinner = (Spinner) findViewById(R.id.subtestSpinner);
    ArrayAdapter<CharSequence> subtestAdapter =
        ArrayAdapter.createFromResource(this, R.array.subtest_array,
                                        android.R.layout.simple_spinner_item);

    subtestAdapter.setDropDownViewResource(resource);
    subtestSpinner.setAdapter(subtestAdapter);
    subtestSpinner.setOnItemSelectedListener(this);

    remoteSurface = (LinearLayout) findViewById(R.id.RemoteView);
    surfaceView = new SurfaceView(this);
    remoteSurface.addView(surfaceView);

    localSurface = (LinearLayout) findViewById(R.id.LocalView);
    localSurfaceView = new SurfaceView(this);
    localSurfaceView.setZOrderMediaOverlay(true);
    localSurface.addView(localSurfaceView);

    
    testSelection = 0;
    subTestSelection = 0;
  }

  public void onClick(View v) {
    Log.d("*WEBRTC*", "Button clicked...");
    switch (v.getId()) {
      case R.id.Button01:
        new Thread(new Runnable() {
            public void run() {
              Log.d("*WEBRTC*", "Calling RunTest...");
              RunTest(testSelection, subTestSelection,
                      localSurfaceView, surfaceView);
              Log.d("*WEBRTC*", "RunTest done");
            }
          }).start();
    }
  }

  public void onItemSelected(AdapterView<?> parent, View v,
                             int position, long id) {

    if (parent == (Spinner) findViewById(R.id.testSpinner)) {
      testSelection = position;
    } else {
      subTestSelection = position;
    }
  }

  public void onNothingSelected(AdapterView<?> parent) {
  }

  @Override
  protected void onStart() {
    super.onStart();
  }

  @Override
  protected void onResume() {
    super.onResume();
  }

  @Override
  protected void onPause() {
    super.onPause();
  }

  @Override
  protected void onStop() {
    super.onStop();
  }

  @Override
  protected void onDestroy() {

    super.onDestroy();
  }

  
  
  
  private native int RunTest(int testSelection, int subtestSelection,
                             SurfaceView window1, SurfaceView window2);

  
  
  static {
    Log.d("*WEBRTC*", "Loading ViEAutotest...");
    System.loadLibrary("webrtc-video-autotest-jni");
  }
}
