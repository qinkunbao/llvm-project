package net.hanshq.hello;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends Activity {
    static {
        System.loadLibrary("loader");
        loadDFM();

        // This doesn't actually load libhello.so (that was done in loadDFM), it
        // just registers it with the Java runtime so that JNI calls work
        // correctly.
        System.loadLibrary("hello");
    }

    public native String getMessage();
    public static native void loadDFM();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final TextView text = (TextView)findViewById(R.id.my_text);
        text.setText(getMessage());

        final Button button = findViewById(R.id.my_button);
        button.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
               text.setText(getMessage());
            }
        });
    }
}
