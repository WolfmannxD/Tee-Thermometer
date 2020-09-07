# Tee-Thermometer

This is a small arduino project which helps to track the temperature of a hot drink. It is based on an Arduino Nano and a DS1020 digital temperature sensor. This sensor can be connected directly to the Arduino without any additional circuitry. As output, I use a 128x64 OLED Display with I2C interface. 
The temperature is logged and used to extrapolate the time when the desired temperature is reached. This is accomplished by doing a linear regression on the logarithm of the temperatures, to simplify the computation. For more information on the calculations see Tee-Thermometer.pdf. 

The main function of this device is the calculation and display of the remaining time until the tea (or other hot drink) has cooled enough to be drinkable. It also shows the remaining time until the drink is too cold. The temperature values may vary from person to person and must be changed in the sketch directly. 