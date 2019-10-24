/*
 ALTURA MIDI Theremin by Zeppelin Design Labs LLC, 2017
  by Thomas K Wray & Glen van Alkemade. Code inspirations included:
  thereThing
  MiniWI
*/

#define _TASK_MICRO_RES

#include <TaskScheduler.h> //https://github.com/arkhipenko/TaskScheduler

#include <MIDI.h>
#include <EEPROMex.h>

#include <EightSegmentDisplayableCharacters.h>
#include <EightSegmentDisplay.h>

MIDI_CREATE_DEFAULT_INSTANCE();

const byte buttonPinUp = 21;
const byte buttonPinDown = 5;
const byte buttonPinSave = 20;

// Potentiometer layout with their multiplex
// channel number.
//
//                          
//                        
//
//  2   4   1   3   5   0   6 
// +-+ +-+ +-+ +-+ +-+ +-+ +-+
// |o| |o| |o| |o| |o| |o| |o|
// +-+ +-+ +-+ +-+ +-+ +-+ +-+  

const byte dataFarPot        = 2;
const byte dataNearPot     	 = 1;
const byte functionSelectPot = 4;
const byte keyPot            = 6;
const byte scalePot          = 5;
const byte octaveNearPot     = 0;
const byte octaveFarPot      = 3;

//###################################################
//          Pin Definitions
//###################################################

//sensors
const byte rightTriggerPin = 9;
const byte rightEchoPin = 3;
const byte leftTriggerPin = 4;
const byte leftEchoPin = 2;

//multiplexer -------------------------------------
const byte multiplexChannelSelectMSBPin = 6;
const byte multiplexChannelSelectMiddleBitPin = 7;
const byte multiplexChannelSelectLSBPin = 8;

#define multiplexDataPin A5

//8-Segment LED Display --------------------------
const byte ledLatch = 14;
const byte ledClock = 15;
const byte ledData = 10;
const byte ledLeftDigitPin = 18;
const byte ledMiddleDigitPin = 17;
const byte ledRightDigitPin = 16;

byte ledLeftDigit = 0;  
byte ledMiddleDigit = 0;
byte ledRightDigit = 0;
byte displayPriority = 0;

byte ledA = 6;
byte ledB = 4;
byte ledC = 2;
byte ledD = 0;
byte ledE = 7;
byte ledF = 5;
byte ledG = 3;
byte ledDp = 1;
byte segments[8] = {ledA,ledB,ledC,ledD,ledE,ledF,ledG,ledDp};

EightSegmentDisplay display1(segments);

//###################################################
//          Global Variables
//###################################################


//Sensor Setup -----------------------------------------                     
const byte leftHandBufferAmmount = 4; //try to keep as a power of 2

const int minimumDistance = 350;
const int maximumDistance = 3000;
const int sensorTimeOut = 4000;
const int noteBufferSpace = 720;


unsigned long leftSensorProcessed=350;
unsigned long rightSensorProcessed=350; 

//Notes Setup -----------------------------------------
byte notesInCurrentScale = 15; 
byte scaleCurrent;
byte keyCurrent;

int noteBuffer = noteBufferSpace / notesInCurrentScale;

int lastNote = 0;

const byte octaveMax = 8;
byte octaveNearCurrent = 5;
byte octaveFarCurrent = 4;
byte numberOfOctavesCurrent = 2;
bool descending = true;
byte nearNotePointer = 1;
byte farNotePointer = 8;

const byte scales[12][13] = {       //The value in the last column indicates the number of notes in the scale.
                  
  { 2, 2, 1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 7 }, // Ionian Mode (Major)
  { 2, 1, 2, 2, 2, 1, 2, 0, 0, 0, 0, 0, 7 }, // Dorian Mode
  { 1, 2, 2, 2, 1, 2, 2, 0, 0, 0, 0, 0, 7 }, // Phrygian Mode
  { 2, 2, 2, 1, 2, 2, 1, 0, 0, 0, 0, 0, 7 }, // Lydian Mode
  { 2, 2, 1, 2, 2, 1, 2, 0, 0, 0, 0, 0, 7 }, // Mixolydian
  { 2, 1, 2, 2, 1, 2, 2, 0, 0, 0, 0, 0, 7 }, // Aeolian Mode (Natural Minor)
  { 1, 2, 2, 1, 2, 2, 2, 0, 0, 0, 0, 0, 7 }, // Locrian Mode
  { 2, 1, 2, 2, 1, 3, 1, 0, 0, 0, 0, 0, 7 }, // Harmonic Minor
  { 2, 2, 3, 2, 3, 0, 0, 0, 0, 0, 0, 0, 5 }, // Major Pentatonic
  { 3, 2, 2, 3, 2, 0, 0, 0, 0, 0, 0, 0, 5 }, // Minor Pentatonic
  { 2, 2, 2, 2, 2, 2, 0, 0, 0, 0, 0, 0, 6 }, // Whole Tone
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 12}, // Chromatic
  };	

//Pot Functionality Setup -----------------------------
byte potLock[7] = {3,3,3,3,3,3,3};
int oldReading[7] = {-5,-5,-5,-5,-5,-5,-5};
byte functionSelectCurrent;

int dataFar = 127;
int dataNear = 0;

int xyDataFarRight = 127;
int xyDataNearRight = 0;

int xyLeftControlChange = 85;
int xyRightControlChange = 86;

bool xyMode = false;

bool arpMode = false;

bool articulationMode = true;

byte articulation = 15;
byte arpSpeed = 4;
byte tapDivisor = 4;
unsigned long articulationTime = 15000;
unsigned long arpArticulationTime = 15000;

int arpegio[8] = {0, 0, 0, 0, 0, 0, 0, 0};
const byte arpDisplay[3][18] = {
  {48, 58, 51, 64, 64, 64, 64, 64, 64, 64, 64, 64 ,64, 64, 64, 64, 64, 64},
  {55, 59, 54, 71, 71, 71, 71, 71, 71, 71, 64, 64, 64, 64, 64, 64, 64, 64},
  {47, 60, 47,  1,  2,  3,  4,  5,  6,  7,  1,  2,  3,  4,  5,  6,  7, 17},
};

const byte arpDisplayPentatonic[3][14] = {
  {48, 58, 51, 64, 64, 64, 64, 64, 64, 64 ,64, 64, 64, 64},
  {55, 59, 54, 71, 71, 71, 71, 71, 64, 64, 64, 64, 64, 64},
  {47, 60, 47,  1,  2,  3,  4,  5,  1,  2,  3,  4,  5, 17},
};

