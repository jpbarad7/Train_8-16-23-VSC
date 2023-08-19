#include <SPI.h>
#include <Wire.h>
#include <Adafruit_SH1107_Ext.h>
#include <MFRC522.h>
#include <SimpleTimer.h>

//Pin Definitions
#define RST_PIN 13
#define SS_PIN 12
#define BUTTON_A 5
#define BUTTON_B 6
#define BUTTON_C 9

// Diplay parameters
#define D_HEIGHT 64   //Display height
#define D_WIDTH 128   //Display width
#define D_DELAY 1000  // how many miliseconds after which to clear the display

//Display instance
//Note this is our _Ext or extention of the Adafruit library to include the following functions:
// 1. void display.centeredDisplay("line1", ...); with up to four lines as arguments with an optional last numeric delay parameter which clears the display after delay
// 2. void display.clear_Disp(); can be called with no argument (executes immediately)
//or with a number of miliseconds to clear the display after
// 3. void display.timerRun(); to make the timer in the new display library run

Adafruit_SH1107_Ext display = Adafruit_SH1107_Ext(D_HEIGHT, D_WIDTH, &Wire, -1, 1000000, 1000000);

//RFID Reader Variables              //Information is stored in block 4 and 5 for total of 32 character of storage
MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

//State Variable to indicate whether wand is in Read Mode or Write Mode
#define WRITE_MODE 0
#define READ_MODE 1
int mode = READ_MODE;

//Used to create an array of train commands
#define NUM_TRAIN_COMMANDS 19        //Number of train functions
#define TRAIN_COMMAND_STR_LENGTH 32  //Max length of function description.  Note text may only be 31 characters.
#define MAX_COMMAND_LENGTH 30        //Max DCC base station command length.  Defined in SerialCommand.h

// Index for the trainCommandTextArray and the trainComandDccArray
int trainCommandIndex = 0;

//Simple Timer to debounce buttons
//Will call timer.setInterval() in setup
SimpleTimer timer;

//Function Declarations
//Helper Functions
//RFID loop
void RFID();
// Returns the index of a of a train function string in the trainCommandTextArray[] or -1 if string is not in trainCommandTextArray[]
int tFIMatch(byte tagText[]);
// Increments the train function index
// Called by button A (increases) and Button C (Decreases)
void tFIIncrementer(int inc);
//Displays Read Mode or Write Mode or train command.  Takes var which indicates whether to display DISPLAY_COMMAND ie to display a train command or DISPLAY_MODE to display Read or Write Mode
void commandDisplay(int command);
//Responds to buttons
void buttons();

//Add new train functions to this array
//Train functions must also be added to the function commandDisplay()
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

void commandDisplay(int command) {  //var is either DISPLAY_MODE or DISPLAY_COMMAND
  switch (command) {
    case 0:  // Blank screen:
      display.centeredDisplay("", D_DELAY);
      break;

    case 1:  // Long whistles:
      display.centeredDisplay("Long", "whistles", D_DELAY);
      break;

    case 2:  // Whistle
      display.centeredDisplay("Whistle", D_DELAY);
      break;

    case 3:  // Short whistle:
      display.centeredDisplay("Short", "whistle", D_DELAY);
      break;

    case 4:  // Bell:
      display.centeredDisplay("Bell", D_DELAY);
      break;

    case 5:  // Speed 20%:
      display.centeredDisplay("Speed 20%", D_DELAY);
      break;

    case 6:  // Speed 40%:
      display.centeredDisplay("Speed 40%", D_DELAY);
      break;

    case 7:  // Speed 60%:
      display.centeredDisplay("Speed 60%", D_DELAY);
      break;

    case 8:  // Speed 80%:
      display.centeredDisplay("Speed 80%", D_DELAY);
      break;

     case 9:  // Speed 100%:
      display.centeredDisplay("Speed 100%", D_DELAY);
      break;           

    case 10:  // Park Trigger 1:
      display.centeredDisplay("Park", "trigger 1", D_DELAY);
      break;

    case 11:  // Park Trigger 2:
      display.centeredDisplay("Park", "trigger 2", D_DELAY);
      break;

    case 12:  // Park:
      display.centeredDisplay("Park", D_DELAY);
      break;

    case 13:  // Station 1:
      display.centeredDisplay("Station 1", D_DELAY);
      break;

    case 14:  // Station 2:
      display.centeredDisplay("Station 2", D_DELAY);
      break;

    case 15:  // Station 3:
      display.centeredDisplay("Station 3", D_DELAY);
      break;  

    case 16:  // Station 4:
      display.centeredDisplay("Station 4", D_DELAY);
      break; 

    case 17: // Station 5:
      display.centeredDisplay("Station 5", D_DELAY);
      break;  

    case 18: // Station 6:
      display.centeredDisplay("Station 6", D_DELAY);
      break;  

    default:
      display.centeredDisplay("!No Match!", D_DELAY);
      
  }
}

