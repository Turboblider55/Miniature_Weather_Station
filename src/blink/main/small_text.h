#include <ssd1306.h>

#define FONT_FIRST_CHAR 32
#define FONT_CHAR_COUNT 96

void display_small_char(ssd1306_handle_t handle, char c, int x, int y);
void display_small_text(ssd1306_handle_t handle, const char *text, int x, int y);
