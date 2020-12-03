Copy sns_client_example.cpp file to \qrb5165\apps_proc\src\vendor\qcom\proprietary\sensors-see\test\sns_client_example_cpp\src folder
Then build HLOS
sns_client_example_cpp executable get created
push this on the RB5 device to usr/bin
To use this tool:
Please issue below command:
sns_client_example_cpp SENSOR_NAME, TEST_LENGTH,SAMPLE_RATE,BATCH_PERIOD
Here sensor name can be accel or gyro
Sensor data and timestamp willl be stored in a file named something similar to 2018.01.28_16.03.46-motion_sensor
on RB5 



