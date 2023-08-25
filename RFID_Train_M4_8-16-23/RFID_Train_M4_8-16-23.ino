#include <SPI.h>
#include <Wire.h>
#include <MFRC522.h>
#include <Adafruit_SH1107_Ext.h>
#include <SimpleTimer.h>

#define RST_PIN 12
#define SS_PIN 11
#define RFID_READY_PIN 10

// Diplay parameters and instance`
#define D_HEIGHT 64  //Display heighy
#define D_WIDTH 128  //Display width

//Display instance
//Note this is our _Ext or extention of the Adafruit library to include the following functions:
// 1. void display.centeredDisplay("line1", ...); with up to four lines as arguments with an optional last numeric delay parameter which clears the display after delay
// 2. void display.clear_Disp(); can be called with no argument (executes immediately)
//or with a number of miliseconds to clear the display after
// 3. void display.timerRun(); to make the timer in the new display library run

Adafruit_SH1107_Ext display = Adafruit_SH1107_Ext(D_HEIGHT, D_WIDTH, &Wire, -1, 1000000, 1000000);

//Timer
SimpleTimer timer;

#define NUM_TRAIN_COMMANDS 19        //Number of train functions
#define TRAIN_COMMAND_STR_LENGTH 32  //Max length of function description.  Note text may only be 31 characters as last character is the end of string character.
#define MAX_COMMAND_LENGTH 30        //Max DCC base station command length.  Defined in SerialCommand.h

//For Display Function
#define D_DELAY 1000  // how many miliseconds after which to clear the display

//RFID Reader Variables
//Information is stored in block 4 and 5 for total of 32 character of storage
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

//Function Declarations
//Helper Functions
// Sends data from the RFID board to central board via Serial1.  Takes optional delay parameter
void sendDataCentral(int data, unsigned long int delay = 0);
//RFID loop
void RFID();
// Returns the index of a of a train function string in the trainCommandTextArray[] or -1 if string is not in trainCommandTextArray[]
int tFIMatch(byte tagText[]);
//Sends Data from RFID reader board to Central Board and Displays command name to RFID LCD display
void sendAndDisplayCommand(int trainCommandIndex);

//Add new train functions to this array
//Train functions must also be added to the function sendAndDisplayCommand()
byte trainCommandTextArray[NUM_TRAIN_COMMANDS][TRAIN_COMMAND_STR_LENGTH] = {
  "",                // 0
  "Long whistles",   // 1
  "Whistle",         // 2
  "Short whistle",   // 3
  "Bell",            // 4
  "Speed 20%",       // 5
  "Speed 40%",       // 6
  "Speed 60%",       // 7
  "Speed 80%",       // 8
  "Speed 100%",      // 9
  "Park Trigger 1",  // 10
  "Park Trigger 2",  // 11
  "Park",            // 12
  "Station 1",       // 13
  "Station 2",       // 14
  "Station 3",       // 15
  "Station 4",       // 16
  "Station 5",       // 17
  "Station 6",       // 18
};


