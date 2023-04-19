#include <Energia.h>
#ifndef __CC3100R1M1RGC__
#include <SPI.h>
#endif
#include <WiFi.h>


float alpha = 0.2; // filter coefficient
float y_high[3] = {0}; // output samples
float y_low[6] = {0}; // output samples
float y_der = 0;
float y_sq[5] = {0};
float sq_av = 0;
int his[2] = {0};
unsigned long T[10] = {0};
double ans = 0;
float x[33] = {0}; // input samples
char ssid[] = "";
char pass[] = "";
int status = WL_IDLE_STATUS;
int bpm = 0;
int n=0;
long prev_beat = 0;
long curr_beat = 0;

long lastSent = millis() + 60000;

IPAddress server(52,54,163,195);  // Adafruit
WiFiClient client;



void setup() {
    Serial.begin(115200);

    while (status != WL_CONNECTED) {
        Serial.print("Attempting to connect to open SSID: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        // wait 10 seconds for connection:
        delay(10000);
    }
    if (client.connect(server, 80)) {
      Serial.println("connected");
    }

    pinMode(10, INPUT); // Setup for leads off detection LO +
    pinMode(11, INPUT); // Setup for leads off detection LO -

}

void loop() {
    if((digitalRead(10) == 1)||(digitalRead(11) == 1)){
        Serial.println("."); 
    }
    else{
        // send the value of analog input 0:
        x[0] = analogRead(A0);

        // calculate filter output
        // LOW-PASS Filter 
        y_low[2] = 2*y_low[3] - y_low[5] + x[2] - 2*x[8] + x[14];

        y_high[2] = 32*x[18] - y_low[3] - x[2] + x[34];
        y_der = (-x[4] - 2*x[3] + 2*x[1] + x[0])/8;
        y_sq[0] = y_der*y_der;

        // shift samples
        for(int i =32; i>0; i--){
            x[i] = x[i-1];
        }

        sq_av = y_sq[0]+y_sq[1]+y_sq[2]+y_sq[3]+y_sq[4];

        for(int i = 4; i>0; i--){
            y_sq[i] = y_sq[i-1];
        }

        y_low[2] = y_low[1];
        y_low[1] = y_low[0];
        y_high[2] = y_high[1];
        y_high[1] = y_high[0];

        if(sq_av < 8000){
            sq_av = 0;
        }
        else{
            sq_av = 1;
        }

        his[1] = his[0];
        his[0] = sq_av;

        //Rising Edge
        if(his[1] == 0 && his[0] == 1){
              curr_beat = millis();
            T[0] = curr_beat - prev_beat ;
            prev_beat = curr_beat;

            for(int i =9; i>0; i--){
                T[i] = T[i-1];
                Serial.println(T[i]);

            }


            for(int i = 0; i<10; i++){
                len = len + T[i];
            }

             bpm = 60000/(len/10);
             Serial.print("BPM:");
             Serial.println(bpm);
            ans = 0;

        }
    }

    //Wait for a bit to keep serial data from saturating
    delay(10);

//    Sending to Adafruit
  if(millis()-lastSent > 60000){
    long ts = millis();
    client.println("POST /api/v2/StephenCondon/feeds/welcome-feed/data HTTP/1.1\r\nX-AIO-Key: {API KEY}\r\nHost: io.adafruit.com\r\nContent-Type: multipart/form-data; boundary=--------------------------391877519121121795085607\r\nContent-Length: 162\r\n\r\n----------------------------391877519121121795085607\r\nContent-Disposition: form-data; name=\"value\"\r\n");
          client.println(String(bpm));
          client.println("\n----------------------------391877519121121795085607--");
          client.println();
          delay(5000);
  
          while(client.available() == 0){
            
          }

          lastSent = millis();
          Serial.println("Sent to IOT server");

          prev_beat = prev_beat + millis() - ts;
         
  }
         
}
