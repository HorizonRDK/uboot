/*
 *   Copyright 2020 Horizon Robotics, Inc.
 */
#ifndef BOARD_HOBOT_XJ3_XJ3_SET_PIN_H_
#define BOARD_HOBOT_XJ3_XJ3_SET_PIN_H_
enum pull_state {
	NO_PULL = 0,
	PULL_UP,
	PULL_DOWN,
};

enum pin_dir {
	IN = 0,
	OUT,
};

enum xj3_pin_type {
	PIN_INVALID = 0,
	PULL_TYPE1,
	PULL_TYPE2,
};
/* to set xj3 pin */
struct pin_info {
	int pin_index;
	enum pull_state state;
};
#endif // BOARD_HOBOT_XJ3_XJ3_SET_PIN_H_
