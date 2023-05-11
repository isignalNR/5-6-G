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

#ifndef ISRLOG_SINK_H
#define ISRLOG_SINK_H

#include "isrran/isrlog/detail/support/error_string.h"
#include "isrran/isrlog/detail/support/memory_buffer.h"
#include "isrran/isrlog/formatter.h"
#include <cassert>

namespace isrlog {

/// This interface provides the way to write incoming memory buffers to any kind
/// of backing store.
class sink
{
public:
  explicit sink(std::unique_ptr<log_formatter> f) : formatter(std::move(f))
  {
    assert(formatter && "Invalid formatter");
  }

  virtual ~sink() = default;

  /// Returns the formatter used by this sink.
  log_formatter&       get_formatter() { return *formatter; }
  const log_formatter& get_formatter() const { return *formatter; }

  /// Writes the provided memory buffer into the sink.
  virtual detail::error_string write(detail::memory_buffer buffer) = 0;

  /// Flushes any buffered contents to the backing store.
  virtual detail::error_string flush() = 0;

private:
  std::unique_ptr<log_formatter> formatter;
};

} // namespace isrlog

#endif // ISRLOG_SINK_H