const byte arpDisplayWhole[3][16] = {
  {48, 58, 51, 64, 64, 64, 64, 64, 64, 64, 64 ,64, 64, 64, 64, 64},
  {55, 59, 54, 71, 71, 71, 71, 71, 71, 64, 64, 64, 64, 64, 64, 64},
  {47, 60, 47,  1,  2,  3,  4,  5,  6,  1,  2,  3,  4,  5,  6, 17},
};
const byte arpDisplayChromatic[3][28] = {
  {48, 58, 51, 64, 64, 64, 64, 64, 64, 64, 64, 64, 71, 71, 71, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64},
  {55, 59, 54, 71, 71, 71, 71, 71, 71, 71, 71, 71,  1,  1,  1, 64, 64, 64, 64, 64, 64, 64, 64, 64,  1,  1,  1, 64},
  {47, 60, 47,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  1,  2,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  1,  2, 17},
};

//MIDI Packet Data -------------------------------------
int pitchBendNeutralZone = 10;
int  pitchBendUp = 1700;
int  pitchBendDown = 1700;

bool portamentoOn = false;
byte portamentoTime = 0;

byte noteVelocity = 127;
byte channelVolume = 127;
byte modulation = 127;

byte midiChannel = 1;

long shortTimeoutTime = 2000000;
long longTimeoutTime = 60000000;

struct Preset{
 byte velocity;
 byte channel;
 byte key;
 byte mode;
 byte octaveNear;
 byte octaveFar;
 byte octaveNumber;
 bool descending;
 byte arpegio1;
 byte arpegio2;
 byte arpegio3;
 byte arpegio4;
 byte arpegio5;
 byte arpegio6;
 byte arpegio7;
 byte volume;
 byte modulation;
 byte portTime;
 byte portOn;
 byte dataNear;
 byte dataFar;
 byte function;
 unsigned long tempo;
}currentPreset;

const byte presetRoot = 10;
byte presetNumber = 0;
bool scrollPreset = false;
byte midiNotes[96];

volatile unsigned long rightSensorRaw = 0;
volatile unsigned long leftSensorRaw = 0;
volatile unsigned long rightSensorTimer = 0;
volatile unsigned long leftSensorTimer = 0;
   

//Scheduler
Scheduler ts;

void scopefix(){}

Task Display(1000, TASK_FOREVER, &runDisplay, &ts);
Task LeftSensor(15000, TASK_FOREVER, &pingLeftSensor, &ts);
Task RightSensor(25000, TASK_FOREVER, &pingRightSensor, &ts);
Task Pots(20000, TASK_FOREVER, &checkPots, &ts);
Task Buttons(20000, TASK_FOREVER, &checkSwitches, &ts);
Task ShortTimeout(TASK_IMMEDIATE, TASK_FOREVER, &defaultDisplay, &ts);
Task LongTimeout(TASK_IMMEDIATE, TASK_FOREVER, &wipeDisplay, &ts);
Task StartupAnimation(45000, TASK_FOREVER, &startupDisplay, &ts);
Task PresetButtonTap(TASK_IMMEDIATE, TASK_FOREVER, &readPreset, &ts);
Task PresetButtonHold(TASK_IMMEDIATE, TASK_FOREVER, &writePreset, &ts);
Task ShiftButton(TASK_IMMEDIATE, TASK_FOREVER, &scrollPresetUp, &ts);
Task TapButton(TASK_IMMEDIATE, TASK_FOREVER, &scrollPresetDown, &ts);
Task LeftSensorTimeout(TASK_IMMEDIATE, TASK_FOREVER, &leftPingTimeout, &ts);
Task RightSensorTimeout(TASK_IMMEDIATE, TASK_FOREVER, &rightPingTimeout, &ts);


void checkSwitches(){
  static bool buttonUp = true;
  static bool buttonDown = true;
  static bool buttonSave = true;
  static unsigned long timestamp = 0;
  static byte i = 0;

  switch (i){
    case 0:
      if (digitalRead(buttonPinUp) != buttonUp){
        if (buttonUp){
         //on press
         buttonUp = false;
          }
           else{
         //on release
         ShiftButton.enable();
         buttonUp = true;
          }
      }
      i = 1;
      break;
    case 1:
    if (digitalRead(buttonPinDown) != buttonDown){
    if (buttonDown){
       //on press
       buttonDown = false;
    }
    else{
      //on release
      TapButton.enable();
      buttonDown = true;
    }
  }
      i = 2;
     break;
    case 2:
        if (digitalRead(buttonPinSave) != buttonSave){
    if (buttonSave){
       //on press
       timestamp = millis();
       ShortTimeout.disable();
       digitSplit(presetNumber);
       buttonSave = false;
    }   
    else{
      //on release
      if (millis() - timestamp > 2000){

       PresetButtonHold.enable();
      }
      else      {
       PresetButtonTap.enable();
      }
      ShortTimeout.enableDelayed(2000000);
      buttonSave = true;
    }
  } 
     if (millis() - timestamp > 2000 && digitalRead(buttonPinSave)== LOW){
      ledLeftDigit = 59;
      ledMiddleDigit = 44;
      ledRightDigit = 62;
      startTimerWithPriority(3); 
 }
      i = 0;
     break;
    default:
    break;
  }
}

void speedUp(){
  articulation++;
  articulationTime = 15 + articulation;
  articulationTime = articulationTime * 1000;
  articulationTime = constrain(articulationTime, 15000, 270000);
  RightSensor.setInterval(articulationTime);
  digitSplit(15+articulation);
  ShiftButton.disable(); 
    
}

void slowDown(){
  articulation--;
  articulationTime = 15 + articulation;
  articulationTime = articulationTime * 1000;
  articulationTime = constrain(articulationTime, 15000, 270000);
  RightSensor.setInterval(articulationTime);
  digitSplit(15+articulation);
  TapButton.disable();     
}

void saveArticulation(){
  EEPROM.writeByte(1, articulation);
  ledRightDigit = 10;
  PresetButtonHold.disable();
}

void readArticulation(){
  articulation = EEPROM.readByte(1);
  articulationTime = 15 + articulation;
  articulationTime = articulationTime * 1000;
  articulationTime = constrain(articulationTime, 15000, 270000);
  RightSensor.setInterval(articulationTime);
  digitSplit(15+articulation); 
   PresetButtonTap.disable();  
}

/*
void debugNotes(){
  for (int i = 0; i <= notesInCurrentScale; i++){
    if (i > 0){
    MIDI.sendNoteOn(midiNotes[i - 1], 0, midiChannel);
    }    
    MIDI.sendNoteOn(midiNotes[i], 127, midiChannel);
      delay(100);
  }
  MIDI.sendNoteOn(midiNotes[notesInCurrentScale], 0, midiChannel);
}
*/
void scrollPresetUp(){
  if (scrollPreset){
  if (presetNumber == 20){
    presetNumber = 0;
  }
  else {presetNumber++;}
  }
  scrollPreset = true;
  digitSplit(presetNumber);
  startTimerWithPriority(1);
  ShiftButton.disable();
}
void scrollPresetDown(){
  if (scrollPreset){
   if (presetNumber == 0){
    presetNumber = 20;
  }
  else {presetNumber--;}
  }
  scrollPreset = true;
  digitSplit(presetNumber);
  startTimerWithPriority(1);
  TapButton.disable();
}

