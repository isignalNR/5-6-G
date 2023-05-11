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

#ifndef ISRRAN_INTERVAL_H
#define ISRRAN_INTERVAL_H

#include "isrran/isrlog/bundled/fmt/format.h"
#include "isrran/support/isrran_assert.h"
#include <cassert>
#include <string>
#include <type_traits>

namespace isrran {

/// Representation of an interval between two numeric-types in the math representation [start, stop)
template <typename T>
class interval
{
  // TODO: older compilers may not have defined this C++11 trait.
#if (defined(__clang__) && (__clang_major__ >= 5)) || (defined(__GNUG__) && (__GNUC__ >= 5))
  static_assert(std::is_trivially_copyable<T>::value, "Expected to be trivially copyable");
#endif

public:
  interval() : start_(T{}), stop_(T{}) {}
  interval(T start_point, T stop_point) : start_(start_point), stop_(stop_point) { assert(start_ <= stop_); }

  T start() const { return start_; }
  T stop() const { return stop_; }

  bool empty() const { return stop_ == start_; }

  auto length() const -> decltype(std::declval<T>() - std::declval<T>()) { return stop_ - start_; }

  void set(T start_point, T stop_point)
  {
    isrran_assert(stop_point >= start_point, "interval::set called for invalid range points");
    start_ = start_point;
    stop_  = stop_point;
  }

  void resize_by(T len)
  {
    // Detect length overflows
    isrran_assert(std::is_unsigned<T>::value or (len >= 0 or length() >= -len), "Resulting interval would be invalid");
    stop_ += len;
  }

  void resize_to(T len)
  {
    isrran_assert(std::is_unsigned<T>::value or len >= 0, "Interval width must be positive");
    stop_ = start_ + len;
  }

  void displace_by(int offset)
  {
    start_ += offset;
    stop_ += offset;
  }

  void displace_to(T start_point)
  {
    stop_  = start_point + length();
    start_ = start_point;
  }

  bool overlaps(interval other) const { return start_ < other.stop_ and other.start_ < stop_; }

  bool contains(T point) const { return start_ <= point and point < stop_; }

  interval<T>& intersect(const interval<T>& other)
  {
    if (not overlaps(other)) {
      *this = interval<T>{};
    } else {
      start_ = std::max(start(), other.start());
      stop_  = std::min(stop(), other.stop());
    }
    return *this;
  }

private:
  T start_;
  T stop_;
};

template <typename T>
bool operator==(const interval<T>& lhs, const interval<T>& rhs)
{
  return lhs.start() == rhs.start() and lhs.stop() == rhs.stop();
}

template <typename T>
bool operator!=(const interval<T>& lhs, const interval<T>& rhs)
{
  return not(lhs == rhs);
}

template <typename T>
bool operator<(const interval<T>& lhs, const interval<T>& rhs)
{
  return lhs.start() < rhs.start() or (lhs.start() == rhs.start() and lhs.stop() < rhs.stop());
}

//! Union of intervals
template <typename T>
interval<T> operator|(const interval<T>& lhs, const interval<T>& rhs)
{
  if (not lhs.overlaps(rhs)) {
    return interval<T>{};
  }
  return {std::min(lhs.start(), rhs.start()), std::max(lhs.stop(), rhs.stop())};
}

template <typename T>
interval<T> make_union(const interval<T>& lhs, const interval<T>& rhs)
{
  return lhs | rhs;
}

//! Intersection of intervals
template <typename T>
interval<T> operator&(const interval<T>& lhs, const interval<T>& rhs)
{
  if (not lhs.overlaps(rhs)) {
    return interval<T>{};
  }
  return interval<T>{std::max(lhs.start(), rhs.start()), std::min(lhs.stop(), rhs.stop())};
}

template <typename T>
interval<T> make_intersection(const interval<T>& lhs, const interval<T>& rhs)
{
  return lhs & rhs;
}

} // namespace isrran

namespace fmt {

template <typename T>
struct formatter<isrran::interval<T> > : public formatter<T> {
  template <typename FormatContext>
  auto format(const isrran::interval<T>& interv, FormatContext& ctx) -> decltype(std::declval<FormatContext>().out())
  {
    return format_to(ctx.out(), "[{}, {})", interv.start(), interv.stop());
  }
};

} // namespace fmt

#endif // ISRRAN_INTERVAL_H
