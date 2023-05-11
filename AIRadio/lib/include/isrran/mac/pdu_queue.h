/**
 * Copyright 2013-2022 iSignal Research Labs Pvt Ltd.
 *
 * This file is part of isrRAN.
 *
 * isrRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * isrRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#ifndef ISRRAN_PDU_QUEUE_H
#define ISRRAN_PDU_QUEUE_H

#include "isrran/adt/circular_buffer.h"
#include "isrran/common/block_queue.h"
#include "isrran/common/buffer_pool.h"
#include "isrran/common/timers.h"
#include "isrran/mac/pdu.h"

/* Logical Channel Demultiplexing and MAC CE dissassemble */

namespace isrran {

class pdu_queue
{
public:
  typedef enum { DCH, BCH, MCH } channel_t;
  class process_callback
  {
  public:
    virtual void process_pdu(uint8_t* buff, uint32_t len, channel_t channel, int ul_nof_prbs = -1) = 0;
  };

  pdu_queue(isrlog::basic_logger& logger) : pool(DEFAULT_POOL_SIZE), callback(NULL), logger(logger) {}
  void init(process_callback* callback);

  uint8_t* request(uint32_t len);
  void     deallocate(const uint8_t* pdu);
  void     push(const uint8_t* ptr, uint32_t len, channel_t channel = DCH, int ul_nof_prbs = -1);

  bool process_pdus();

  void reset();

private:
  const static int DEFAULT_POOL_SIZE = 128;            // Number of PDU buffers in total
  const static int MAX_PDU_LEN       = 150 * 1024 / 8; // ~ 150 Mbps

  typedef struct {
    uint8_t   ptr[MAX_PDU_LEN];
    uint32_t  len;
    channel_t channel;
    int       grant_nof_prbs;
#ifdef ISRRAN_BUFFER_POOL_LOG_ENABLED
    char debug_name[128];
#endif

  } pdu_t;

  buffer_pool<pdu_t>                               pool;
  static_blocking_queue<pdu_t*, DEFAULT_POOL_SIZE> pdu_q;

  process_callback*     callback;
  isrlog::basic_logger& logger;
};

} // namespace isrran

#endif // ISRRAN_PDU_QUEUE_H
