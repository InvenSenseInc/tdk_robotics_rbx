
adb wait-for-device
adb shell tdk-chx01-fw -n 2
adb shell tdk-chx01-fw -n 5
adb shell tdk-chx01-get-data-app -d 5  -s 40 -f 5 -l /usr/chirp.csv
adb pull /usr/chirp.csv ./
