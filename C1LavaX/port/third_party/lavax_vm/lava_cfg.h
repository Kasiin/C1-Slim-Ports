#pragma once

#define LAVA_MAX_WIDTH      320
#define LAVA_MAX_HEIGHT     240

// Maximum 24-bit 16MiB
#define LAVA_MAX_RAM_SIZE   0x01000000

#define LAVA_STACK_OFFSET   0x1b00
#define LAVA_STRING_OFFSET  0x1c00
#define LAVA_STRING_LIMIT   768

// When set to 0, screen refresh is only requested
// when waiting for user input or delay
#define LAVA_REFRESH_AT_FLUSH 0
