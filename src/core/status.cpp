#include <core/status.hpp>
#include <src/core/internal.hpp>

#include <cerrno>

namespace para {

namespace {
thread_local int g_last_os_error = 0;
} // namespace

namespace detail {

void set_os_error(int err) noexcept {
    g_last_os_error = err;
}

Status from_errno(int err) noexcept {
    set_os_error(err);
    switch (err) {
    case 0:
        return Status::Ok;
    case EINVAL:
        return Status::Invalid;
    case ENOMEM:
        return Status::NoMemory;
    case EAGAIN:
        return Status::Again;
    case EBUSY:
        return Status::Busy;
    case ETIMEDOUT:
        return Status::TimedOut;
    default:
        return Status::OsError;
    }
}

} // namespace detail

int last_os_error() noexcept {
    return g_last_os_error;
}

std::string_view to_string(Status st) noexcept {
    switch (st) {
    case Status::Ok:
        return "ok";
    case Status::Invalid:
        return "ogiltigt argument";
    case Status::NoMemory:
        return "slut på minne";
    case Status::Again:
        return "inte just nu, försök igen";
    case Status::Busy:
        return "upptagen";
    case Status::TimedOut:
        return "tidsgränsen gick ut";
    case Status::Closed:
        return "stängd";
    case Status::Full:
        return "full";
    case Status::Empty:
        return "tom";
    case Status::NotFound:
        return "hittades inte";
    case Status::OsError:
        return "systemanropet misslyckades";
    case Status::NotBuilt:
        return "inte byggd ännu (se modulen i huvudfilen)";
    }
    return "okänd status";
}

} // namespace para
