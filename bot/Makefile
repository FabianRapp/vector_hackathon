a = a


.PHONOY: all run upload compile_commands.json monitor

all: upload

run:
	pio run

upload:
	pio run -t upload

compile_commands.json:
	pio run -t compiledb


monitor:
	pio device monitor -b 115200 -p /dev/ttyACM0
