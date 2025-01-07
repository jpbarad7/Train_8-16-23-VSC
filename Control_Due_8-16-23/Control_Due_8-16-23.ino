//Multiple "SimpleTimer" libaries available.  Must use the following:
//https://github.com/schinken/SimpleTimer

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SH1107_Ext.h>
#include <SimpleTimer.h>
SimpleTimer timer;

// Diplay parameters
#define D_HEIGHT 64                                         // Display heighy
#define D_WIDTH 128                                         // Display width
#define D_DELAY 1000                                        // Delay after which to clear the Display
#define Sound_Delay 1000                                    // Delay after which to cancel sound
#define SD 500                                              // speed delay
#define RFID_READY_PIN 4                                    // RFID M4 sends a LOW signal when it's operational

// Display instance
// Note this is our _Ext or extention of the Adafruit library to include the following functions:
// 1. void display.centeredDisplay("line1", ...); with up to four lines as arguments with an optional last numeric delay parameter which clears the display after delay
// 2. void display.clear_Disp(); can be called with no argument (executes immediately)
//    or with a number of milliseconds to clear the display after
// 3. void display.timerRun(); to make the timer in the new display library run
Adafruit_SH1107_Ext display = Adafruit_SH1107_Ext(D_HEIGHT, D_WIDTH, &Wire, 1000000, 1000000);

//State Variables
int active = 0;
int GUI_data = 0;  
int RFID_data = 0;
int RFID_sensor = 0;
int park_switch = 0;
int RFID_READY = 0;
int sound_switch = 1;
int light_switch = 0;
int smoke_switch = 0;
int sas = 0;
int train_direction = 1;
int train_speed = 0;
int train_speed_hold = 0;
int speed_switch = 0;
int station_number_data = 0;

unsigned long previousMillis = 0;


//Serial connections
//Serial1 is RFID connection
//Serial2 is GUI ESP32
//Serial3 is to DCC++ board


//Helper Functinos
//Reads data coming in from RFID and Blynk GUI
void parseIncomingData();
//Sends data to DCC++ board over Serial3
void sendDataDccpp(char data[], unsigned long int delay = 0);
//Sends data to Gui Board over Serial2
void sendDataGui(int data, unsigned long int delay = 0);

void setup() {

  delay (1000);                                         // Allows Decoder and RFID MCUs to complete start up

  pinMode(RFID_READY_PIN, INPUT_PULLUP);

  Serial1.begin(115200);                                // RFID M4
  Serial2.begin(115200);                                // GUI ESP32
  Serial3.begin(115200);                                // DCC++ Uno

  // Set up display
  display.begin(0x3C, true);
  display.setRotation(3);
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.clearDisplay();
  display.display();
}

void loop() {
  parseIncomingData();
  timer.run();
  display.timerRun();
}


