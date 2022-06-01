/**
 * @file rlbox_tainted_volatile_standard.hpp
 * @copyright Copyright (c) 2022 UCSD PLSysSec. This project is released under
 * the MIT License. You can obtain a copy of the License at
 * https://raw.githubusercontent.com/PLSysSec/rlbox/master/LICENSE
 * @brief This header implements the tainted_volatile_standard class.
 */

#pragma once

#include <type_traits>

#include "rlbox_abi_conversion.hpp"
#include "rlbox_helpers.hpp"
#include "rlbox_sandbox.hpp"
#include "rlbox_tainted_base.hpp"
#include "rlbox_type_conversion.hpp"
#include "rlbox_type_traits.hpp"

namespace rlbox {
/**
 * @brief Implementation of tainted_volatile data wrapper.
 * tainted_volatile_standard indicates that this data is memory that can be
 * modified by the sandbox.
 *
 * @details tainted_volatile_standard data in addition to being untrusted (since
 * it is memory written to be sandboxed code), should be carefully checked for
 * double-read or time-of-check-time-of-use style bugs, i.e., if this data is
 * read twice, the data could have been changed asynchronously by a concurrent
 * sandbox thread. To safely handle tainted_volatile_standard data, the host
 * application should make a copy of tainted_volatile_standard data into the
 * application memory (tainted data) or call copy_and_verify to make a sanitized
 * copy of this data.
 *
 * @note This implementatation handles only non-class/non-struct T types.
 * Specializations of this class are generated by the rlbox_lib_load_classes
 * when T is a class/struct.
 *
 * @tparam T is the type of the data being wrapped.
 * @tparam TSbx is the type of the sandbox plugin that represents the underlying
 * sandbox implementation.
 */
template <typename T, typename TSbx>
class tainted_volatile_standard : public tainted_volatile_base<T, TSbx> {
  KEEP_RLBOX_CLASSES_FRIENDLY;

 private:
  using TSbxRep = detail::rlbox_base_types_convertor<T, TSbx>;

  // NOLINTNEXTLINE(hicpp-use-nullptr, modernize-use-nullptr)
  detail::value_type_t<TSbxRep> data{0};

  ////////////////////////////////

  inline tainted_volatile_standard() = default;

  /**
   * @brief Construct a new tainted object from another tainted wrapper
   * @tparam TWrap is the other wrapper type
   * @tparam TOther is the type of the rhs value being wrapped
   * @tparam RLBOX_REQUIRE param checks to see if this is (1) a tainted wrapper,
   * (2) meets the assignable criterion
   * @param aOther is the rhs being assigned
   */
  template <template <typename, typename...> typename TWrap, typename TOther,
            RLBOX_REQUIRE(detail::is_tainted_wrapper_v<TWrap, TOther, TSbx>&&
                              std::is_assignable_v<decltype(data)&, TOther>)>
  inline tainted_volatile_standard(const TWrap<TOther, TSbx>& aOther)
      : data(aOther.raw_sandbox_rep()) {}

 public:
  /**
   * @brief Unsafely remove the tainting and get the raw data.
   * @tparam TDummy is a dummy parameter to do our static type checks
   * @tparam RLBOX_REQUIRE param ensures this is allowed for primitive types
   * only.
   * @return detail::value_type_t<T> is the raw data
   */
  template <typename TDummy = T,
            RLBOX_REQUIRE(detail::is_fundamental_or_enum_v<TDummy>)>
  [[nodiscard]] inline detail::value_type_t<T> UNSAFE_unverified() const {
    auto converted =
        detail::convert_type_fundamental<detail::value_type_t<T>>(data);
    return converted;
  }

  /**
   * @brief Unsafely remove the tainting and get the raw data.
   * @param aSandbox is the sandbox this tainted value belongs to
   * @return detail::value_type_t<T> is the raw data
   */
  [[nodiscard]] inline detail::value_type_t<T> UNSAFE_unverified(
      [[maybe_unused]] rlbox_sandbox<TSbx>& aSandbox) const {
    if constexpr (std::is_pointer_v<T>) {
      return aSandbox.get_unsandboxed_pointer(data);
    } else {
      return UNSAFE_unverified();
    }
  }

  /**
   * @brief Unsafely remove the tainting and get the raw data in the sandboxed
   * ABI.
   * @return detail::value_type_t<TSbxRep> is the raw data
   */
  [[nodiscard]] inline detail::value_type_t<TSbxRep> UNSAFE_sandboxed() const {
    return data;
  }

