include config.mk

SRC = ${PLUGIN_NAME}.c
DESTDIR = ${PREFIX}/lib/mosquitto

all: ${PLUGIN_NAME}.so

config.h:
	cp config.def.h config.h

${PLUGIN_NAME}.so: ${SRC} config.h
	${CC} ${CFLAGS} ${SRC} -o $@

install: ${PLUGIN_NAME}.so
	mkdir -p ${DESTDIR}
	cp -f ${PLUGIN_NAME}.so ${DESTDIR}

uninstall:
	rm -f ${DESTDIR}/${PLUGIN_NAME}.so

clean:
	rm -rf ${PLUGIN_NAME}.so

.PHONY: all clean install uninstall