void readPreset(){
  lockPots();
  EEPROM.readBlock(presetRoot + (presetNumber * sizeof(currentPreset)), currentPreset);
  noteVelocity = constrain(currentPreset.velocity, 0, 127);
  midiChannel = constrain(currentPreset.channel, 1, 16);
  keyCurrent = constrain(currentPreset.key, 0, 11);
  scaleCurrent = constrain(currentPreset.mode, 0, 11);
  octaveNearCurrent = constrain(currentPreset.octaveNear, 1, 8);
  octaveFarCurrent = constrain(currentPreset.octaveFar, 1, 8);
  numberOfOctavesCurrent = constrain(currentPreset.octaveNumber, 1, 8);
  descending = currentPreset.descending;
  notesInCurrentScale = 8 * scales[scaleCurrent][12] ;
  noteBuffer =  noteBufferSpace / (numberOfOctavesCurrent * 8);
  SetScale();
    arpegio[1] = constrain(currentPreset.arpegio1, 0, 17);
    arpegio[2] = constrain(currentPreset.arpegio2, 0, 17);
    arpegio[3] = constrain(currentPreset.arpegio3, 0, 17);
    arpegio[4] = constrain(currentPreset.arpegio4, 0, 17);
    arpegio[5] = constrain(currentPreset.arpegio5, 0, 17);
    arpegio[6] = constrain(currentPreset.arpegio6, 0, 17);
    arpegio[7] = constrain(currentPreset.arpegio7, 0, 17);
  dataNear = constrain(currentPreset.dataNear,0,127);
  dataFar = constrain(currentPreset.dataFar,0,127);
  channelVolume = constrain(currentPreset.volume, 0, 127);
  modulation = constrain(currentPreset.modulation, 0, 127);
  portamentoTime = constrain(currentPreset.portTime, 0, 127);
  portamentoOn = currentPreset.portOn;
  arpArticulationTime = constrain(currentPreset.tempo, 15000, 270000);
  functionSelectCurrent = constrain(currentPreset.function, 1, 8);
  if(functionSelectCurrent == 8){
   RightSensor.setInterval(arpArticulationTime);
  }
  else{ 
   RightSensor.setInterval(articulationTime);
  }
  MIDI.sendControlChange(65,portamentoOn,midiChannel);
  MIDI.sendControlChange(5,portamentoTime,midiChannel);
  MIDI.sendControlChange(7,channelVolume,midiChannel);
  MIDI.sendControlChange(1,modulation,midiChannel);
  if (functionSelectCurrent == 6){
    xyModeStart();
  }
  else if (functionSelectCurrent == 8){
    arpModeStart();
  }
  else{
    xyModeStop();
    arpModeStop();
  }
  ledLeftDigit = 58;
  ledMiddleDigit = 46;
  ledRightDigit = 54;
  startTimerWithPriority(3);
  PresetButtonTap.disable();
}

void writePreset(){
  currentPreset = {noteVelocity, midiChannel, keyCurrent, scaleCurrent, octaveNearCurrent, 
  octaveFarCurrent, numberOfOctavesCurrent, descending, arpegio[1],arpegio[2],arpegio[3],arpegio[4],arpegio[5],arpegio[6],arpegio[7],
  channelVolume, modulation, portamentoTime, portamentoOn, dataNear, dataFar, functionSelectCurrent, arpArticulationTime};
  EEPROM.writeBlock( presetRoot + (presetNumber * sizeof(currentPreset)), currentPreset);
  PresetButtonHold.disable();
  }

void toggleArpegioSetup(){
  static bool isOn = false;
  isOn = !isOn;
  lockPots();
  if (isOn){
  ledLeftDigit = 44;
  ledMiddleDigit = 58;
  ledRightDigit = 57;
  startTimerWithPriority(4);
  Pots.setCallback(&arpPots);
  TapButton.setCallback(&tapTime);
  }
  else {
    ledLeftDigit = 60;
    ledMiddleDigit = 51;
    ledRightDigit = 58;
    startTimerWithPriority(4);
    Pots.setCallback(&checkPots);
    TapButton.setCallback(&scrollPresetDown);
  }
  ShiftButton.disable();
}

void tapTime(){
  static unsigned long timestamp = 0;
  
  unsigned long newTime = constrain((micros() - timestamp)/tapDivisor, 15000, 270000);
  if (newTime != 270000){
  arpArticulationTime = newTime;
  RightSensor.setInterval(arpArticulationTime); 
  }
  timestamp = micros();
   TapButton.disable();
}

//###################################################
//###################################################

//General IO
int readMultiplex(byte address){	
    digitalWrite(multiplexChannelSelectMSBPin, bitRead(address, 0));
    digitalWrite(multiplexChannelSelectMiddleBitPin, bitRead(address, 1));
    digitalWrite(multiplexChannelSelectLSBPin, bitRead(address, 2));
    return analogRead(multiplexDataPin);
  }


//Note selection
void SetScale() {
  midiNotes[0] = keyCurrent + 12;
  for (int note = 0; note < notesInCurrentScale; note++) {
    midiNotes[note + 1] = midiNotes[note] + scales[scaleCurrent][note % scales[scaleCurrent][12]];
    }
    if (descending){
    nearNotePointer = octaveNearCurrent * scales[scaleCurrent][12];
    farNotePointer = (octaveFarCurrent - 1) * scales[scaleCurrent][12];
    }
    else {
    nearNotePointer = (octaveNearCurrent - 1) * scales[scaleCurrent][12];
    farNotePointer = octaveFarCurrent  * scales[scaleCurrent][12];
    }
  }

//###################################################
//        Display
//###################################################

void runDisplay() {
	static byte i = 0;
	switch 	(i){
case 0:	
    lightDigit(display[ledLeftDigit], ledRightDigitPin, ledLeftDigitPin);
	i = 1;
	break;
case 1:		
    lightDigit(display[ledMiddleDigit], ledLeftDigitPin, ledMiddleDigitPin);
	i = 2;
	break;
case 2:
    lightDigit(display[ledRightDigit], ledMiddleDigitPin, ledRightDigitPin);
	i = 0;
	break;
default:
	i = 0;
	break;
	}
}

void lightDigit(byte displayNumber, byte digitOff, byte digitOn) {
  digitalWrite(digitOff, LOW);
  digitalWrite(ledLatch, LOW);
  shiftOut(ledData, ledClock, MSBFIRST, displayNumber);
  digitalWrite(ledLatch, HIGH);
  digitalWrite(digitOn, HIGH);
}

//The following two functions are split so two digit numbers can be displayed while the left digit is left alone to show something else.
void digitSplit2(int number) {
ledMiddleDigit = (number < 10 ? 64 : (number/10) % 10);
ledRightDigit = number % 10;
   }

