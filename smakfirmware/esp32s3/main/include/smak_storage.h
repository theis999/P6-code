#ifndef SMAK_INCLUDED_SMAK_STORAGE_H_
#define SMAK_INCLUDED_SMAK_STORAGE_H_

#include <stdint.h>

/**
 * @brief Initialize storage for SMAK related things
 * @return int
 * @retval 0 on success
 * @retval -1 if NVS couldn't be opened
 */
[[nodiscard("Return value MUST be checked for errors")]]
int smak_storage_init(void);

/**
 * @brief Set the current game ID in NVS storage
 *
 * @param id The ID to set
 * @return int
 * @retval 0 on success
 * @retval -1 if NVS couldn't be opened
 * @retval -2 if the ID couldn't be stored in NVS
 */
[[nodiscard("Return value MUST be checked for errors")]]
int smak_gameid_store(uint64_t id);

uint64_t smak_gameid_current_get(void);

#endif // SMAK_INCLUDED_SMAK_STORAGE_H_