
adb wait-for-device
adb push tdk-chx01-fw.c /usr/
adb shell "gcc /usr/tdk-chx01-fw.c -o /usr/bin/tdk-chx01-fw"

