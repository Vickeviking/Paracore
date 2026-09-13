/* Internt för src/core. Ligger medvetet INTE i core/ — det som ligger i
 * core/ är publikt, och gränsen ska gå att se i filträdet.
 *
 * (Mallarna i ds/ och mem/ kan inte följa exakt den regeln — en mall måste nå
 * varje översättningsenhet — så där heter den privata halvan ds/detail/ och
 * mem/detail/. Samma regel, annan mekanik. Se ds/detail/stack_impl.hpp.) */
#ifndef PARACORE_SRC_CORE_INTERNAL_HPP
#define PARACORE_SRC_CORE_INTERNAL_HPP

#include <core/status.hpp>

namespace para::detail {

void set_os_error(int err) noexcept;

/* errno -> Status, och spara den råa koden för felsökning. */
[[nodiscard]] Status from_errno(int err) noexcept;

} // namespace para::detail

#endif /* PARACORE_SRC_CORE_INTERNAL_HPP */