void digitSplit(int number) {
ledLeftDigit = (number < 100 ? 64 : (number / 100) % 10 );  
digitSplit2(number);
  }  

  void startTimerWithPriority(byte priority) {
  
  ShortTimeout.enableDelayed(shortTimeoutTime);
  LongTimeout.enableDelayed(longTimeoutTime);
  displayPriority = priority;  //used to prevent certain sections from running until reverting to the default display
 }
 
 bool checkPriority(byte priority){
	 if (priority >= displayPriority){
		 return true;
	 }
	 return false;
 }
 void lockPots(){
  for (int i = 0; i <= 6;i++){
  potLock[i] = 255;
  }
 }
 
 bool outsidePotBuffer(int oldValue, int newValue, byte potNumber) {
  if (oldValue >= (newValue + potLock[potNumber]) || oldValue <= newValue - potLock[potNumber]){
   potLock[potNumber] = 3;
    return true;
  }
  return false;
}

void startupDisplay(){ 
static int j = 74;
static int k = 3;      
        ledLeftDigit = j;
        ledMiddleDigit = j;
        ledRightDigit = j;
		j++;
		if (j >= 82){
			k--;
			if (k <= 0){
        ledLeftDigit = 73;
        ledMiddleDigit = 73;
        ledRightDigit = 73;
        LeftSensor.enableDelayed(1000000);
        RightSensor.enableDelayed(1000000);
        Pots.enableDelayed(1000000);
        Buttons.enableDelayed(1000000);      
        StartupAnimation.disable();
			}
			j = 74;
      }
}

void displayKeyAndMode(){
  digitSplit2(scaleCurrent + 1);
  ledLeftDigit = keyCurrent + 32;  
}

void checkScalePot(int scalePot){
  
  int scaleNew = readMultiplex(scalePot);
    if (outsidePotBuffer(oldReading[scalePot], scaleNew, scalePot)) {
    scaleCurrent = map(scaleNew, 0, 1023, 0, 12);
    if (scaleCurrent > 11) {
      scaleCurrent = 11;
    }
  notesInCurrentScale = 8 * scales[scaleCurrent][12] ;
  noteBuffer =  noteBufferSpace / (numberOfOctavesCurrent * 8); 
   SetScale();               
    if (checkPriority(2)) {
      startTimerWithPriority(2);
      displayKeyAndMode();
    }
    oldReading[scalePot] = scaleNew;
  }
}

void checkKeyPot(int keyPot){
  int keyNew = readMultiplex(keyPot);
    if (outsidePotBuffer(oldReading[keyPot], keyNew, keyPot)) {
    keyCurrent = map(keyNew, 0, 1023, 0, 12);
    if (keyCurrent > 11) {
      keyCurrent = 11;
    }
    SetScale();   

    if (checkPriority(2)) {
      startTimerWithPriority(2);
      displayKeyAndMode();
    }
    oldReading[keyPot] = keyNew;
  }
}

void checkOctavePots (int octaveNearPot, int octaveFarPot){
  static int slopeCurrent = 1;
    static int slopeOld = 1;
  if (outsidePotBuffer(oldReading[octaveNearPot], readMultiplex(octaveNearPot), octaveNearPot) || outsidePotBuffer(oldReading[octaveFarPot], readMultiplex(octaveFarPot), octaveFarPot)) {
  if(readMultiplex(octaveNearPot) != oldReading[octaveNearPot]){
    octaveNearCurrent = map(readMultiplex(octaveNearPot), 0, 1023, 1, octaveMax+2);
        if (octaveNearCurrent > octaveMax) {
      octaveNearCurrent = octaveMax;
        }
  }
  if (readMultiplex(octaveFarPot) != oldReading[octaveFarPot]){
    octaveFarCurrent = map(readMultiplex(octaveFarPot), 0, 1023, 1, octaveMax+2);
        if (octaveFarCurrent > octaveMax) {
      octaveFarCurrent = octaveMax;
        }
  }

  slopeCurrent = (octaveFarCurrent + octaveMax) - octaveNearCurrent;
  if (slopeCurrent == octaveMax) {slopeCurrent = slopeOld;}
  if (slopeCurrent < octaveMax) {descending = true;}
  if (slopeCurrent > octaveMax) {descending = false;}

  numberOfOctavesCurrent = max(octaveFarCurrent, octaveNearCurrent) - min(octaveFarCurrent, octaveNearCurrent) + 1;

  notesInCurrentScale = 8 * scales[scaleCurrent][12] ;
  noteBuffer =  noteBufferSpace / (numberOfOctavesCurrent * 8); 
  SetScale();   
    if (outsidePotBuffer(oldReading[octaveFarPot], readMultiplex(octaveFarPot), octaveFarPot) || outsidePotBuffer(oldReading[octaveNearPot], readMultiplex(octaveNearPot), octaveNearPot)&& checkPriority(2) ) {
        startTimerWithPriority(2);
          ledLeftDigit = octaveNearCurrent;
          ledRightDigit = octaveFarCurrent;
          if (descending){
            ledMiddleDigit = 65;
          }
          else{
            ledMiddleDigit = 68;
          }
    }
    oldReading[octaveNearPot] = readMultiplex(octaveNearPot);
    oldReading[octaveFarPot] = readMultiplex(octaveFarPot);
    slopeOld = slopeCurrent;
  }
}

void checkFunctionPot(int functionSelectPot){
  int functionSelectNew = readMultiplex(functionSelectPot);
   if (outsidePotBuffer(oldReading[functionSelectPot], functionSelectNew, functionSelectPot)) {
    functionSelectCurrent = map(functionSelectNew, 0, 1023, 1, 9);
    if (functionSelectCurrent > 8) {
      functionSelectCurrent = 8;
    }
    if (checkPriority(3) && oldReading[functionSelectPot] > 0) {
      startTimerWithPriority(3);
      ledRightDigit = 64;
      ledMiddleDigit = 64;
      ledLeftDigit = functionSelectCurrent;
      
      //reset data to force it to adjust to the new setting
      oldReading[dataNearPot] = -1;    
      oldReading[dataFarPot]  = -1;
    }
  if (functionSelectCurrent == 6 && !xyMode){
    xyModeStart();
  }
  if (functionSelectCurrent != 6 && xyMode){
    xyModeStop();
  }
    if (functionSelectCurrent == 8 && !arpMode){
    arpModeStart();
  }
  if (functionSelectCurrent != 8 && arpMode){
    arpModeStop();
  }
    oldReading[functionSelectPot] = functionSelectNew;
  }
}


