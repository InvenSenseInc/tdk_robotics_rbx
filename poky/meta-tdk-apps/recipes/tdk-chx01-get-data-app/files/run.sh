
adb wait-for-device
adb shell tdk-ch101-fw 3
adb shell tdk-ch101-fw 6
adb shell tdk-chx01-get-data -d 3 -s 40 -f 10 -l /usr/chirp.csv
adb pull /usr/chirp.csv ./
