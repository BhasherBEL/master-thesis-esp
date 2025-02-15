all:
	platformio run -t upload -t monitor

monitor:
	platformio device monitor
