#ifndef _TEATIMER_H_
#define _TEATIMER_H_
// #include <Arduino.h>

class TeaTimer;          // main class
class Parameters;        // hold config parameters
class TemperatureSensor; // provides interface to sensor
class SerialInput;       // simulates sensor with received data
class SerialOutput;      // print output over serial
class SerialInterface;   // handles serial communication
class MQTTOutput;        // publish temperature and times via mqtt
class OledDisplay;       // show data on display
class WebServer;         // provide simple website with data and config

class DataInput;  // interface for input classes
class DataOutput; // interface for output classes

// Interfaces

/*
Data input is handled by a get-method that returns 
a temperature value. 
*/
class DataInput
{
public:
    virtual double getTemperature() = 0;
};

/*
The different data output channels use a single
method to update the displayed or transferred data.
*/
class DataOutput
{
public:
    virtual void updateDisplay(
        double &temperature,
        double &timeDrink,
        double &timeCold) = 0;
};

/*
Wrap the builtin Serial class and parse incoming
messages. Provide methods for input and output.
*/
class SerialInterface
{
public:
    bool initSuccessful{false}; // Was the initialization successful?
    /*
    Instantiate serial connection and maybe 
    check if something is connected.
    */
    SerialInterface()
    {
        Serial.begin(9600); // can not fail
        // ...               // <-- this can maybe fail?
        initSuccessful = true;
    };
};
#endif