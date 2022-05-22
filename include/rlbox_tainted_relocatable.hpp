/**
 * @file rlbox_tainted_relocatable.hpp
 * @copyright Copyright (c) 2022 UCSD PLSysSec. This project is released under
 * the MIT License. You can obtain a copy of the License at
 * https://raw.githubusercontent.com/PLSysSec/rlbox/master/LICENSE
 * @brief This header implements the tainted_relocatable class.
 */

#pragma once

#include <stdlib.h>
// IWYU incorrectly reports this as unnecessary as the use of type_traits is in
// a templated class
#include <type_traits>  // IWYU pragma: keep

#include "rlbox_helpers.hpp"
#include "rlbox_tainted_base.hpp"
#include "rlbox_type_conversion.hpp"
#include "rlbox_type_traits.hpp"
#include "rlbox_wrapper_traits.hpp"

namespace rlbox {
/**
 * @brief Implementation of tainted data wrappers that supports the movement of
 * the sandbox heap after creation.
 *
 * @note This implementatation handles only non-class/non-struct and non-pointer
 * T types and its behavior is identical to tainted_fixed_aligned.
 * Specializations of this class are generated by the rlbox_lib_load_classes
 * when T is a class/struct.
 *
 * @tparam T is the type of the data being wrapped.
 * @tparam TSbx is the type of the sandbox plugin that represents the underlying
 * sandbox implementation.
 */
template <typename T, typename TSbx, typename TEnable = void>
class tainted_relocatable;

/**
 * @brief Specialization for fundamental types
 * @tparam T is the type of the data being wrapped.
 * @tparam TSbx is the type of the sandbox plugin that represents the underlying
 * sandbox implementation.
 */
template <typename T, typename TSbx>
class tainted_relocatable<T, TSbx,
                          RLBOX_SPECIALIZE(detail::is_fundamental_or_enum_v<T>)>
    : public tainted_base<T, TSbx> {
  KEEP_RLBOX_CLASSES_FRIENDLY;

 private:
  using TSbxRep = detail::rlbox_base_types_convertor<T, TSbx>;

  detail::value_type_t<T> data;

  /**
   * @brief Unsafely remove the tainting and get the raw data. For internal use
   * only.
   * @return detail::value_type_t<T> is the raw data
   */
  [[nodiscard]] inline detail::value_type_t<T> raw_host_rep() const {
    return data;
  }

  /**
   * @brief Unsafely remove the tainting and get the raw data converted to the
   * sandboxed ABI. For internal use only.
   * @return detail::value_type_t<TSbxRep> is the raw data in the sandboxed ABI
   */
  [[nodiscard]] inline detail::value_type_t<TSbxRep> raw_sandboxed_rep() const {
    auto converted =
        detail::convert_type_fundamental<detail::value_type_t<TSbxRep>>(data);
    return converted;
  }

 public:
  /**
   * @brief Unsafely remove the tainting and get the raw data.
   * @return detail::value_type_t<T> is the raw data
   */
  [[nodiscard]] inline detail::value_type_t<T> UNSAFE_unverified() const {
    return raw_host_rep();
  }

  /**
   * @brief Unsafely remove the tainting and get the raw data converted to the
   * sandboxed ABI.
   * @return detail::value_type_t<TSbxRep> is the raw data in the sandboxed ABI
   */
  [[nodiscard]] inline detail::value_type_t<TSbxRep> UNSAFE_sandboxed() const {
    return raw_sandboxed_rep();
  }

  ////////////////////////////////

  inline tainted_relocatable() = default;

  ////////////////////////////////

  /**
   * @brief Construct a new tainted fixed aligned object from another tainted
   * wrapper
   * @tparam TWrap is the other wrapper type
   * @tparam TOther is the type of the rhs value being wrapped
   * @tparam RLBOX_REQUIRE param checks to see if this is (1) a tainted wrapper,
   * (2) meets the assignable criterion
   * @param aOther is the rhs being assigned
   */
  template <template <typename, typename...> typename TWrap, typename TOther,
            RLBOX_REQUIRE(detail::is_tainted_wrapper<TWrap, TOther, TSbx>&&
                              std::is_assignable_v<decltype(data)&, TOther>)>
  inline tainted_relocatable(const TWrap<TOther, TSbx>& aOther)
      : data(aOther.raw_host_rep()) {}

  /**
   * @brief Construct a new tainted fixed aligned object from a raw primitive
   * value
   * @param aOther is the raw primitive
   */
  inline tainted_relocatable(const T& aOther) : data(aOther) {}
};

/**
 * @brief Specialization for non-fundamental types
 * @tparam T is the type of the data being wrapped.
 * @tparam TSbx is the type of the sandbox plugin that represents the underlying
 * sandbox implementation.
 */
template <typename T, typename TSbx>
class tainted_relocatable<
    T, TSbx, RLBOX_SPECIALIZE(!detail::is_fundamental_or_enum_v<T>)>
    : public tainted_base<T, TSbx> {
  KEEP_RLBOX_CLASSES_FRIENDLY;

 private:
  /**
   * @brief Unsafely remove the tainting and get the raw data. For internal use
   * only.
   * @return detail::value_type_t<T> is the raw data
   */
  [[nodiscard]] inline detail::value_type_t<T> raw_host_rep() const { abort(); }
  // [[nodiscard]]
  // inline detail::value_type_t<TSbxRep> raw_sandboxed_rep() const { abort(); }

 public:
  /**
   * @brief Unsafely remove the tainting and get the raw data.
   * @return detail::value_type_t<T> is the raw data
   */
  [[nodiscard]] inline detail::value_type_t<T> UNSAFE_unverified() const {
    return raw_host_rep();
  }

  // [[nodiscard]]
  // inline detail::value_type_t<TSbxRep> UNSAFE_sandboxed() const {
  //   return raw_sandboxed_rep();
  // }
};

}  // namespace rlbox
