idf_component_register(
	SRCS "ldr_sensor.c"
	INCLUDE_DIRS "include"
	REQUIRES
		driver
		esp_adc
)