//Sends Data from RFID reader board to Central Board and Displays command name to RFID LCD display
void sendAndDisplayCommand(int trainCommandIndex) {
  switch (trainCommandIndex) {
    case 0:  // Blank screen:
      display.centeredDisplay("", D_DELAY);
      break;

    case 1:  // Long whistles:
      sendDataCentral(1);
      display.centeredDisplay("Long", "whistles", D_DELAY);
      break;

    case 2:  // Whistle
      sendDataCentral(2);
      display.centeredDisplay("Whistle", D_DELAY);
      break;

    case 3:  // Short whistle:
      sendDataCentral(3);
      display.centeredDisplay("Short", "whistle", D_DELAY);
      break;

    case 4:  // Bell:
      sendDataCentral(4);
      display.centeredDisplay("Bell", D_DELAY);
      break;

    case 5:  // Speed 20%:
      sendDataCentral(5);
      display.centeredDisplay("Speed 20%", D_DELAY);
      break;

    case 6:  // Speed 40%:
      sendDataCentral(6);
      display.centeredDisplay("Speed 40%", D_DELAY);
      break;

    case 7:  // Speed 60%:
      sendDataCentral(7);
      display.centeredDisplay("Speed 60%", D_DELAY);
      break;

    case 8:  // Speed 80%:
      sendDataCentral(7);
      display.centeredDisplay("Speed 80%", D_DELAY);
      break;

    case 9:  // Speed 100%:
      sendDataCentral(7);
      display.centeredDisplay("Speed 100%", D_DELAY);
      break;

    case 10:  // Park Trigger 1:
      display.centeredDisplay("Park", "trigger 1", D_DELAY);
      sendDataCentral(8);
      break;

    case 11:  // Park Trigger 2:
      sendDataCentral(9);
      display.centeredDisplay("Park", "trigger 2", D_DELAY);
      break;

    case 12:  // Park:
      sendDataCentral(10);
      display.centeredDisplay("Park", D_DELAY);
      break;

    case 13:  // Station 1:
      sendDataCentral(11);
      display.centeredDisplay("Station 1", D_DELAY);
      break;

    case 14:  // Station 2:
      sendDataCentral(12);
      display.centeredDisplay("Station 2", D_DELAY);
      break;

    case 15:  // Station 3:
      sendDataCentral(13);
      display.centeredDisplay("Station 3", D_DELAY);
      break;      

    case 16:  // Station 4:
      sendDataCentral(14);
      display.centeredDisplay("Station 4", D_DELAY);
      break;

    case 17:  // Station 5:
      sendDataCentral(15);
      display.centeredDisplay("Station 5", D_DELAY);
      break;

    case 18:  // Station 6:
      sendDataCentral(16);
      display.centeredDisplay("Station 6", D_DELAY);
      break;

    default:
      display.centeredDisplay("!No Match!", D_DELAY);
  }
}

void setup() {
  pinMode(RFID_READY_PIN, OUTPUT);

  Serial1.begin(115200);

  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
 
  //Set up Display
  display.begin(0x3C, true);
  display.setRotation(3);                            //Landscape Mode with USB at top left
  display.setTextSize(2);                            //Character are 10 pixels wide and 16 pixels high
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);  //Added a back ground color for printing text, so can overwrite other characters

  digitalWrite(RFID_READY_PIN, LOW);  //  LOW output required to achieve 'Train ready' state on Due
  display.centeredDisplay("RFID", "Ready", D_DELAY);

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) { key.keyByte[i] = 0xFF; }
}

void loop() {
  display.timerRun();  //Display Timer
  timer.run();         //Timer
  RFID();
}

//Helper Functions

// Sends data from the RFID board to central board via Serial1.  Takes optional delay parameter
void sendDataCentral(int data, unsigned long int delay) {
  if (!delay) {  //delay is 0, execute immediately
    if (Serial1.availableForWrite()) {
      Serial1.write(data);
    }
  } else {  //delay is longer than 0, execute after delay through timer library
    timer.setTimeout(delay,
                     [=]() {
                       Serial1.write(data);
                     });
  }
}

void RFID() {
  //Loop does not get past here unless a new card is present
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) return;


  //Block to get info from a tag / card
  // e use the second sector, that is: sector #1, covering block #4 up to and including block #7
  byte blockAddr = 4;
  byte trailerBlock = 7;
  MFRC522::StatusCode status;
  byte buffer[34];  //Store information read from the tag
  byte size = sizeof(buffer);

  // Authenticate using key A
  status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) { return; }

  // Read data from the block
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);  //First 16 byte block
  if (status != MFRC522::STATUS_OK) { return; }
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr + 1, &buffer[16], &size);  //Second 16 byte block
  if (status != MFRC522::STATUS_OK) { return; }

  sendAndDisplayCommand(tFIMatch(buffer));  //send tag data stored in buffer to be matched to the trainCommandTextArray and reset the train Func Index

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// Returns the index of a of a train function string in the trainCommandTextArray[] or -1 if string is not in trainCommandTextArray[]
int tFIMatch(byte tagText[]) {
  int returnVal = NUM_TRAIN_COMMANDS + 1;                            // tag text does not match anything in the trainCommandTextArray[]
  for (int i = 0; i < NUM_TRAIN_COMMANDS; i++) {                     // Set returnVal to something other than -1 if tagText[] is in trainCommandTextArray
    if (!strcmp((char*)tagText, (char*)trainCommandTextArray[i])) {  // Note the '!' (not bit operator) here bc strcmp returns 0 (zero) if the strings match
      returnVal = i;
    }
  }
  return returnVal;
}