package com.kaju.fingerprintlockcontrol;

import androidx.appcompat.app.AppCompatActivity;

import android.bluetooth.BluetoothAdapter;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.Toast;

import java.util.Set;

public class CheckForBtActivity extends AppCompatActivity {
    Button btn = (Button)findViewById(R.id.refreshBtn);
    private BluetoothAdapter btAdapter = BluetoothAdapter.getDefaultAdapter();
    private Set pairedDevices;
    private void checkBluetooth() {
        if (btAdapter == null) {
            Toast.makeText(getApplicationContext(),"Your device has no bluetooth module.", Toast.LENGTH_SHORT);
            finish();
        } else {
            if (!btAdapter.isEnabled()) {
                Intent turnBtOn = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(turnBtOn, 1);
            }
        }
    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_check_for_bt);
    }
}