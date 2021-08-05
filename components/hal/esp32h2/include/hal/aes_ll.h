// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <stdbool.h>
#include "soc/hwcrypto_reg.h"
#include "hal/aes_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief State of AES accelerator, busy, idle or done
 *
 */
typedef enum {
    ESP_AES_STATE_IDLE = 0, /* AES accelerator is idle */
    ESP_AES_STATE_BUSY,     /* Transform in progress */
    ESP_AES_STATE_DONE,     /* Transform completed */
} esp_aes_state_t;


/**
 * @brief Write the encryption/decryption key to hardware
 *
 * @param key Key to be written to the AES hardware
 * @param key_word_len Number of words in the key
 *
 * @return volatile number of bytes written to hardware, used for fault injection check
 */
static inline uint8_t aes_ll_write_key(const uint8_t *key, size_t key_word_len)
{
    /* This variable is used for fault injection checks, so marked volatile to avoid optimisation */
    volatile uint8_t key_in_hardware = 0;
    uint32_t *key_words = (uint32_t *)key;

    for (int i = 0; i < key_word_len; i++) {
        REG_WRITE(AES_KEY_BASE + i * 4, *(key_words + i));
        key_in_hardware += 4;
    }
    return key_in_hardware;
}

/**
 * @brief Sets the mode
 *
 * @param mode ESP_AES_ENCRYPT = 1, or ESP_AES_DECRYPT = 0
 * @param key_bytes Number of bytes in the key
 */
static inline void aes_ll_set_mode(int mode, uint8_t key_bytes)
{
    const uint32_t MODE_DECRYPT_BIT = 4;
    unsigned mode_reg_base = (mode == ESP_AES_ENCRYPT) ? 0 : MODE_DECRYPT_BIT;

    /* See TRM for the mapping between keylength and mode bit */
    REG_WRITE(AES_MODE_REG, mode_reg_base + ((key_bytes / 8) - 2));
}

/**
 * @brief Writes message block to AES hardware
 *
 * @param input Block to be written
 */
static inline void aes_ll_write_block(const void *input)
{
    const uint32_t *input_words = (const uint32_t *)input;
    uint32_t i0, i1, i2, i3;

    /* Storing i0,i1,i2,i3 in registers, not in an array
    helps a lot with optimisations at -Os level */
    i0 = input_words[0];
    REG_WRITE(AES_TEXT_IN_BASE, i0);

    i1 = input_words[1];
    REG_WRITE(AES_TEXT_IN_BASE + 4, i1);

    i2 = input_words[2];
    REG_WRITE(AES_TEXT_IN_BASE + 8, i2);

    i3 = input_words[3];
    REG_WRITE(AES_TEXT_IN_BASE + 12, i3);
}

/**
 * @brief Read the AES block
 *
 * @param output the output of the transform, length = AES_BLOCK_BYTES
 */
static inline void aes_ll_read_block(void *output)
{
    uint32_t *output_words = (uint32_t *)output;
    const size_t REG_WIDTH = sizeof(uint32_t);

    for (size_t i = 0; i < AES_BLOCK_WORDS; i++) {
        output_words[i] = REG_READ(AES_TEXT_OUT_BASE + (i * REG_WIDTH));
    }
}

/**
 * @brief Starts block transform
 *
 */
static inline void aes_ll_start_transform(void)
{
    REG_WRITE(AES_TRIGGER_REG, 1);
}


/**
 * @brief Read state of AES accelerator
 *
 * @return esp_aes_state_t
 */
static inline esp_aes_state_t aes_ll_get_state(void)
{
    return REG_READ(AES_STATE_REG);
}


/**
 * @brief Set mode of operation
 *
 * @note Only used for DMA transforms
 *
 * @param mode
 */
static inline void aes_ll_set_block_mode(esp_aes_mode_t mode)
{
    REG_WRITE(AES_BLOCK_MODE_REG, mode);
}

/**
 * @brief Set AES-CTR counter to INC32
 *
 * @note Only affects AES-CTR mode
 *
 */
static inline void aes_ll_set_inc(void)
{
    REG_WRITE(AES_INC_SEL_REG, 0);
}

/**
 * @brief Release the DMA
 *
 */
static inline void aes_ll_dma_exit(void)
{
    REG_WRITE(AES_DMA_EXIT_REG, 0);
}

/**
 * @brief Sets the number of blocks to be transformed
 *
 * @note Only used for DMA transforms
 *
 * @param num_blocks Number of blocks to transform
 */
static inline void aes_ll_set_num_blocks(size_t num_blocks)
{
    REG_WRITE(AES_BLOCK_NUM_REG, num_blocks);
}

/*
 * Write IV to hardware iv registers
 */
static inline void aes_ll_set_iv(const uint8_t *iv)
{
    uint32_t *iv_words = (uint32_t *)iv;
    uint32_t *reg_addr_buf = (uint32_t *)(AES_IV_BASE);

    for (int i = 0; i < IV_WORDS; i++ ) {
        REG_WRITE(&reg_addr_buf[i], iv_words[i]);
    }
}

/*
 * Read IV from hardware iv registers
 */
static inline void aes_ll_read_iv(uint8_t *iv)
{
    uint32_t *iv_words = (uint32_t *)iv;
    const size_t REG_WIDTH = sizeof(uint32_t);

    for (size_t i = 0; i < IV_WORDS; i++) {
        iv_words[i] = REG_READ(AES_IV_BASE + (i * REG_WIDTH));
    }
}

/**
 * @brief Enable or disable DMA mode
 *
 * @param enable true to enable, false to disable.
 */
static inline void aes_ll_dma_enable(bool enable)
{
    REG_WRITE(AES_DMA_ENABLE_REG, enable);
}

/**
 * @brief Enable or disable transform completed interrupt
 *
 * @param enable true to enable, false to disable.
 */
static inline void aes_ll_interrupt_enable(bool enable)
{
    REG_WRITE(AES_INT_ENA_REG, enable);
}

/**
 * @brief Clears the interrupt
 *
 */
static inline void aes_ll_interrupt_clear(void)
{
    REG_WRITE(AES_INT_CLR_REG, 1);
}


#ifdef __cplusplus
}
#endif