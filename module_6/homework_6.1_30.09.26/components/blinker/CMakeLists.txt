idf_component_register(
	SRCS "blinker.cpp"
	INCLUDE_DIRS "include"
	REQUIRES
		freertos
		led
		config
)

