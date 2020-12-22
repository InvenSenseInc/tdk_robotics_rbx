inherit uimage extrausers

#Install packages for tdk applications
CORE_IMAGE_BASE_INSTALL_append = " \
            tdk-thermistor-app \
            tdk-chx01-get-data-app \
            tdk-hvc4223f-scripts \
            "
