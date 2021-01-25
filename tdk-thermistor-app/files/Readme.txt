Introduction:
	The "tdk_thermistor_app" enables iio triggered buffers for TDK Thermistor and reads the sensor values.


Prerequisites:
1. The application automatically checks for thermistor device in /sys/bus/iio/devices/

2. Make sure the trigger is created; trigger will be created in same folder as devices "/sys/bus/iio/devices/" . Trigger number is assigned based on iio device number.
   For thermistor we are using "tdk_thermistor-hrtimer3" as trigger. In below example the trigger number is 0:
   sh-4.4# ls /sys/bus/iio/devices 
           iio:device0  iio:device1  iio:device2  iio:device3  trigger0
   sh-4.4# cat /sys/bus/iio/devices/trigger0/name     
           tdk_thermistor-hrtimer3


Running the application:
To run tdk_thermistor_app iio device number and enable/disble should be provided. 
1. To enable : ./tdk-thermistor-app -e 1          --> Example:./tdk-thermistor-app -e 1
2. To disable: ./tdk-thermistor-app -e 0          --> Example:./tdk-thermistor-app -e 0

The app will display timestamp and temperature value in celcius. Same will logged in csv file on RB5.

