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
        return "invalid argument";
    case Status::NoMemory:
        return "out of memory";
    case Status::Again:
        return "not right now, try again";
    case Status::Busy:
        return "busy";
    case Status::TimedOut:
        return "timed out";
    case Status::Closed:
        return "closed";
    case Status::Full:
        return "full";
    case Status::Empty:
        return "empty";
    case Status::NotFound:
        return "not found";
    case Status::OsError:
        return "the system call failed";
    case Status::NotBuilt:
        return "not built yet (see the module in the header)";
    }
    return "unknown status";
}

} // namespace para
