ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = tdk-thermistor-app

AM_CPPFLAGS = -I./                \
-I../recipe-sysroot/usr/include

tdk_thermistor_app_SOURCES := \
    tdk-thermistor-app.c

tdk_thermistor_app_CPPFLAGS = -Wno-all
tdk_thermistor_app_CPPFLAGS += -Wno-error
tdk_thermistor_app_CPPFLAGS += -Wno-unused-parameter
tdk_thermistor_app_CPPFLAGS += -DARM_BASED
tdk_thermistor_app_CPPFLAGS += -I.
tdk_thermistor_app_CPPFLAGS += -lm