void parseIncomingData() {

  if ((Serial2.available() > 0) || (Serial1.available() > 0)) {
    GUI_data = Serial2.read();
    RFID_data = Serial1.read();

  if (RFID_data >= 25 && RFID_data <= 29) {               //  Sends Station # data from RFID reader to ESP-32 for display on GUI
    sendDataGui(RFID_data);
    station_number_data = 1;}
    else {station_number_data = 0;}  

//  RFID_READY = digitalRead(RFID_READY_PIN);

  if ((GUI_data == 39) && (active == 0)) {

    delay (1500);                                         // Asthetically pleasing interval between 'WiFi available' and 'Train Ready'
 
    // Activate Decoder
    sendDataDccpp("<0>");                                 // Turns power to DCC++ rails (communication lines) off
    sendDataDccpp("<1>", 100);                            // Turns power to DCC++ rails on 
    sendDataDccpp("<f 3 184>", 200);                      // Start sound program

    display.centeredDisplay("Train", "Ready", 1200);

    sendDataDccpp("<f 3 222 16>", 4500);                        // AM F17 Conductor calling "All Aboard"
    sendDataDccpp("<f 3 222 0>", Sound_Delay);            // Cancel conductor calling after Sound_Delay

    active = 1;
  }

  if ((GUI_data >= 0) && (GUI_data != 39)) {
    display.setCursor(17, 14);
    display.print("GUI data");
    display.display();

    if (GUI_data < 10) {
      display.setCursor(62, 35);
      display.print(GUI_data); 
      display.display();
    }

    if ((GUI_data >= 10) && (GUI_data < 100)) {
      display.setCursor(56, 35);
      display.print(GUI_data); 
      display.display(); 
    }

    if (GUI_data >= 100) {
      display.setCursor(46, 35);
      display.print(GUI_data); 
      display.display(); 
    }
    
    delay(1000); display.clearDisplay(); display.display();
  }

  if (RFID_data > 0) {
    if ((RFID_sensor == 1) || (station_number_data == 1)) {
      display.setCursor(12, 14);
      display.print("RFID data");
      display.display();

      if (RFID_data < 10) {
        display.setCursor(62, 35);
        display.print(RFID_data); 
        display.display();
      }  

      if ((RFID_data >= 10) && (RFID_data < 100)) {
        display.setCursor(56, 35);
        display.print(RFID_data); 
        display.display(); 
      }

      if (RFID_data >= 100) {
        display.setCursor(46, 35);
        display.print(RFID_data); 
        display.display(); 
      }

     delay(1000); display.clearDisplay(); display.display();
    }   
  }  


//  GUI initiated train functions (lights, sound, smoke, park, RFID sensor, Direction) - ON / OFF SWITCH control

  if (GUI_data == 101) {
    sendDataDccpp("<f 3 144>");                       // Lights ON / OFF
  } else if (GUI_data == 100) {
      sendDataDccpp("<f 3 128>");
    }
      
  if (GUI_data == 251) {RFID_sensor = 1;}             // RFID sensor ON / OFF
  else if (GUI_data == 250) {RFID_sensor = 0;}

  if (GUI_data == 253) {park_switch = 1;}             // park_switch ON / OFF
  else if (GUI_data == 252) {park_switch = 0;}

  if (GUI_data == 255) {train_direction = 1;}
  else if (GUI_data == 254) {train_direction = 0;}    // Direction FWD / REV

  if (GUI_data == 161) {                              // Sound ON / OFF            
      sound_switch = 1;
      sas = 1;
    } else if (GUI_data == 160) {
      sound_switch = 0;
      sas = 1;
    }
                                 
  if (GUI_data == 151){                             // Smoke ON / OFF                
      smoke_switch = 1;
      sas = 1;
    } else if (GUI_data == 150) {
      smoke_switch = 0;
      sas = 1;
    }

  if (sas == 1) {                                     //  Sound and Smoke switches have the same OFF code.  This avoids conflict in operation:
    if ((sound_switch == 0) && (smoke_switch == 0)) { sendDataDccpp("<f 3 176>"); }
    if ((sound_switch == 1) && (smoke_switch == 0)) { sendDataDccpp("<f 3 184>"); }
    if ((sound_switch == 0) && (smoke_switch == 1)) { sendDataDccpp("<f 3 177>"); }   
    if ((sound_switch == 1) && (smoke_switch == 1)) { sendDataDccpp("<f 3 185>"); }
    sas = 0;
  }


//  GUI initiated speed control (Start / Faster speed, Slower speed, Stop) - PUSH BUTTON control

  // Train Start / Faster /Slower
  if (GUI_data == 7) {train_speed = train_speed + 1;}     
  if (GUI_data == 8) {train_speed = train_speed - 1;} 

  // Speed switch
  switch (train_speed) {
    case 0:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 0 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 0 0>");}
      break;
    case 1:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 10 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 10 0>");}
      break;
    case 2:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 20 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 20 0>");}
      break;
    case 3:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 30 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 30 0>");}
      break;
    case 4:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 40 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 40 0>");}
      break;
    case 5:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 50 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 50 0>");}
      break;
    case 6:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 60 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 60 0>");}
      break;
    case 7:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 70 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 70 0>");}
      break;
    case 8:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 80 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 80 0>");}
      break;
    case 9:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 90 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 90 0>");}
      break;
    case 10:
      if (train_direction == 1) {sendDataDccpp("<t 1 03 100 1>");}
      else if (train_direction == 0) {sendDataDccpp("<t 1 03 100 0>");}
      break;
  }

  if (train_speed >= 10) {train_speed = 10;}
  if (train_speed <= 0) {train_speed = 0;}   

  // Train stop
  if (GUI_data == 9) {                        // Train stop
    sendDataDccpp("<t 1 03 -1 1>");
    train_speed = 0;
    }  


//  RFID tag initiated sound, speed, and park function control 

  // Sound
  if (RFID_sensor == 1) { 

    if (RFID_data == 42) {
      sendDataDccpp("<f 3 130>");                       // Train whistles - long
      sendDataDccpp("<f 3 128>", Sound_Delay);          // Cancel train whistles after Sound_Delay
    }

    if (RFID_data == 43) {
      sendDataDccpp("<f 3 132>");                       // Train whistle
      sendDataDccpp("<f 3 128>", Sound_Delay);          // Cancel train whistle after Sound_Delay
    }

    if (RFID_data == 44) {
      sendDataDccpp("<f 3 136>");                       // Train whistle - short
      sendDataDccpp("<f 3 128>", Sound_Delay);          // Cancel train short whistle after Sound_Delay
    }

    if (RFID_data == 41) {
      sendDataDccpp("<f 3 129>");                       // Bell from RFID
      sendDataDccpp("<f 3 128>", Sound_Delay);          // Cancel bell after Sound_Delay
    }
  
  // RFID speed function
  if (RFID_data == 20) {train_speed = 2; sendDataDccpp("<t 1 03 20 1>");}               // Train speed 20%
    else if (RFID_data == 21) {train_speed = 4;sendDataDccpp("<t 1 03 40 1>");}         // Train speed 40%
    else if (RFID_data == 22) {train_speed = 6;sendDataDccpp("<t 1 03 60 1>");}         // Train speed 80%
    else if (RFID_data == 24) {train_speed = 10;sendDataDccpp("<t 1 03 100 1>");}       // Train speed 100%

  }

  // Park function
  if (park_switch == 1) {
    if (train_direction == 1) {
      if (RFID_data == 31) {train_speed = 5;sendDataDccpp("<t 1 03 50 1>");}           // Trigger 1 - Speed 40%
      if (RFID_data == 32) {train_speed = 3;sendDataDccpp("<t 1 03 30 1>");}           // Trigger 2 - Speed 20%
      if (RFID_data == 25) {train_speed = 0;sendDataDccpp("<t 1 03 -1 1>");}           // Park at Station 1
    }
  }

  // Sends train_speed to GUI if there has been a speed change
  if (train_speed != train_speed_hold) {
    sendDataGui(((train_speed * 10) + 100), 0);
    }
    train_speed_hold = train_speed;   
  

// American Mogul (AM) Sounds/Functions triggered by GUI input - Push button control

  if (GUI_data == 41) {
    sendDataDccpp("<f 3 129>");                         // AM F1 Bell from GUI or RFID
    sendDataDccpp("<f 3 128>", Sound_Delay);            // Cancel bell after Sound_Delay
    delay(5000);
  }

  if (GUI_data == 42) {
    sendDataDccpp("<f 3 130>");                         // AM F2 Long whistles
    sendDataDccpp("<f 3 128>", Sound_Delay);            // Cancel long whistles after Sound_Delay
  }

  if (GUI_data == 43) {
    sendDataDccpp("<f 3 132>");                         // AM F3 Whistle
    sendDataDccpp("<f 3 128>", Sound_Delay);            // Cancel train whistle after Sound_Delay
  }

  if (GUI_data == 44) {
    sendDataDccpp("<f 3 136>");                         // AM F4 Short whistle
    sendDataDccpp("<f 3 128>", Sound_Delay);            // Cancel short whistle after Sound_Delay
  }

  if (GUI_data == 45) {}                                // AM F5 Smoke ON/OFF via Smoke switch (V2)

  if (GUI_data == 46) {}                                // AM Open F6

  if (GUI_data == 47) {}                                // AM Open F7

  if (GUI_data == 48) {}                                // AM F8 Sound ON/OFF via Sound switch (V5)

  if (GUI_data == 49) {
    sendDataDccpp("<f 3 161>");                         // AM F9 Wheels squealing
    sendDataDccpp("<f 3 160>", Sound_Delay);            // Cancel wheels squealing after Sound_Delay
  }

  if (GUI_data == 50) {
    sendDataDccpp("<f 3 162>");                         // AM 50 Shoveling coal
    sendDataDccpp("<f 3 160>", Sound_Delay);            // Cancel shoveling coal after Sound_Delay
  }

  if (GUI_data == 51) {
    sendDataDccpp("<f 3 164>");                         // Blower sound
    sendDataDccpp("<f 3 160>", Sound_Delay);            // Cancel blower after Sound_Delay
  }

  if (GUI_data == 52) {
    sendDataDccpp("<f 3 168>");                         // Coupler/Uncoupler
    sendDataDccpp("<f 3 160>", Sound_Delay);            // Cancel coupler/uncoupler after Sound_Delay
  }

  if (GUI_data == 53) {
    sendDataDccpp("<f 3 222 1>");                       // Coal dropping
    sendDataDccpp("<f 3 222 0>", Sound_Delay);          // Cancel coal dropping after Sound_Delay
  }

  if (GUI_data == 54) {
    sendDataDccpp("<f 3 222 2>");                       // Steam venting
    sendDataDccpp("<f 3 222 0>", Sound_Delay);          // Cancel steam venting after Sound_Delay
  }

  if (GUI_data == 55) {}                                // AM Open F15

  if (GUI_data == 56) {}                                // AM Open F16

  if (GUI_data == 57) {
    sendDataDccpp("<f 3 222 16>");                      // AM F17 Conductor calling "All Aboard"
    sendDataDccpp("<f 3 222 0>", Sound_Delay);          // Cancel conductor calling after Sound_Delay
  }

  if (GUI_data == 58) {
    sendDataDccpp("<f 3 222 32>");                      // AM F18 Generator sound
    sendDataDccpp("<f 3 222 0>", Sound_Delay);          // Cancel generator sound after Sound_Delay
  }

  if (GUI_data == 59) {
    sendDataDccpp("<f 3 222 64>");                      // AM F19 Air pump
    sendDataDccpp("<f 3 222 0>", Sound_Delay);          // Cancel air pump after Sound_Delay
  }

  if (GUI_data == 60) {
    sendDataDccpp("<f 3 222 128>");                     // AM F20 Coal unloading into hopper
    sendDataDccpp("<f 3 222 0>", Sound_Delay);          // Cancel coal unloading after Sound_Delay
  }

  if (GUI_data == 61) {
    sendDataDccpp("<f 3 223 1>");                       // AM F21 Water filling tank
    sendDataDccpp("<f 3 223 0>", Sound_Delay);          // Cancel water filling tank after Sound_Delay
  }

  if (GUI_data == 62) {}                                // AM Open F22

  if (GUI_data == 63) {}                                // AM Open F23

  if (GUI_data == 64) {}                                // AM Open F24


// K3 Stainz (K3) Sounds/Functions triggered by GUI input - Push button control

  if (GUI_data == 65) {}                                // K3 F0 Lights ON/OFF toggle via AM F0

  if (GUI_data == 66) {}                                // K3 Open F1

  if (GUI_data == 67) {
    sendDataDccpp("<f 3 130>");                         // K3 F2 Conductor calling "All Aboard"
    sendDataDccpp("<f 3 128>", Sound_Delay);            // Cancel conductor calling after Sound_Delay
  }

  if (GUI_data == 68) {
    sendDataDccpp("<f 3 132>");                         // K3 F3 Conductor's whistle
    sendDataDccpp("<f 3 128>", Sound_Delay);            // Cancel conductor's whistle after Sound_Delay
  }

  if (GUI_data == 69) {}                                // K3 Open F4

  if (GUI_data == 70) {
    sendDataDccpp("<f 3 177>");                         // K3 F5 Coal shoveling
    sendDataDccpp("<f 3 176>", Sound_Delay);            // Cancel coal shoveling after Sound_Delay
  }

  if (GUI_data == 71) {
    sendDataDccpp("<f 3 178>");                         // K3 F6 Injector sound
    sendDataDccpp("<f 3 176>", Sound_Delay);            // Cancel injector after Sound_Delay
  }

  if (GUI_data == 72) {
    sendDataDccpp("<f 3 180>");                         // K3 F7 Bell
    sendDataDccpp("<f 3 176>", Sound_Delay);            // Cancel Bell after Sound_Delay
  }

  if (GUI_data == 73) {}                                // K3 F8 Sound ON/OFF toggle via V3

  if (GUI_data == 74) {
    sendDataDccpp("<f 3 161>");                         // K3 F9 Whistle
    sendDataDccpp("<f 3 160>", Sound_Delay);            // Cancel Whistle after Sound_Delay
  } 

  if (GUI_data == 75) {
    sendDataDccpp("<f 3 162>");                         // K3 F10 Coupler/uncoupler
    sendDataDccpp("<f 3 160>", Sound_Delay);            // Cancel coupler/uncoupler after Sound_Delay
  }

  if (GUI_data == 76) {
    sendDataDccpp("<f 3 164>");                         // K3 F11 Air pump
    sendDataDccpp("<f 3 160>", Sound_Delay);            // Cancel air pump after Sound_Delay
  }

  if (GUI_data == 77) {
    sendDataDccpp("<f 3 168>");                         // K3 F12 Boiler sound
    sendDataDccpp("<f 3 160>", Sound_Delay);            // Cancel boiler sound after Sound_Delay
  }

  if (GUI_data == 78) {
    sendDataDccpp("<f 3 222 1>");                       // K3 F13 Steam venting
    sendDataDccpp("<f 3 222 0>", Sound_Delay);          // Cancel steam venting after Sound_Delay
  }

  if (GUI_data == 79) {}                                // K3 Open F14
 

  }  // End of 'if Serial.available()' loop

}    // End of 'parseIncomingData()' loop 

// Helper Functions

//Sends data to DCC++ board over Serial3
void sendDataDccpp(char data[], unsigned long int delay) {
  if (!delay) {  //delay is 0, execute immediately
    if (Serial1.availableForWrite()) {
      Serial3.print(data);
    }
  } else {  //delay is longer than 0, execute after delay through timer library
    timer.setTimeout(delay,
                     [=]() {
                       Serial3.print(data);
                     });
  }
}

void sendDataGui(int data, unsigned long int delay) {
  if (!delay) {  //delay is 0, execute immediately
    if (Serial1.availableForWrite()) {
      Serial2.write(data);
    }
  } else {  //delay is longer than 0, execute after delay through timer library
    timer.setTimeout(delay,
                     [=]() {
                       Serial2.write(data);
                     });
  }
}



