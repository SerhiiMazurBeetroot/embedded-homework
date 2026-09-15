#pragma once

// limit value in the range [val_min, val_max]
static inline int range_clamp(int value, int val_min, int val_max) {
	if(value < val_min) return val_min;
	if(value > val_max) return val_max;
	return value;
}

// linear range transform
static inline int range_map(int value, int in_min, int in_max, int out_min, int out_max) {
	if(in_min == in_max) return out_max;
	value = range_clamp(value, in_min, in_max);
	return out_min + (value -in_min) * (out_max - out_min) / (in_max - in_min);
}