void checkDataPots(int dataNearPot, int dataFarPot){
   if (outsidePotBuffer(oldReading[dataNearPot] , readMultiplex(dataNearPot),dataNearPot)) {
    switch (functionSelectCurrent)
    {
      case 1:
        pitchBendNeutralZone = map(readMultiplex(dataNearPot), 0, 1023, 0, 127);
        if (outsidePotBuffer(oldReading[dataNearPot] , readMultiplex(dataNearPot),dataNearPot)&& checkPriority(2)) {
          startTimerWithPriority(2);
          digitSplit(pitchBendNeutralZone);
          oldReading[dataNearPot] = readMultiplex(dataNearPot);
        pitchBendUp = 1700 + pitchBendNeutralZone * 4;
        pitchBendDown = 1700 - pitchBendNeutralZone * 4;
        }
        break;
    case 7:
      break;
      default:
        if (outsidePotBuffer(oldReading[dataNearPot] , readMultiplex(dataNearPot),dataNearPot)&& checkPriority(2)) {
          dataNear = map(readMultiplex(dataNearPot), 0, 1023, 0, 127);
          startTimerWithPriority(2);
          digitSplit(dataNear);
           oldReading[dataNearPot] = readMultiplex(dataNearPot);
       }    
    }
  }

  if (outsidePotBuffer(oldReading[dataFarPot] , readMultiplex(dataFarPot),dataFarPot)) {
    switch (functionSelectCurrent)
    {
      case 1: 
        dataFar = map(readMultiplex(dataFarPot), 0, 1023, 0, 13);
        if (dataFar > 12) {
          dataFar = 12;
        }
        if (outsidePotBuffer(oldReading[dataFarPot] , readMultiplex(dataFarPot),dataFarPot)&& checkPriority(2)) {
          startTimerWithPriority(2);
          digitSplit(dataFar);
          oldReading[dataFarPot]  = readMultiplex(dataFarPot);
        }
        MIDI.sendControlChange(20, dataFar, midiChannel);
        break;
    case 7:
    if (checkPriority(1)){
      midiChannel = map(readMultiplex(dataFarPot),0,1023,1,17);
    if (midiChannel > 16) {
          midiChannel = 16;
    }    
    startTimerWithPriority(1);
          digitSplit(midiChannel);
    }
    break;
      default:
        dataFar = map(readMultiplex(dataFarPot), 0, 1023, 0, 127);
        if (outsidePotBuffer(oldReading[dataFarPot] , readMultiplex(dataFarPot),dataFarPot)&& checkPriority(2)) {
          startTimerWithPriority(2);
          digitSplit(dataFar);
          oldReading[dataFarPot]  = readMultiplex(dataFarPot);
        }
    }
    
  }
}

void xyModeStart(){
  xyMode = true;
  Pots.setCallback(&xyCheckPots);
  RightSensorTimeout.setCallback(&xyRightPingTimeout);
  ShortTimeout.setCallback(&xyDefaultDisplay);
  Buttons.disable();
}

void xyModeStop(){
  xyMode = false;
  Pots.setCallback(&checkPots);
  RightSensorTimeout.setCallback(&rightPingTimeout);
  ShortTimeout.setCallback(&defaultDisplay);
  Buttons.enable();
}

void arpModeStart(){
  arpMode = true;
  RightSensorTimeout.disable();
  LeftSensorTimeout.disable();
  detachInterrupt(digitalPinToInterrupt(rightEchoPin));  
  RightSensorTimeout.setCallback(&arpRightPingTimeout);
  RightSensor.setInterval(arpArticulationTime);
  ShiftButton.setCallback(&toggleArpegioSetup);
}

void arpModeStop(){
  arpMode = false;
  RightSensorTimeout.disable();
  LeftSensorTimeout.disable();
  detachInterrupt(digitalPinToInterrupt(rightEchoPin));  
  ShiftButton.setCallback(&scrollPresetUp);
   RightSensorTimeout.setCallback(&rightPingTimeout);
   RightSensor.setInterval(articulationTime);
   Pots.setCallback(&checkPots);
}

void xyCheckControlPots(int leftControlPot, int rightControlPot){
  if (outsidePotBuffer(oldReading[leftControlPot], readMultiplex(leftControlPot),leftControlPot)){
    xyLeftControlChange = map(readMultiplex(leftControlPot), 0, 1023, 0, 127);
  if (checkPriority(2)){
    startTimerWithPriority(2);
    digitSplit(xyLeftControlChange);
      oldReading[leftControlPot] = readMultiplex(leftControlPot);
  }
  }
  if (outsidePotBuffer(oldReading[rightControlPot], readMultiplex(rightControlPot),rightControlPot)){   
    xyRightControlChange = map(readMultiplex(rightControlPot), 0, 1023, 0, 127);
  if (checkPriority(2)){
    startTimerWithPriority(2);
    digitSplit(xyRightControlChange);
     oldReading[rightControlPot] = readMultiplex(rightControlPot);
  }
  }
}


void xyCheckRightDataPots(int rightDataNearPot, int rightDataFarPot){

  if (outsidePotBuffer(oldReading[rightDataFarPot] , readMultiplex(rightDataFarPot),rightDataFarPot)){ 
  xyDataFarRight = map(readMultiplex(rightDataFarPot), 0, 1023, 0, 127);
        if (checkPriority(2)) {
          startTimerWithPriority(2);
          digitSplit(xyDataFarRight);
              oldReading[rightDataFarPot]  = readMultiplex(rightDataFarPot);
        }
    }
  if (outsidePotBuffer(oldReading[rightDataNearPot] , readMultiplex(rightDataNearPot),rightDataNearPot)){ 
  xyDataNearRight = map(readMultiplex(rightDataNearPot), 0, 1023, 0, 127);
        if (checkPriority(2)) {
          startTimerWithPriority(2);
          digitSplit(xyDataNearRight);
            oldReading[rightDataNearPot] = readMultiplex(rightDataNearPot);
        }
    }


  }
  
  void checkPots(){
  checkOctavePots(octaveNearPot, octaveFarPot);
  checkFunctionPot(functionSelectPot);
  checkDataPots(dataNearPot,dataFarPot);
  checkScalePot(scalePot);
  checkKeyPot(keyPot);
}

void arpPots(){
 checkArpPot(dataFarPot, 1);
 checkArpPot(dataNearPot, 2);
 checkArpPot(functionSelectPot, 3);
 checkArpPot(keyPot, 4);
 checkArpPot(scalePot, 5);
 checkArpPot(octaveNearPot, 6);
 checkArpPot(octaveFarPot, 7);
}

  
void checkArpPot(int currentPot, int arpPosition){
static int reading = 0;
    if (outsidePotBuffer(readMultiplex(currentPot), oldReading[currentPot], currentPot)){
      oldReading[currentPot] = readMultiplex(currentPot);

if (scaleCurrent <= 7){
     reading = map(readMultiplex(currentPot), 0, 1023, 0, 17);
  if (reading != arpegio[arpPosition]){
 arpegio[arpPosition] = reading;
    ledLeftDigit = arpDisplay[0][reading];
    ledMiddleDigit = arpDisplay[1][reading];
    ledRightDigit = arpDisplay[2][reading];
    startTimerWithPriority(1);
    } 
}
else if(scaleCurrent <= 9){
   reading = map(readMultiplex(currentPot), 0, 1023, 0, 13);
  if (reading != arpegio[arpPosition]){
 arpegio[arpPosition] = reading;
    ledLeftDigit = arpDisplayPentatonic[0][reading];
    ledMiddleDigit = arpDisplayPentatonic[1][reading];
    ledRightDigit = arpDisplayPentatonic[2][reading];
    startTimerWithPriority(1);
    }
}
else if (scaleCurrent == 10){
  reading = map(readMultiplex(currentPot), 0, 1023, 0, 15);
  if (reading != arpegio[arpPosition]){
 arpegio[arpPosition] = reading;
    ledLeftDigit = arpDisplayWhole[0][reading];
    ledMiddleDigit = arpDisplayWhole[1][reading];
    ledRightDigit = arpDisplayWhole[2][reading];
    startTimerWithPriority(1);
    }
}
else if (scaleCurrent == 11){
  reading = map(readMultiplex(currentPot), 0, 1023, 0, 27);
  if (reading != arpegio[arpPosition]){
 arpegio[arpPosition] = reading;
    ledLeftDigit = arpDisplayChromatic[0][reading];
    ledMiddleDigit = arpDisplayChromatic[1][reading];
    ledRightDigit = arpDisplayChromatic[2][reading];
    startTimerWithPriority(1);
    }
    }
  }
}

