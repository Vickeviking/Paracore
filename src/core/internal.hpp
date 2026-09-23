/* Internal to src/core. Deliberately NOT in core/ — what lives in core/ is
 * public, and the boundary should be visible in the file tree.
 *
 * (The templates in ds/ and mem/ cannot follow exactly that rule — a template
 * has to reach every translation unit — so there the private half is called
 * ds/detail/ and mem/detail/. Same rule, different mechanics. See
 * ds/detail/stack_impl.hpp.) */
#ifndef PARACORE_SRC_CORE_INTERNAL_HPP
#define PARACORE_SRC_CORE_INTERNAL_HPP

#include <core/status.hpp>

namespace para::detail {

void set_os_error(int err) noexcept;

/* errno -> Status, and keep the raw code for debugging. */
[[nodiscard]] Status from_errno(int err) noexcept;

} // namespace para::detail

#endif /* PARACORE_SRC_CORE_INTERNAL_HPP */