  /**
   * @brief Unsafely remove the tainting and get the raw data converted to the
   * sandboxed ABI.
   * @param aSandbox is the sandbox this tainted value belongs to
   * @return detail::value_type_t<TSbxRep> is the raw data in the sandboxed ABI
   */
  [[nodiscard]] inline detail::value_type_t<TSbxRep> UNSAFE_sandboxed(
      [[maybe_unused]] rlbox_sandbox<TSbx>& aSandbox) const {
    return UNSAFE_sandboxed();
  }

  ////////////////////////////////

  /**
   * @brief Operator= for tainted values from another tainted wrapper
   * @tparam TWrap is the other wrapper type
   * @tparam TOther is the type of the rhs value being wrapped
   * @tparam RLBOX_REQUIRE param checks to see if this is (1) a tainted wrapper,
   * (2) meets the assignable criterion
   * @param aOther is the rhs being assigned
   * @return tainted_volatile_standard<T, TSbx>& is the reference to this value
   */
  template <template <typename, typename> typename TWrap, typename TOther,
            RLBOX_REQUIRE(detail::is_tainted_wrapper_v<TWrap, TOther, TSbx>&&
                              std::is_assignable_v<decltype(data)&, TOther>)>
  inline tainted_volatile_standard<T, TSbx>& operator=(
      const TWrap<T, TSbx>& aOther) {
    data = aOther.raw_sandbox_rep();
  }

  /**
   * @brief Operator= for tainted values from a raw primitive value.
   * @tparam TOther is the type of the rhs value being assigned
   * @tparam TDummy is a dummy parameter to do our static type checks
   * @tparam RLBOX_REQUIRE param ensures this is allowed for primitive types
   * only.
   * @param aOther is the raw primitive
   * @return tainted_volatile_standard<T, TSbx>& is the reference to this value
   */
  template <typename TOther, typename TDummy = T,
            RLBOX_REQUIRE(detail::is_fundamental_or_enum_v<TDummy>)>
  inline tainted_volatile_standard<T, TSbx>& operator=(const TOther& aOther) {
    // We need to first converting from host representation to sandbox
    // representation. Since the ABI is not consistent, converting an integer
    // for example may need a bounds check. This will occur automatically if we
    // convert this value to tainted value and convert that to tainted_volatile.
    using tainted_tother_t = typename TSbx::template tainted<TOther>;
    tainted_tother_t tainted_other(aOther);
    *this = tainted_other;
    return *this;
  }

  ////////////////////////////////

 private:
  template <typename TSub>
  using tainted = typename TSbx::template tainted<TSub>;

  using TOpDeref = tainted_volatile_standard<std::remove_pointer_t<T>, TSbx>;

 public:
  /**
   * @brief Operator* which dereferences tainted and gives a tainted_volatile&
   * @tparam TDummy is a dummy parameter to do our static type checks
   * @tparam RLBOX_REQUIRE ensures this is allowed for pointer types only.
   * @return TOpDeref& is the reference to the sandbox memory that holds this
   * data, i.e., memory which is a tainted_volatile type
   */
  template <typename TDummy = T, RLBOX_REQUIRE(std::is_pointer_v<TDummy>)>
  inline tainted_volatile_standard<std::remove_pointer_t<T>, TSbx>&
  operator*() {
    // Deliberately use a C style cast as we we want to get rid of any CV
    // qualifers here. CV qualifiers are moved inside the wrapper type and thus
    // continue to be tracked.

    // NOLINTNEXTLINE(google-readability-casting)
    auto data_tainted_volatile = (TOpDeref*)data;
    return *data_tainted_volatile;
  }

 private:
  using TOpAddrOf = tainted<std::add_pointer_t<T>>;

 public:
  /**
   * @brief Operator& which gets the address of tainted_volatile to get tainted
   * @return TOpAddrOf is the pointer to the sandbox memory that holds this data
   */
  inline TOpAddrOf operator&() {
    // Deliberately use a C style cast as we we want to get rid of any CV
    // qualifers here. CV qualifiers are moved inside the wrapper type and thus
    // continue to be tracked.

    // NOLINTNEXTLINE(google-readability-casting)
    auto* data_ptr = (std::add_pointer_t<T>)&data;
    TOpAddrOf ret(data_ptr);
    return ret;
  }
};

}  // namespace rlbox
