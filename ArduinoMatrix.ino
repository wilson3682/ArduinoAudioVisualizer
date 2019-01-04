#include <Wire.h>
#include <RGBmatrixPanel.h>
#include "Adafruit_GFX.h"
#include "arduinoFFT.h"
#include <math.h>
using namespace std;

//ports used on arduino board
#define CLK 11
#define OE   9
#define LAT 10
#define A   A0
#define B   A1
#define C   A2
#define D   A3
#define SAMPLE 64
#define S_FREQUENCY 10000

//arduino will use this to determine when to turn off
char on;
//determins the type of scene that will play on the matrix
char choice;
// Ranges for the sounds read by the microphone
int RANGE = 32;
//this is for the quieter sounds
int low = 5;
//mid sounds
int mid_lower = 10;
int mid_upper = 15;
//loudest sounds
int high = 20;
int high_mid = 25;
int higher = 30;

//value that the arduino reads from the microphone
unsigned int sample;

//initialize the color matrix
RGBmatrixPanel matrix(A, B, C, CLK, LAT, OE, false);
//initialize the EFT(will analyze frequencies)
arduinoFFT FFT = arduinoFFT();

//arrays that will hold the FFT data(frequencies)
double real[SAMPLE];
double imag[SAMPLE];
//flag to determine if the matrix is on or off
bool flag = true;

//class for the color of the matrix
class paint{
public:
  int r;
  int g;
  int b;
  void setColor(int red, int green, int blue, int topF);
  paint();
};
//constructor that sets the color to blank
paint::paint(){
  r = 0;
  b = 0;
  g = 0;
}
//function that will change the color based on the values passed
void paint::setColor(int red, int green, int blue, int topF){
  r = red;
  g = green;
  b = blue;
}

//a stack to hold the samples and frequencies

class SampleStack{
public:

    int top;
    int topF;
    SampleStack();
    //this will hold the level of each sample.
    int sampleLevel;
    //array that holds the samples. 
    int samples[30];
    //a second array to hold the averages.  
    int sampleAverage;
    //holds the calculated peak from FFT calculations
    double peak;
    double freqAverage;
    //array for frequencies
    double freq[10];
    //array of type paint to hold all the previous colors
    //pushes the latest sample onto the stack
    void push(int sampleLevel);
    //pushes frequency onto a stack
    void push(double frequency, bool flag);
    //this could either pop from the sample stack or the freq stack
    int pop();
    int pop(bool flag);
    //calculates the average of volume the samples and stores it 
    void CalcSampleAvg();
    //calculates the average of frequency the samples and stores it 
    void CalcFreqAvg();
    void DrawFreq(int i, int j, paint color);

    void Clear(int i, int j);
};
//constructor for the stack that will hold the samples
SampleStack::SampleStack(){
    top = 0;
    topF = 0;
    sampleAverage = 0;
    freqAverage = 0;
}
//pushes the sample into the sample stack
void SampleStack::push(int sampleLevel){
      //check to see if the stack is full
      if(top < 30){
        //if it is not full we can add the sample to the top of the stack
        samples[top] = sampleLevel;
        top++;
      }
      else{
      //once we reach the limit of the stack we store the average sample level over the last 30 samples
        CalcSampleAvg();
        push(sampleLevel);
      }
}

void SampleStack::push(double frequency, bool flag){
      //check to see if the stack is full
      peak = frequency;
      if(topF < 10){
        //if it is not full we can add the sample to the top of the stack
        freq[topF] = peak;
      }
      //check to see stack that holds the averages is full
      //store the frequency average over the last 10 freqeuncies recorded
      else{
        CalcFreqAvg();
        push(peak, flag);
      }
      topF++;
}
//pops and returns the next value from the stack(volumes)
int SampleStack::pop(){
    if(top > 0){
      return samples[top--];
    }
    else{
      return;
    }
}
//pops and returns the next value from the stack(frequencies)
int SampleStack::pop(bool flag){
  if(topF > 0){
    return freq[topF--];
  }
  else{
    return;
  }
}
//based off the stack, an average volume is calculated 
void SampleStack::CalcSampleAvg(){
  int total = 0;
  while(top > 0){
    total = total + pop();
  }
  top = 0;
  
  sampleAverage = total / 30;
}
//based off the stack, an average frequency is calculated 
void SampleStack::CalcFreqAvg(){
  double total = 0;
  while(topF > 0){
    total = total + pop(flag);
  }
  freqAverage =  total / 10;
}
//fills the screen based on the freqeuncy
void SampleStack::DrawFreq(int i, int j, paint color){
    matrix.drawPixel(j, i, matrix.Color888(color.r, color.g, color.b));
}
 

