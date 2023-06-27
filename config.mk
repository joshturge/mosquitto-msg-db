VERSION = 0.0.1

PLUGIN_NAME = mosquitto_msg_db

# path
PREFIX = /usr/local

INCS = -I/usr/local/include

CFLAGS = ${INCS} -std=c11 -pedantic -Wall -Wno-deprecated-declarations -Os -fPIC -shared

CC = cc
