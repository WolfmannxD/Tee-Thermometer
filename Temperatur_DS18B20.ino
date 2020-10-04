#include <OneWire.h> 
#include <DallasTemperature.h>
#include <math.h>
#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"

#define I2C_ADDRESS 0x3C
#define RST_PIN -1

SSD1306AsciiWire oled;

#define ONE_WIRE_BUS 2 // Data wire pin
OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to Dallas Temperature. 

#define ARRAYSIZE 80 // 150
#define WAITTIME 5000 // Waiting time between computations (milliseconds)
#define ambient_optim_interval 60000 // time interval for ambient_search

double temperatures[ARRAYSIZE]; // array for the temperatures
double times[ARRAYSIZE]; // array for the times
double y[ARRAYSIZE];
// double sq_res[ARRAYSIZE]; // squared residuals
double newtemp;
double newtime;
double lasttime = 0.0;
int index = 0; // current index in arrays
int counter = 0; // Number of total values
double ambient_temp; // ambient temperature
double trink_temp;
double cold_temp;
double fitparams[2];  // fit parameters A, k
double time_trink = 0.0;
double time_cold = 0.0;
double time_trink1 = 0.0;
double time_cold1 = 0.0;
int print_time[2]; // minutes, seconds
long optimize_ambient_time = 0; // time of last ambient_search


void setup() {
  Serial.begin(9600); 
  sensors.begin(); 
  sensors.setResolution(12); // Can be 9, 10, 11, 12
  //Serial.print("Resolution: ");
  //Serial.println(sensors.getResolution()); 
  ambient_temp = 22.0; //28.4; // limit of temperature used for fit
  trink_temp = 60.0; // target trinking temperature
  cold_temp = 40.0; // tearget cold temperature

  // Setup display
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(Callibri15);
  oled.clear();
  // Überschrift: "Tee Temperatur Timer"
  oled.setCursor(0, 0);
  oled.println(F("Tee Temperatur Timer"));
  // Aktuelle Temperatur
  oled.print(F("Temperatur: "));
  oled.print(newtemp, 1);
  oled.setCursor(110, 8);
  oled.println(F(" C"));
  // Initializing...
  oled.setCursor(0, 48);
  oled.print(F("Aufwaermphase"));

  // initialize
  initialize();
  oled.clear();
}

void loop() {
  // get new sensor values
  sensors.requestTemperatures();
  newtemp = sensors.getTempCByIndex(0);
  newtime = millis()/1000.0; // seconds
  if ((index > 5) &((temperatures[index-1] - newtemp) > 2)){ 
    // Temperature difference of 2 degree or more
    // This filters outliers
    // Outliers can occur while drinking or moving the 
    // cup.
    Serial.println(F("rejecting new temp"));
    return; // Probably no good value
  }
  if ((newtime - times[index-1]) > 3){ // Sekunden 
    Serial.println(F("Adding new values."));
    addvalue(temperatures, index, newtemp);
    addvalue(times, index, newtime);
    if (index > 3) {
      // fit routine here
      linfit(times, temperatures);
      if ((millis()-optimize_ambient_time) > ambient_optim_interval){
        //Serial.print("optimize_ambient_time= ");
        //Serial.println(optimize_ambient_time);
        //Serial.print("millis: ");
        //Serial.println(millis());
        search_ambient_temp();
      }
    }  
    if (index < ARRAYSIZE-1) { // move index forward if not at end already
      index += 1;
    }
    counter += 1;
  }
  Serial.print(F("Temperature is: ")); 
  Serial.print(newtemp);
  Serial.println(F("°C")); 

  // ------------------
  // Send all data points to Serial for debugging:
//  for (int i = 0; i < ARRAYSIZE; i++){
//    Serial.print(temperatures[i]);
//    Serial.print(',');
//  }
//  for (int i = 0; i < ARRAYSIZE; i++){
//    Serial.print(times[i]);
//    Serial.print(',');
//  }
//  Serial.print(fitparams[0], 5);
//  Serial.print(',');
//  Serial.print(fitparams[1], 5);
//  Serial.print(',');
//  Serial.print(time_trink1);
//  Serial.print(',');
//  Serial.println(time_cold1);
  // ------------------
//  if ((millis() > WAITTIME) & ((millis() - lasttime) > WAITTIME)){
//    lasttime = millis();
//    if (index > 3) {
//      // fit routine here
//      linfit(times, temperatures);
//    }  
//  }

  time_trink = estimate_time(fitparams, trink_temp);
  time_cold = estimate_time(fitparams, cold_temp);

  // Überschrift: "Tee Temperatur Timer"
  oled.setCursor(0, 0);
  oled.println(F("Tee Temperatur Timer"));
  // Aktuelle Temperatur
  oled.print(F("Temperatur: "));
  oled.print(newtemp, 1);
  oled.setCursor(110, 8);
  oled.println(F(" C"));
  // Trinkbar in: 
  oled.print(F("Trinkbar in:  "));
  oled.setCursor(70, 16);
  print_time_format(time_trink);
  oled.setCursor(110, 16);
  oled.println(F("min"));
  // Kalt in: 
  oled.print(F("Kalt in:  "));
  oled.setCursor(70, 24);
  print_time_format(time_cold);
  oled.setCursor(110, 24);
  oled.println(F("min"));
  
  //delay(1000);
}