void setup() 
{
   Serial.begin(9600);
   matrix.begin();
} 
void loop() 
{
   //reads from visual studio to determine if the user has started the matrix or not
   on = Serial.read();
   if(on == 'o'){
   SampleStack sounds;
   paint color;
   //loops and collects data from the microphone based off of the SAMPLE size (higher size = slower performance, higher resolution)
    for(int i = 0; i < SAMPLE; i++){
        //reads data from the port we assigned to the letter D
        real[i] = analogRead(D);
        //this information is then stored in the arrays
        sample = real[i];
        //this is used to scale larger numbers down to the size of the LED Matrix
        sample = sample % RANGE;
        sounds.push(sample);
        imag[i] = real[i];
    }
  
  //FFT calculations that read the data from the microphone and calculate the frequencies present
  FFT.Windowing(real, SAMPLE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(real, imag, SAMPLE, FFT_FORWARD);
  FFT.ComplexToMagnitude(real, imag, SAMPLE);
  double peak = FFT.MajorPeak(real, SAMPLE, S_FREQUENCY);
  //FFT.DCRemoval(real, SAMPLE);
  sounds.push(peak, flag);
  Serial.println(peak);
  Serial.println(125 * sounds.sampleAverage);
  
   //loops and selects color to display

      if(peak < 625){
        color.setColor(0, 5, 95, sounds.topF);
      }
      else if(peak < 1250){
        color.setColor(0, 25, 75, sounds.topF);
      }
      else if(peak < 1875){
        color.setColor(0, 50, 50, sounds.topF);
      }
      else if(peak < 2500){
        color.setColor(0, 75, 25, sounds.topF);
      }
      else if(peak < 3125){
        color.setColor(10, 65, 25, sounds.topF);
      }
      else if(peak < 3750){
       color.setColor(45, 65, 35, sounds.topF);
      }
      else if(peak < 4375){
       color.setColor(50, 25, 20, sounds.topF);
      }
      else if(peak < 5000){
      color.setColor(50, 60, 0, sounds.topF);
      }
      else{
        color.setColor(100, 25, 0, sounds.topF);
      }
      //switch reads the user's choice from visual studio
      switch(Serial.read()){
      //each case displays something different on the screen
      case 'a':{
        //this block displays the peaks of all the frequencies
        for(int i = 0; i < 32; i++){
            matrix.drawPixel(i, fmod(real[i],16), matrix.Color333(color.b, color.r, color.g));
            //erases the old pixels after loops re runs
            matrix.drawPixel(i, fmod(real[i-1],16), matrix.Color333(0, 0, 0));
            matrix.drawPixel(i, fmod(real[i-2],16), matrix.Color333(0, 0, 0));

        }
        break;
      }
      //displays the frequencies present - Spectrum -->  needs smoothing
      case 'b':{
        double maxP = 0;
        int maxX = 0;
        matrix.fillScreen(0);
       for(int i = 0; i < 32;i++){
            
            if(real[i] > maxP){
              maxP = fmod(real[i], 16);
              maxX = i;
            }
            for(int j = 16; j > fmod(real[i], 16); j--){
            matrix.drawPixel(i, j, matrix.Color333(color.r, color.g, color.b));
            }
        }
        break;
      }
      //fills the entire screen based on the sample volume
      case 'c': {
        for(int i = 0; i < sample; i++){
          for(int j = 0; j < 32; j++){
          sounds.DrawFreq(i, j, color);
          delay(1);
          }
        }
        break;
      }
      //displays the current time on the screen
      case 'd':{
        matrix.fillScreen(0);
        matrix.setCursor(4, 0);
        matrix.setTextSize(1);
        char date[10] = {};
        for(int i = 0; i < 10; i++){
          date[i] = Serial.read();
          if(i == 5){
            matrix.setCursor(1, 9);
          }
          //color is determined by the frequency
          matrix.setTextColor(matrix.Color333(color.r, color.g, color.b));
          matrix.print(date[i]);
        }
        break;
      }
      case 'e':{
        for(int i = 0; i < sample%10; i++){
          matrix.fillScreen(0);
          matrix.fillCircle(16, 7, i, matrix.Color333(color.r, color.g, color.b));
          
        }
        for(int j = 0; j < sample%4; j++){
          matrix.fillCircle(11, 5, j, matrix.Color333(color.b, color.r, color.g));
          matrix.fillCircle(21, 5, j, matrix.Color333(color.b, color.r, color.g));
          matrix.fillCircle(11, 2, j, matrix.Color333(color.b, color.r, color.g));
          matrix.fillCircle(21, 2, j, matrix.Color333(color.b, color.r, color.g));
          matrix.fillCircle(5, 11, j, matrix.Color333(color.g, color.g, color.r));
          matrix.fillCircle(25, 11, j, matrix.Color333(color.g, color.g, color.r));
          
        }
        for(int k = 0; k < sample % 7; k++){
          matrix.fillCircle(2, 5, k, matrix.Color333(color.b, color.r, color.g));
          matrix.fillCircle(29, 5, k, matrix.Color333(color.b, color.r, color.g));
          matrix.fillCircle(2, 10, k, matrix.Color333(color.b, color.r, color.g));
          matrix.fillCircle(29, 10, k, matrix.Color333(color.b, color.r, color.g));
        }
        break;
      }
      
    }
    delay(15);
  }
   //if the user turns off the matrix all the lights are turned off
   else if(on == 'f'){
    matrix.fillScreen(0);
   }
}
