// CylonRaider bikeLight Rainbow version
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_LSM9DS0.h>

#define PIN      6
#define N_LEDS 30

// Create neoPixel strip object
Adafruit_NeoPixel           strip = Adafruit_NeoPixel(N_LEDS, PIN, NEO_GRB + NEO_KHZ800);
// Create accelerometer object. 
Adafruit_LSM9DS0            lsm   = Adafruit_LSM9DS0(1000);  // Use I2C, ID #1000

// Initialize variables
float turn_threshold = 1;
float brake_threshold = 1;
bool leftpass = true;  // Boolean of weather to swipe left
bool rightpass = true; // Boolean of weather to swipe right
bool brake = false;    // Boolean for wheather to brake
int  target = strip.numPixels()/2; // Integer for pixel to emphasize
int  colors[6][3] = {  // Array of colors to cycle through
  {255,0,0}
  ,{255,30,0}
  ,{255,255,0}
  ,{0,255,0}
  ,{0,0,255}
  ,{75,0,130}
 // ,{238,130,238}
  };


int c = 0;               // Initial Color
int N_col = sizeof (colors) / sizeof (colors[0]);   // Number of colors
float c_sub = 0;         // Subcolor
int N_sub = 55;          // number of subcolors
int color[3] = {0,0,0};  // make the current color a global variable


// *** setup ****
void setup() {
  strip.begin();
  configureSensor();
}


// *** loop ****
void loop() {
  // If reached end of color list, start again. 
  
  // Get reatings from the accelerometer
  sensors_event_t accel, mag, gyro, temp;
  lsm.getEvent(&accel, &mag, &gyro, &temp); 

  // Choose whether to perform leftpass, rightpass, and/or brake this loop. 
  brake       = checkbrake();
  leftpass    = accel.acceleration.x < turn_threshold & !brake;
  rightpass   = accel.acceleration.x > (-1 * turn_threshold) & !brake;

  // Set the target pixel for brightness flash
  if (!leftpass){
    target = 4; // left side
  } else if (!rightpass){
    target = strip.numPixels()-4; // right side
  } else {
    target      = strip.numPixels()/2; // middle
  }

  // Set the "target" light, which gets brighter

  if (leftpass){
    raiderpass(true, target); // Red
  }

  if (rightpass){
    raiderpass(false, target); // Red
  }

  if (brake){
    brakelight();
  }

  // Cycle to next color
  //c = c+1;
  Serial.print("Accel X: "); Serial.print(accel.acceleration.x); Serial.print("\n");
  Serial.print("Accel Y: "); Serial.print(accel.acceleration.y); Serial.print("\n\n");
  
}

// ******** checkbrake *********
// Checks whether bike is braking
// uses accelerometer in they direction
static bool checkbrake(){
  sensors_event_t accel, mag, gyro, temp;
  lsm.getEvent(&accel, &mag, &gyro, &temp); 
  return accel.acceleration.y > brake_threshold;
}

// ******** nextcolor ************
// Function to get the next color in sequence
// todo: modularize the color interpolation
void nextColor(){
  // Get left color in spectrum
  int d = 0;
  // Get right color in spectrum
  if ((c+1) >= N_col){
    d = 0;
  } else {
    d = c+1;
  }

  // Linearly interpolate color between the two
  float ratio = c_sub / N_sub;
  // Interpolate to global color variable 
  for (int i=0; i<3; i++){
    color[i] = colors[c][i] * (1-ratio) + colors[d][i] * ratio;
  }

  // increment colors 
  c_sub = c_sub + 1;
  if (c_sub >= N_sub){
    c_sub = 0;
    c     = c+1;
    if (c >= N_col){
      c = 0;
    }
  }


}

// ********** raiderpass **************
// Cylon Raider Pass, forward or backward, 
// brighter in the center 4 pixles
// Right now this ignores color and produces red. 
static void raiderpass(bool forward, int target) {

  int pon;  // pixel to turn on
  int poff; // pixel to turn off
  
  for(uint16_t i=0; i<strip.numPixels() + 4; i++) {
    // * Check whether bike is braking, if so break loop
    if (checkbrake()){
      break;
    } 

    // Get a color
    nextColor();
    int r = color[0];
    int g = color[1];
    int b = color[2];
  
    // Initialize values to scale colors  
    float d = 1; // denominator of color scaling
    float m = 255; // maximum value of denominator
    if (r > 0){m = min(m,r/2);}
    if (g > 0){m = min(m,g/2);}
    if (b > 0){m = min(m,b/2);}
    
    // Define pixel on/off sequence by forward or backward pass
    if (forward){
      pon  = i;      // Draw new pixel
      poff = i - 4;  // Erase pixel a few steps back
    } else {
      pon  = strip.numPixels() - i - 1;     // Draw new pixel
      poff = strip.numPixels() - i - 1 + 4; // Erase pixel a few steps back
    }

    // set denominator
    d=min( m, sq( pon - target  ) + 1 );

    // Get color
    uint32_t c = strip.Color(r/d,g/d,b/d);
  
    // Implement passes forward or backward
    strip.setPixelColor(pon , c); // Draw new pixel
    strip.setPixelColor(poff, 0); // Erase pixel a few steps back
    strip.show();  
    delay(30);
  }
}

// ******** brakelight *********
// Brake Light!
// Pulse at full brightness
static void brakelight() {
  // Get a color
  nextColor();
  int r = color[0];
  int g = color[1];
  int b = color[2];
  
  // Set all lights to one color
  uint32_t c = strip.Color(r,g,b);
  for(uint16_t i=0; i<strip.numPixels() + 4; i++) {
    strip.setPixelColor(i , c); // Draw new pixel
  }
  strip.show(); 
  delay(150);
  for(uint16_t i=0; i<strip.numPixels() + 4; i++) {
    strip.setPixelColor(i , 0); // Draw new pixel
  }
  strip.show(); 
  delay(30);
}


// ******** configure sensor ************
void configureSensor(void)
{
  // 1.) Set the accelerometer range
  lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_2G);
  //lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_4G);
  //lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_6G);
  //lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_8G);
  //lsm.setupAccel(lsm.LSM9DS0_ACCELRANGE_16G);
  
  // 2.) Set the magnetometer sensitivity
  lsm.setupMag(lsm.LSM9DS0_MAGGAIN_2GAUSS);
  //lsm.setupMag(lsm.LSM9DS0_MAGGAIN_4GAUSS);
  //lsm.setupMag(lsm.LSM9DS0_MAGGAIN_8GAUSS);
  //lsm.setupMag(lsm.LSM9DS0_MAGGAIN_12GAUSS);

  // 3.) Setup the gyroscope
  lsm.setupGyro(lsm.LSM9DS0_GYROSCALE_245DPS);
  //lsm.setupGyro(lsm.LSM9DS0_GYROSCALE_500DPS);
  //lsm.setupGyro(lsm.LSM9DS0_GYROSCALE_2000DPS);
}