void xyCheckPots(){
  xyCheckControlPots(keyPot, scalePot);
  checkDataPots(dataNearPot, dataFarPot);
  xyCheckRightDataPots(octaveNearPot, octaveFarPot);  
  checkFunctionPot(functionSelectPot);  
}

void defaultDisplay(){
  digitSplit2(scaleCurrent + 1);    
  ledLeftDigit = keyCurrent + 32;  
  displayPriority = 0;
  scrollPreset = false;
  ShortTimeout.disable();
}

void xyDefaultDisplay(){
    ledLeftDigit = 96;
    ledMiddleDigit = 96;
    ledRightDigit = 96;
    displayPriority = 0;
    scrollPreset = false;
    ShortTimeout.disable();
   
}

void wipeDisplay(){
    ledLeftDigit=64;
    ledMiddleDigit=64;
    ledRightDigit=64;
    displayPriority = 0;
      LongTimeout.disable();
}

long sensorConstrain(long reading){
  if (reading == 0 || reading > 3500){return 0;}
  if (reading <= minimumDistance){return minimumDistance;}
  if (reading >= maximumDistance){return maximumDistance;}
  return reading;
}

void pingLeftSensor(){
    attachInterrupt(digitalPinToInterrupt(leftEchoPin), startLeft, RISING);
    digitalWrite(leftTriggerPin, HIGH);
    delayMicroseconds(12);
    digitalWrite(leftTriggerPin, LOW); 
}

void startLeft(){
  leftSensorTimer = micros();
  leftSensorRaw = 0;   
  attachInterrupt(digitalPinToInterrupt(leftEchoPin), echoLeft, FALLING);
  LeftSensorTimeout.enableDelayed(4000);
}

void echoLeft(){
  leftSensorRaw = micros() - leftSensorTimer;
  detachInterrupt(digitalPinToInterrupt(leftEchoPin));
}

void leftPingTimeout(){
 static int counter = 0;
  detachInterrupt(digitalPinToInterrupt(leftEchoPin));
  leftSensorProcessed = stabilizeLeftReadings(sensorConstrain(leftSensorRaw));
   counter++;
   if (counter == 4){
   handleLeftSensor();
   counter = 0;
   }
  LeftSensorTimeout.disable();
}

void pingRightSensor(){
   
    attachInterrupt(digitalPinToInterrupt(rightEchoPin), startRight, RISING);
    digitalWrite(rightTriggerPin, HIGH);
    delayMicroseconds(12);
    digitalWrite(rightTriggerPin, LOW); 
}

void startRight(){ 
  rightSensorTimer = micros();
   rightSensorRaw = 0;
  attachInterrupt(digitalPinToInterrupt(rightEchoPin), echoRight, FALLING);
  RightSensorTimeout.enableDelayed(4000);   
}

void echoRight(){
  detachInterrupt(digitalPinToInterrupt(rightEchoPin));  
  rightSensorRaw = micros() - rightSensorTimer;
}

void rightPingTimeout(){
  static int counter = 0;
   detachInterrupt(digitalPinToInterrupt(rightEchoPin));
   rightSensorProcessed = checkNoteBuffer(stabilizeRightReadings(sensorConstrain(rightSensorRaw)));
   counter++;
   if (counter == 4){
   handleRightSensor(rightSensorProcessed);
    counter = 0;
   }
   RightSensorTimeout.disable();   
}

void xyRightPingTimeout(){
    detachInterrupt(digitalPinToInterrupt(rightEchoPin));
    xyHandleRightSensor(stabilizeRightReadings((sensorConstrain(rightSensorRaw))));
    RightSensorTimeout.disable();
}
void arpRightPingTimeout(){
  static int counter = 0;
    detachInterrupt(digitalPinToInterrupt(rightEchoPin));
    rightSensorProcessed = checkNoteBuffer(stabilizeRightReadings(sensorConstrain(rightSensorRaw)));
    counter++;
   if (counter >= arpSpeed){
   arpHandleRightSensor(rightSensorProcessed);
    counter = 0;
   }
    RightSensorTimeout.disable();
}
long stabilizeLeftReadings(long reading){
  static byte pointer = 0;
  static int leftReadings[leftHandBufferAmmount];
  static int counter;
  return reading;
if (reading == 0){
  if (counter >= 5){
     for (int j = 0; j < leftHandBufferAmmount; j++) {
      leftReadings[j] = 0;
      
     }
   return reading;
  }
  else{
    counter++;
    if (pointer == 0){
    reading = leftReadings[pointer] + (leftReadings[0] - leftReadings[3])/8;
    }
    else {
    reading = leftReadings[pointer] + (leftReadings[pointer] - leftReadings[pointer - 1]/8);
    }
    reading = constrain(reading, minimumDistance, maximumDistance);
  }
   }
   else{
    counter = 0;
   }
    pointer++;
     if (pointer >= leftHandBufferAmmount) {
        pointer = 0;
      }
  leftReadings[pointer] = reading;
     
     
      int readingsTotal = 0;
      for (int j = 0; j < leftHandBufferAmmount; j++) {
        readingsTotal = readingsTotal + leftReadings[j];
      }
      return readingsTotal / leftHandBufferAmmount;
}

long stabilizeRightReadings(long reading){
  static byte pointer = 0;
  static int rightReadings[leftHandBufferAmmount];
  static int counter;
  return reading;
 if (reading == 0){
  if (counter >= 5){
     for (int j = 0; j < leftHandBufferAmmount; j++) {
      rightReadings[j] = 0;
     }
   return reading;
  }
  else{
    counter++;
    if (pointer == 0){
    reading = rightReadings[pointer] + (rightReadings[0] - rightReadings[3])/8;
    }
    else {
    reading = rightReadings[pointer] + (rightReadings[pointer] - rightReadings[pointer - 1]/8);
    }
    reading = constrain(reading, minimumDistance, maximumDistance);
  }
   }
   else{
    counter = 0;
   }
  pointer++;
  if (pointer >= leftHandBufferAmmount) {
        pointer = 0;
      }
  rightReadings[pointer] = reading;
      
      
      int readingsTotal = 0;
      for (int j = 0; j < leftHandBufferAmmount; j++) {
        readingsTotal = readingsTotal + rightReadings[j];
      }
      return readingsTotal / leftHandBufferAmmount;
}

