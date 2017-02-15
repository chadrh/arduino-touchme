ARDUINO_DIR   = /usr/share/arduino
ARDMK_DIR     = /usr/share/arduino
AVR_TOOLS_DIR = /usr

# ARDUINO_LIBS = Ethernet SPI
BOARD_TAG    = uno
MONITOR_PORT = /dev/ttyACM*

ARDUINO_QUIET = 1
CPPFLAGS = -std=gnu++11 -Wall -Wextra

include /usr/share/arduino/Arduino.mk
