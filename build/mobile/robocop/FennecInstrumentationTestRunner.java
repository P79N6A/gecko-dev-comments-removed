



package org.mozilla.gecko;

import android.os.Bundle;
import android.test.InstrumentationTestRunner;
import android.util.Log;

public class FennecInstrumentationTestRunner extends InstrumentationTestRunner {
    private static Bundle sArguments;

    @Override
    public void onCreate(Bundle arguments) {
        sArguments = arguments;
        if (sArguments == null) {
            Log.e("Robocop", "FennecInstrumentationTestRunner.onCreate got null bundle");
        }
        super.onCreate(arguments);
    }

    
    
    public static Bundle getFennecArguments() {
        if (sArguments == null) {
            Log.e("Robocop", "FennecInstrumentationTestCase.getFennecArguments returns null bundle");
        }
        return sArguments;
    }
}
