




package org.mozilla.gecko.webapp;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;

public class Dispatcher extends Activity {
    private static final String LOGTAG = "GeckoWebappDispatcher";

    @Override
    protected void onCreate(Bundle bundle) {
        super.onCreate(bundle);

        Allocator allocator = Allocator.getInstance(getApplicationContext());

        if (bundle == null) {
            bundle = getIntent().getExtras();
        }

        if (bundle == null) {
            Log.e(LOGTAG, "Passed intent data missing.");
        }

        String packageName = bundle.getString("packageName");

        int index = allocator.getIndexForApp(packageName);
        boolean isInstalled = index >= 0;
        if (!isInstalled) {
            index = allocator.findOrAllocatePackage(packageName);
        }

        
        Intent intent = new Intent(getIntent());

        
        intent.setClassName(getApplicationContext(), getPackageName() + ".WebApps$WebApp" + index);

        
        intent.putExtra("isInstalled", isInstalled);

        startActivity(intent);
    }
}