void loop() {
  //Read RFID Tags
  RFID();
  //SimpleTimer to debounce buttons by checking buttons() function every 200 miliseconds through timer.setInterval in setup()
  timer.run();
  //Used by display instance
  display.timerRun();
}

void setup() {
Serial.begin(115200);

  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
  // Prepare the RFID key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  //Set up Display
  display.begin(0x3C, true);
  display.setRotation(3);                            //Landscape Mode with USB at top left
  display.setTextSize(2);                            //Character are 10 pixels wide and 16 pixels high
  display.setTextColor(SH110X_WHITE, SH110X_BLACK);  //Added a back ground color for printing text, so can overwrite other characters

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // Initial Call to display something
  display.centeredDisplay("Read", "Mode", D_DELAY);  //Code starts in read mode

  //Debounce by checking buttons every 200 miliseconds instead of constantly
  timer.setInterval(200, buttons);
}

//Helper Functions
//Responds to buttons
void buttons() {
  //Respond to Button A and C.  Increment or decrement trainCommandIndex
  // Only makes sense to respond to Button A and C, if in Write Mode ie does nothing in Read Mode or Train Mode
  if (mode == WRITE_MODE) {
    //Respose to Button A: move foward in the list of train commands
    if (!digitalRead(BUTTON_A)) {
      tFIIncrementer(1);
      commandDisplay(trainCommandIndex);
    }

    //Response to Button C: move backward in the list of train commands
    if (!digitalRead(BUTTON_C)) {
      tFIIncrementer(-1);
      commandDisplay(trainCommandIndex);
    }
  }

  //Changes mode of Wand
  if (!digitalRead(BUTTON_B)) {
    if (mode == WRITE_MODE) {  // IF in  Write Mode
      mode = READ_MODE;        // Change to Read Mode
      display.centeredDisplay("Read", "Mode", D_DELAY);
    } else {  // In Read Mode, Change to Write Mode
      mode = WRITE_MODE;
      display.centeredDisplay("Write", "Mode", D_DELAY);
    }
  }
}

// RFID loop
void RFID() {
  // Loop does not get past here unless a new card is present
  // Block to get info from a tag / card
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) return;


  // In this sample we use the second sector, that is: sector #1, covering block #4 up to and including block #7
  byte blockAddr = 4;
  byte trailerBlock = 7;
  MFRC522::StatusCode status;
  byte buffer[34];  // Store information read from the tag
  byte size = sizeof(buffer);

  // Authenticate using key A
  status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) { return; }

  // Read data from the block
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);           //First 16 byte block
  status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr + 1, &buffer[16], &size);  //Second 16 byte block
  //END Block to get info from a tag / card

  //If in Read mode, Display which train Function it stores or report error.
  if (mode == READ_MODE) {
    commandDisplay(tFIMatch(buffer));  //send tag data stored in buffer to be matched to the trainCommandTextArray and reset the train Func Index
  }

  //If in Write Mode, write the tag with the train function
  if (mode == WRITE_MODE) {
    // Write data to the block
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, trainCommandTextArray[trainCommandIndex], 16);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr + 1, &trainCommandTextArray[trainCommandIndex][16], 16);

    // Check that newly written card data is correct
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr + 1, &buffer[16], &size);
    if (!strcmp((char*)buffer, (char*)trainCommandTextArray[trainCommandIndex])) {
      display.centeredDisplay("Write", "Successful", D_DELAY);
    }
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

// Increments the train function index
// Called by button A (increases) and Button C (Decreases)
void tFIIncrementer(int inc) {
  trainCommandIndex += inc;

Serial.println(trainCommandIndex);

  if (trainCommandIndex < 0) {
    trainCommandIndex = NUM_TRAIN_COMMANDS - 1;
  }
  if (trainCommandIndex > NUM_TRAIN_COMMANDS - 1) {
    trainCommandIndex = 0;
  }
}

// Returns the index of a of a train function string in the trainCommandTextArray[] or -1 if string is not in trainCommandTextArray[]
int tFIMatch(byte tagText[]) {
  int returnVal = NUM_TRAIN_COMMANDS + 1;                            //tag text does not match anything in the trainCommandTextArray[]
  for (int i = 0; i < NUM_TRAIN_COMMANDS; i++) {                     //Set returnVal to something other than -1 if tagText[] is in trainCommandTextArray
    if (!strcmp((char*)tagText, (char*)trainCommandTextArray[i])) {  ///Note the '!' (not bit operator) here bc strcmp returns 0 (zero) if the strings match
      returnVal = i;
    }
  }
  return returnVal;
}