void initialize() {
  // Wait for temperature rise of the sensor
  // before actual logging
  Serial.println(F("Beginn der Aufwärmphase"));
  double oldtemp = 0.0;
  double newtemp = 0.0;
  sensors.requestTemperatures(); 
  newtemp = sensors.getTempCByIndex(0); // New measurement
  delay(1000);
  while (true) { // keep loop running until break
    oldtemp = newtemp; // copy last temperature value
    sensors.requestTemperatures(); 
    newtemp = sensors.getTempCByIndex(0); // New measurement
    oled.setCursor(0, 0);
    oled.println(F("Tee Temperatur Timer"));
    // Aktuelle Temperatur
    oled.print(F("Temperatur: "));
    oled.print(newtemp, 1);
    oled.setCursor(110, 8);
    oled.print(F(" C"));
    if ((newtemp < oldtemp) & (millis() >5000)) {
      // Temperature starts falling
      break; // break loop and continue program
    }
    delay(1000);
  }
  Serial.println(F("Ende der Aufwärmphase"));
}


void print_time_format(double t){
  // Convert remaining time to printable format
  // mm:ss
  int minutes = t / 60; // convert to minutes and seconds
  int seconds = (t-minutes*60);

  if (minutes < 0) minutes = 0; // no negative wait times
  if (seconds < 0) seconds = 0;

  if (minutes < 10) oled.print(0); // print leading zeros
  oled.print(minutes);
  oled.print(":");
  if (seconds < 10) oled.print(0); // print leading zeros
  oled.print(seconds);
  oled.print(F("  "));
}

void addvalue(double list[], int index, double new_value){
  // Add a new value to list of temperatures or times
  if (counter <= ARRAYSIZE-1) { // array not full
    list[index] = new_value;
  }
  else if (counter >= ARRAYSIZE-1){ // array is full
    for (int i = 0; i <= ARRAYSIZE-2; i++){ // move list one index back
      list[i] = list[i+1];
    }
    list[index] = new_value; // last element
  }
}

void linfit(double times[], double temperatures[]){
  // fit a linear curve to log of temperatures. 
  // Ambient temperature is subtracted to make 
  // the problem solvable.
  //double y[ARRAYSIZE]; DUMMY VARIABLE, CHANGES MEANING
  for (int i = 0; i <= index; i++){ // logarithm of temperature
    y[i] = log(temperatures[i] - ambient_temp);
  }
  double x_ = mean(times);
  double y_ = mean(y);
  // linear regression:
  for (int i = 0; i <= index; i++){
    y[i] *= (times[i] - x_); // residuals * y
  }
  double sumy = sum(y); // Sum of residuals * y
  for (int i = 0; i <= index; i++){
    y[i] = pow((times[i] - x_), 2); // squared residuals
  }
  double sumsqres = sum(y); // Sum of squared residuals (t_i- y_)**2
  double b = sumy / sumsqres; // y = residuals * y
  double a = y_ - b*x_;

  // convert to exponential function
  fitparams[0] = exp(a); // A
  fitparams[1] = -b;     // k
  //Serial.print("Fitparams[0] = ");
  //Serial.println(fitparams[0]);
  //Serial.print("Fitparams[1]");
  //Serial.println(fitparams[1]);
}

int estimate_time(double fitparams[], double target_temp){
  // Calculate the remaining time until target_temperature is reached
  // based on the parameters a, k, and ambient_temp.
  //double a = fitparams[0];
  //double k = fitparams[1];
  double time_target = log(fitparams[0]/(target_temp - ambient_temp))/fitparams[1];
  double wait_time = (time_target - times[index]); // in Sekunden
  int round_wait_time;
  round_wait_time = round(wait_time);

  return round_wait_time;
}

double fit_residuals(){
  // Calculate the squared residuals between the fit and 
  // the temperature data.
  double squared_residuals = 0.0;
  double fit_y = 0.0;
  for (int i = 0; i<=index; i++){
    fit_y = fitparams[0]*exp(-times[i]*fitparams[1]) + ambient_temp;
    squared_residuals += pow((fit_y - temperatures[i]), 2);
  }
  return squared_residuals;
}

void search_ambient_temp(){
  // Find optimal value for ambient temperature by calculating
  // the fit for different values of T_ambient. The value with
  // the smallest squared residuals is then chosen.
  Serial.println(F("Searching new optimal ambient_temp..."));
  double residuals = fit_residuals(); // fit residuals for current value of ambient_temp
  double old_ambient_temp = ambient_temp; // copy current value of 
  double new_ambient_temp;
  for (int i=-5; i <=5; i++){ // iterate over some variations of ambient_temp
    //Serial.println(i);
    ambient_temp = old_ambient_temp + i*0.5; // change ambient temperature parameter
    linfit(times, temperatures); // compute new fit
    double new_residuals = fit_residuals(); // compute residuals
    //Serial.print("residuals: ");
    //Serial.println(new_residuals);
    if (new_residuals < residuals){ // check if new value is better
      new_ambient_temp = ambient_temp; // store new value of ambient_temp
      Serial.print(F("found new value for ambient_temp: "));
      Serial.println(ambient_temp);
    }
  }
  ambient_temp = new_ambient_temp; // update value of ambient_time
  linfit(times, temperatures); // compute final fit with regular values
  optimize_ambient_time = millis(); // update time of last function call
}


double mean(double x[]){
  return sum(x)/(index+1);
}

double sum(double x[]){
  // summiere bis index
  double summe = 0;
  //double xx;
  for (int i = 0; i <= index; i++){
    summe += x[i];
  }
  return summe;
}