void handleVelocity(){
  if (leftSensorProcessed != 0) {
            noteVelocity = map(leftSensorProcessed, minimumDistance, maximumDistance, dataNear, dataFar);
            digitSplit(noteVelocity);
            startTimerWithPriority(1);
          }
}

void handlePitchBend(){
  static int  pitchBendOld = 0;
  static byte OutOfRangeL = 0;
  static byte spinDial = 29;
  int  pitchBend = 0;
  if (portamentoTime != 0) {
            portamentoOn = false;
            portamentoTime = 0;
            MIDI.sendControlChange(5, portamentoTime, midiChannel);
            MIDI.sendControlChange (65, 0, midiChannel);
          }
            if (leftSensorProcessed > pitchBendUp) {
              pitchBend = map(leftSensorProcessed, pitchBendUp, maximumDistance, 0, -1023);
            }
            else if (leftSensorProcessed < pitchBendDown) {
              pitchBend = map(leftSensorProcessed, minimumDistance, pitchBendDown, 1023, 0);
            }
            else
              pitchBend = 0;

            if (leftSensorProcessed == 0) {
              if (OutOfRangeL < 16) {
                OutOfRangeL++;
              }
              if (OutOfRangeL == 15)
              {

                ledRightDigit = 88;
                ledMiddleDigit = 64;
                ledLeftDigit = 64;
                startTimerWithPriority(1);

                MIDI.sendPitchBend( 0, midiChannel);
              }
            }
            else {
              if (pitchBend != pitchBendOld)
              {
                if (pitchBend > 0) {
                  spinDial = map(constrain(pitchBend, 0, 1023), 1023, 0 , 93, 88);
                  if (pitchBend == 1023){
                    spinDial++;
                  }
                }
                else if (pitchBend < 0) {
                  spinDial = map(constrain(pitchBend, -1023, 0), -1023, 0, 83, 88);
                   if (pitchBend == -1023){
                    spinDial--;
                  }
                }
                else spinDial = 88;
                ledRightDigit = spinDial;
                ledMiddleDigit = 64;
                ledLeftDigit = 64;
                startTimerWithPriority(1);

                pitchBendOld = pitchBend;
                pitchBend = pitchBend * 8;
                MIDI.sendPitchBend( pitchBend, midiChannel);

                OutOfRangeL = 0;
              }
            }
}

void handleVolume(){
  static byte channelVolumeOld = 127;
  if (leftSensorProcessed != 0)
          {
            channelVolume = map(leftSensorProcessed, minimumDistance, maximumDistance, dataNear, dataFar);
            if (channelVolume != channelVolumeOld)
            {
              digitSplit(channelVolume);
              startTimerWithPriority(1);
              MIDI.sendControlChange(7, channelVolume, midiChannel);
              channelVolumeOld = channelVolume;
            }
          }
}

void handleModulation(){
  static byte modulationOld = 0;
            if (leftSensorProcessed != 0 )
          {modulation = map(leftSensorProcessed, minimumDistance, maximumDistance, dataNear, dataFar);
            if (modulation != modulationOld)
            {
              digitSplit(modulation);
              startTimerWithPriority(1);      
              MIDI.sendControlChange(1, modulation, midiChannel);
              modulationOld = modulation;
            }
          }
}
void handlePortamento(){
  static byte portamentoTimeOld = 0;
  if (leftSensorProcessed !=0 )
          {          
            portamentoTime = map(leftSensorProcessed, minimumDistance, maximumDistance, dataNear, dataFar );
            if (portamentoTime != portamentoTimeOld)
            {
              digitSplit(portamentoTime);
              startTimerWithPriority(1);
              MIDI.sendControlChange(5, portamentoTime, midiChannel);
              if (portamentoTime == 0) {
                MIDI.sendControlChange (65, 0, midiChannel);
                 if (portamentoOn){
                  portamentoOn = false;
                 }
              }
                if (!portamentoOn && portamentoTime != 0){
            MIDI.sendControlChange (65, 127, midiChannel);
            portamentoOn=true;
            }
              portamentoTimeOld = portamentoTime;
            }
          }
}
void handleLeftSensor(){ 
      if (checkPriority(3)) {
      switch (functionSelectCurrent) {
    case 1:
      handlePitchBend();
      break;      
  case 2:
      handleModulation();        
      break;      
    case 3:
      handleVelocity();
      break;        
    case 4:
      handleVolume();
      break;
    case 5:
      handlePortamento();
      break;
    case 6:
      xyHandleLeftSensor();
      break;
    case 8:
      arpHandleLeftSensor();
    break;
    default:
    break;
      }
    }
}
void arpHandleLeftSensor(){
  static byte lastValue = -1;
   if (leftSensorProcessed > 0){
    unsigned long dataLeft = constrain(map(leftSensorProcessed, minimumDistance, maximumDistance, 0, 127), 0, 127);
            if (dataLeft != lastValue && checkPriority(1)){
             
              if (dataLeft > 96){
              arpSpeed = 1;
              digitSplit(3);
              tapDivisor = 3;
              }
              else if (dataLeft > 64){
              arpSpeed = 1;
              digitSplit(4);
              tapDivisor = 4;
              }
              else if (dataLeft > 32){
               arpSpeed = 2;
               digitSplit(2);
               tapDivisor = 4;
              }
              else {
               arpSpeed = 4;
               digitSplit(1);
               tapDivisor = 4;
              }
               
              startTimerWithPriority(0);
              lastValue = dataLeft;
            }
       }
}
void xyHandleLeftSensor(){
  static byte lastValue = -1;
   if (leftSensorProcessed > 0){
            byte dataLeft = map(leftSensorProcessed, minimumDistance, maximumDistance, dataNear, dataFar);
            if (dataLeft != lastValue && checkPriority(1)){
              digitSplit(dataLeft);
              startTimerWithPriority(0);
           }
               MIDI.sendControlChange(xyLeftControlChange, dataLeft, midiChannel);
              lastValue = dataLeft;
       }
}
void xyHandleRightSensor(long rightSensorProcessed){
  static byte lastValue = -1;
  if (checkPriority(3)){
     if (rightSensorProcessed > 0)
          {
            byte dataRight = map(rightSensorProcessed, minimumDistance, maximumDistance, xyDataNearRight, xyDataFarRight);
            if (dataRight != lastValue && checkPriority(1))
            {
              digitSplit(dataRight);
              startTimerWithPriority(0);
             
            }
             MIDI.sendControlChange(xyRightControlChange, dataRight, midiChannel);
              lastValue = dataRight;
            if (leftSensorProcessed > 0 && rightSensorProcessed > 0 && checkPriority(2)){
      ledLeftDigit = 95;
      ledMiddleDigit = 96;
      ledRightDigit = 97;
      startTimerWithPriority(1);
      }
    }  
  }
}
void handleRightSensor(long sensorReading){
  static bool notePlaying = false;
  static byte currentNote;
  static byte oldNote;
  static byte OutOfRange = 0;
  if (sensorReading == 0)   
  {
          lastNote = -10;

    if (OutOfRange <= 5)
    {
      OutOfRange++;
    }
    if (OutOfRange > 5 && notePlaying == true)
    {
      MIDI.sendNoteOn(oldNote, 0, midiChannel);
      OutOfRange = 0;
      notePlaying = false;     

    }
  }
  else
  {
      OutOfRange = 0;
  byte newNote = map(sensorReading, minimumDistance, maximumDistance, nearNotePointer, farNotePointer);  
    newNote = constrain(newNote, min(nearNotePointer, farNotePointer), max(nearNotePointer, farNotePointer));
    lastNote = newNote;
    currentNote = midiNotes[newNote];

    if (notePlaying == false)
    {
      notePlaying = true;
      MIDI.sendNoteOn(currentNote, noteVelocity, midiChannel);
      oldNote = currentNote;
    }
    else
    {
      if (currentNote != oldNote)
      {
        MIDI.sendNoteOn(oldNote, 0, midiChannel);
        MIDI.sendNoteOn(currentNote, noteVelocity, midiChannel);
       
        notePlaying = true;
        oldNote = currentNote;
      }
    }
  }
}
void arpHandleRightSensor(long sensorReading){
  static bool runArp = false;
  static byte currentRoot;
  static byte currentNote;
  static byte oldNote;
  static byte flagForEnd = 0;
  static byte arpPosition = 0;
  
  if (sensorReading == 0)   
  {
   flagForEnd++;
   if (arpPosition == 0){
    if (flagForEnd >= 2 || arpegio[1] == 0){
      MIDI.sendNoteOn(oldNote, 0, midiChannel);
      runArp = false;
    }
    flagForEnd = 0;
  }
  }
  else
  {
  byte newRoot = map(sensorReading, minimumDistance, maximumDistance, nearNotePointer, farNotePointer);  
    newRoot = constrain(newRoot, min(nearNotePointer, farNotePointer), max(nearNotePointer, farNotePointer));
    currentRoot = newRoot;
    runArp = true;
  }
      if (runArp){
        if (arpPosition == 0){ 
          currentNote = midiNotes[currentRoot];       
          MIDI.sendNoteOn(oldNote, 0, midiChannel);
          MIDI.sendNoteOn(currentNote, noteVelocity, midiChannel);
        } 
      if (arpPosition != 0){
        switch (arpegio[arpPosition]){
          case 0:
          arpPosition = 0;
          arpHandleRightSensor(sensorReading);
          return;
          break;
          case 1:
          MIDI.sendNoteOn(oldNote, 0, midiChannel);
          break;
          case 2:
          break;
          default:
          switch (scaleCurrent){
            case 8:
            case 9:
            currentNote = midiNotes[currentRoot + (arpegio[arpPosition] - 8)];
            break;
            case 10:
            currentNote = midiNotes[currentRoot + (arpegio[arpPosition] - 9)];
            break;
            case 11:
            currentNote = midiNotes[currentRoot + (arpegio[arpPosition] - 15)];
            break;
            default:
            currentNote = midiNotes[currentRoot + (arpegio[arpPosition] - 10)];
            break;
          }
          MIDI.sendNoteOn(oldNote, 0, midiChannel);
          MIDI.sendNoteOn(currentNote, noteVelocity, midiChannel); 
          break;
        }
      }
      oldNote = currentNote;
      arpPosition++;
      if (arpPosition > 7){arpPosition = 0;}
      }
}


