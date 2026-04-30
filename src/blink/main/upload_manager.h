#ifndef UPLOAD_MANAGER_H
#define UPLOAD_MANAGER_H

#include <stdbool.h>

/**
 * @brief Try to upload exactly one batch of measurements.
 *
 * Conditions checked internally:
 *  - WiFi must be connected
 *  - Time must be valid
 *  - At least BATCH_SIZE measurements stored
 *
 * @return true  Batch uploaded successfully and deleted
 * @return false Upload not attempted or failed (data kept)
 */
bool upload_manager_try_upload_one_batch(void);

#endif // UPLOAD_MANAGER_H