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

#ifndef ISRLOG_ISRLOG_INSTANCE_H
#define ISRLOG_ISRLOG_INSTANCE_H

#include "formatters/text_formatter.h"
#include "log_backend_impl.h"
#include "object_repository.h"
#include "sinks/stream_sink.h"
#include "isrran/isrlog/log_channel.h"

namespace isrlog {

/// Singleton of the framework containing all the required classes.
class isrlog_instance
{
  isrlog_instance()
  {
    // stdout and stderr sinks are always present.
    auto& stdout_sink =
        sink_repo.emplace(std::piecewise_construct,
                          std::forward_as_tuple("stdout"),
                          std::forward_as_tuple(new stream_sink(sink_stream_type::stdout,
                                                                std::unique_ptr<log_formatter>(new text_formatter))));
    default_sink = stdout_sink.get();

    sink_repo.emplace(std::piecewise_construct,
                      std::forward_as_tuple("stderr"),
                      std::forward_as_tuple(new stream_sink(sink_stream_type::stderr,
                                                            std::unique_ptr<log_formatter>(new text_formatter))));

    // Initialize the default formatter pointer with a text formatter.
    {
      detail::scoped_lock lock(formatter_mutex);
      default_formatter = std::unique_ptr<log_formatter>(new text_formatter);
    }
  }

public:
  isrlog_instance(const isrlog_instance& other) = delete;
  isrlog_instance& operator=(const isrlog_instance& other) = delete;

  /// Access function to the singleton instance.
  static isrlog_instance& get()
  {
    static isrlog_instance instance;
    return instance;
  }

  /// Logger repository accessor.
  using logger_repo_type = object_repository<std::string, detail::any>;
  logger_repo_type&       get_logger_repo() { return logger_repo; }
  const logger_repo_type& get_logger_repo() const { return logger_repo; }

  /// Log channel repository accessor.
  using channel_repo_type = object_repository<std::string, log_channel>;
  channel_repo_type&       get_channel_repo() { return channel_repo; }
  const channel_repo_type& get_channel_repo() const { return channel_repo; }

  /// Sink repository accessor.
  using sink_repo_type = object_repository<std::string, std::unique_ptr<sink> >;
  sink_repo_type&       get_sink_repo() { return sink_repo; }
  const sink_repo_type& get_sink_repo() const { return sink_repo; }

  /// Backend accessor.
  detail::log_backend&       get_backend() { return backend; }
  const detail::log_backend& get_backend() const { return backend; }

  /// Installs the specified error handler into the backend.
  void set_error_handler(error_handler callback) { backend.set_error_handler(std::move(callback)); }

  /// Set the specified sink as the default one.
  void set_default_sink(sink& s) { default_sink = &s; }

  /// Returns the default sink.
  sink& get_default_sink() const { return *default_sink; }

  /// Set the specified formatter as the default one.
  void set_default_formatter(std::unique_ptr<log_formatter> f)
  {
    detail::scoped_lock lock(formatter_mutex);
    default_formatter = std::move(f);
  }

  /// Returns the default formatter.
  std::unique_ptr<log_formatter> get_default_formatter() const
  {
    detail::scoped_lock lock(formatter_mutex);
    return default_formatter->clone();
  }

private:
  /// NOTE: The order of declaration of each member is important here for proper
  /// destruction.
  sink_repo_type                 sink_repo;
  log_backend_impl               backend;
  channel_repo_type              channel_repo;
  logger_repo_type               logger_repo;
  detail::shared_variable<sink*> default_sink{nullptr};
  mutable detail::mutex          formatter_mutex;
  std::unique_ptr<log_formatter> default_formatter;
};

} // namespace isrlog

#endif // ISRLOG_ISRLOG_INSTANCE_H
