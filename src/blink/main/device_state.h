typedef enum {
    STATE_MEASURE,
    STATE_UPLOAD,
    STATE_DISPLAY,
    STATE_IDLE,     // later: replaced by STATE_SLEEP
} device_state_t;

static device_state_t device_state = STATE_MEASURE;
