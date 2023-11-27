package com.kaju.fingerprintlockcontrol;

import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.app.AppCompatActivity;

import android.content.Context;
import android.content.DialogInterface;
import android.graphics.Color;
import android.os.Bundle;
import android.os.CountDownTimer;
import android.text.InputType;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {
    boolean isScanner = false;
    int id = 0;
    //tymczasowy timer dla testów UI. implementacja bluetooth jeszcze nie jest gotowa
    CountDownTimer restartTest = new CountDownTimer(5000,1000){
        public void onTick(long millis){
            TextView statusTxt = (TextView)findViewById(R.id.currStatusTxt);
            statusTxt.setText("Pending... " + ((millis/1000) + 1));
        }
        public void onFinish(){
            isScanner = !isScanner;
            checkScanner();
        }
    };
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        checkScanner();
    }

    private void checkScanner(){
        TextView statusTxt = (TextView)findViewById(R.id.currStatusTxt);
        Button addBtn = (Button)findViewById(R.id.addBtn);
        Button removeBtn = (Button)findViewById(R.id.removeBtn);
        Button wipeBtn = (Button)findViewById(R.id.wipeBtn);
        if(isScanner){
            statusTxt.setText("Online");
            statusTxt.setTextColor(Color.parseColor("#027500"));
            addBtn.setClickable(true);
            addBtn.setTextColor(Color.BLACK);
            removeBtn.setClickable(true);
            removeBtn.setTextColor(Color.BLACK);
            wipeBtn.setClickable(true);
            wipeBtn.setTextColor(Color.BLACK);
        } else {
            statusTxt.setText("Offline");
            statusTxt.setTextColor(Color.parseColor("#ad0000"));
            addBtn.setClickable(false);
            addBtn.setTextColor(Color.parseColor("#9c9c9c"));
            removeBtn.setClickable(false);
            removeBtn.setTextColor(Color.parseColor("#9c9c9c"));
            wipeBtn.setClickable(false);
            wipeBtn.setTextColor(Color.parseColor("#9c9c9c"));
        }
    }
    public void toastPopUp(String txt){
        Context context = getApplicationContext();
        int duration = Toast.LENGTH_SHORT;
        Toast toast = Toast.makeText(context,txt,duration);
        toast.show();
    }
    private void popupAddRemoveWindowTrigger(String title, String msg, final String ctxt, final boolean removeCmd){
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle(title);
        builder.setMessage(msg);
        final EditText input = new EditText(this);
        input.setInputType(InputType.TYPE_CLASS_NUMBER);
        builder.setView(input);
        builder.setPositiveButton("Send", new DialogInterface.OnClickListener(){
            //ten override jest celowo pusty, on jest dalej nadpisany w celu sprawdzenia czy id się mieści w przedziale.
            //gdyby ten warunek był tu to albo okno by się zamknęło tak czy siak, albo
            //program wpadłby w nieskończoną pętlę po kliknięciu "send"
            @Override
            public void onClick(DialogInterface dialog, int which){
            }
        });
        builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.cancel();
                toastPopUp("Operation cancelled.");
            }
        });
        final AlertDialog dialog = builder.create();
        dialog.show();
        dialog.getButton(AlertDialog.BUTTON_POSITIVE).setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v){
                Boolean closeDialog = false;
                id = Integer.parseInt(input.getText().toString());
                if (id <= 0 || id > 20) toastPopUp("Please enter valid id (between 1 and 20)!");
                else closeDialog = true;
                if(closeDialog) {
                    dialog.dismiss();
                    if(removeCmd) removeConfirmationPopup();
                    id = 0;
                    if(!removeCmd) toastPopUp(ctxt);
                }
            }
        }
        );
    }
    private void wipeDatabasePopup(){
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Wipe database");
        builder.setMessage("Are you sure you want to clear scanner's database?");
        builder.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.dismiss();
                toastPopUp("Database wiped.");
            }
        });
        builder.setNegativeButton("No", new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.cancel();
                toastPopUp("Operation cancelled.");
            }
        });
        final AlertDialog dialog = builder.create();
        dialog.show();
    }

    public void removeConfirmationPopup(){
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Remove entry");
        builder.setMessage("Entry with ID of " + id + " will be removed. \nAre you sure?");
        builder.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.dismiss();
                toastPopUp("Entry removed.");
            }
        });
        builder.setNegativeButton("No", new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.cancel();
                toastPopUp("Operation cancelled.");
            }
        });
        final AlertDialog dialog = builder.create();
        dialog.show();
    }

    public void restartConfirmationPopup(){
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Remote restart");
        builder.setMessage("Are you sure you want to restart lock?");
        builder.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.dismiss();
                toastPopUp("Restarting....");
                TextView statusTxt = (TextView)findViewById(R.id.currStatusTxt);
                statusTxt.setText("Pending...");
                statusTxt.setTextColor(Color.BLACK);
                restartTest.start();
            }
        });
        builder.setNegativeButton("No", new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.cancel();
                toastPopUp("Operation cancelled.");
            }
        });
        final AlertDialog dialog = builder.create();
        dialog.show();
    }

    public void backBtnExitConfirmationPopup(){
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Exit.");
        builder.setMessage("Are you sure? Connection with paired lock will be terminated.");
        builder.setPositiveButton("Yes", new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.dismiss();
                finish();
            }
        });
        builder.setNegativeButton("No", new DialogInterface.OnClickListener(){
            @Override
            public void onClick(DialogInterface dialog, int which){
                dialog.cancel();
            }
        });
        final AlertDialog dialog = builder.create();
        dialog.show();
    }
    public void addBtnOnClick(View v){
        //co prawda sprawdzanie, czy isScanner jest 0 czy 1 nie jest tu wymagane, bo już jest funkcja która wyłącza te przyciski, niemniej jednak dla bezpieczeństwa tu go zostawiłem
        if(isScanner) {
            popupAddRemoveWindowTrigger("Add new fingerprint", "Insert ID of new fingerprint to be added:", "Now follow instructions on lock's screen.",false);
        } else {
            toastPopUp("This option is disabled when scanner is unavailable");
        }
    }
    public void removeBtnOnClick(View v){
        if(isScanner) {
        popupAddRemoveWindowTrigger("Remove fingerprint","Insert ID of fingerprint to be removed:","", true);
        } else {
            toastPopUp("This option is disabled when scanner is unavailable");
        }
    }
    public void wipeBtnOnClick(View v){
        if(isScanner) {
            wipeDatabasePopup();
        } else {
            toastPopUp("This option is disabled when scanner is unavailable");
        }
    }
    public void restartBtnOnClick(View v){
        restartConfirmationPopup();
    }
    public void remoteOpnBtnOnClick(View v){
        toastPopUp("Remote open...");
    }
    public void remoteClsBtnOnClick(View v){
        toastPopUp("Remote close...");
    }
    public void onBackPressed(){
        backBtnExitConfirmationPopup();
    }
}