long checkNoteBuffer(long reading){
  static long currentPosition;
   if (reading > map(lastNote + 1, nearNotePointer, farNotePointer, minimumDistance, maximumDistance) + noteBuffer 
   || reading < map(lastNote, nearNotePointer, farNotePointer, minimumDistance, maximumDistance) - noteBuffer) 
  {
    currentPosition = reading; 
  }
  return currentPosition;
}


void initializeArticulation(){
  for (int i = 0; i < 7; i++) 
  {
    if (readMultiplex(i) >= 10) {
      articulationMode = false;
    }
  }
  
  articulation = EEPROM.readByte(1);
  articulationTime = 15 + articulation;
  articulationTime = articulationTime * 1000;
  articulationTime = constrain(articulationTime, 15000, 270000);
  arpArticulationTime = articulationTime;
  RightSensor.setInterval(articulationTime);   
}

void displayVersion(){
	ledLeftDigit = 3 + 16;
    ledMiddleDigit = 1 + 16;
    ledRightDigit = 9;
    startTimerWithPriority(4);
}



///////////////////////////////////////////////
// VOID SETUP             //
///////////////////////////////////////////////

void setup() {

  Serial.begin(31250); // MIDI Begin

  pinMode(rightTriggerPin, OUTPUT);
  pinMode(rightEchoPin, INPUT);
  pinMode(leftTriggerPin, OUTPUT);
  pinMode(leftEchoPin, INPUT);

  pinMode(multiplexChannelSelectMSBPin, OUTPUT);
  pinMode(multiplexChannelSelectMiddleBitPin, OUTPUT);
  pinMode(multiplexChannelSelectLSBPin, OUTPUT);
  pinMode(multiplexDataPin, INPUT);

  pinMode(ledLatch, OUTPUT);
  pinMode(ledClock, OUTPUT);
  pinMode(ledData, OUTPUT);
  pinMode(ledLeftDigitPin, OUTPUT);
  pinMode(ledMiddleDigitPin, OUTPUT);
  pinMode(ledRightDigitPin, OUTPUT);

  
  pinMode(buttonPinDown, INPUT_PULLUP);
  pinMode(buttonPinUp, INPUT_PULLUP);
  pinMode(buttonPinSave, INPUT_PULLUP);
 

//Clear MIDI buffer upon startup 
  byte startupBuffer[2]= {0,0};

MIDI.sendSysEx(2,startupBuffer,false);
//MIDI.sendProgramChange(81,midiChannel); //typically a square lead, appropriate to a theremin

for (int i=0; i < sizeof(display); i++){
display[i] = display1.sort(display[i]);
}
initializeArticulation();
//rightSensorProcessed = readRightSensor();
//leftSensorProcessed = readLeftSensor();

 if (articulationMode){
displayVersion();
Display.enableDelayed(100000);
LeftSensor.enableDelayed(1000000);
RightSensor.enableDelayed(1000000);
Pots.enableDelayed(1000000);
ShiftButton.setCallback(&speedUp);
TapButton.setCallback(&slowDown);  
PresetButtonHold.setCallback(&saveArticulation);
PresetButtonTap.setCallback(&readArticulation);
Buttons.enableDelayed(1000000);
 }
 else{
wipeDisplay();
Display.enable();
StartupAnimation.enable();
 }
}

void loop()
{  
  ts.execute();
}
