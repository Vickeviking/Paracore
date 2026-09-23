#include <core/mutex.hpp>
#include <src/core/internal.hpp>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace para {

namespace {

/* An error from pthread_mutex_lock in an ERRORCHECK build means the program
 * is broken in a way no return code would have saved: a recursive lock, or
 * unlock from a thread that does not hold the lock. Returning a status would
 * only have moved the crash. Abort with a message, and let the test rig
 * report SIGNAL with the test's name. */
[[noreturn]] void mutex_fatal(const char *op, int rc) noexcept {
    const char *why = "unknown error";
    if (rc == EDEADLK) {
        why = "recursive lock — the thread already holds this lock";
    } else if (rc == EPERM) {
        why = "unlock from a thread that does not hold the lock";
    } else if (rc == EINVAL) {
        why = "invalid lock (destroyed? never initialised?)";
    }
    std::fprintf(stderr,
                 "\nparacore: Mutex::%s failed (%d): %s\n"
                 "          The debug build runs with PTHREAD_MUTEX_ERRORCHECK precisely to\n"
                 "          catch this instead of letting it become a deadlock.\n\n",
                 op, rc, why);
    std::abort();
}

} // namespace

Mutex::Mutex() noexcept {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#if defined(PARA_MUTEX_CHECKED)
    /* The debug build: catch a recursive lock and unlock-from-the-wrong-thread
     * as an ERROR, instead of letting them become a deadlock you debug at
     * night.
     *
     * std::mutex has no equivalent — locking it recursively is undefined
     * behaviour and in practice a hang. That is one half of why Paracore has
     * its own. */
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
#endif
    const int rc = pthread_mutex_init(&m_, &attr);
    pthread_mutexattr_destroy(&attr);
    if (rc != 0) {
        mutex_fatal("Mutex", rc);
    }
}

Mutex::~Mutex() {
    pthread_mutex_destroy(&m_);
}

void Mutex::lock() noexcept {
    const int rc = pthread_mutex_lock(&m_);
    if (rc != 0) {
        mutex_fatal("lock", rc);
    }
}

bool Mutex::try_lock() noexcept {
    const int rc = pthread_mutex_trylock(&m_);
    if (rc == 0) {
        return true;
    }
    if (rc == EBUSY) {
        return false;
    }
    mutex_fatal("try_lock", rc);
}

void Mutex::unlock() noexcept {
    const int rc = pthread_mutex_unlock(&m_);
    if (rc != 0) {
        mutex_fatal("unlock", rc);
    }
}

CondVar::CondVar() noexcept {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);
    /* MONOTONIC, not REALTIME: an NTP adjustment in the middle of a timeout
     * must not lengthen or shorten the wait. Same reason as bench::now_ns().
     *
     * std::condition_variable::wait_until measures against system_clock and
     * has exactly that problem. That is the other half of why Paracore has its
     * own. */
    pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
    const int rc = pthread_cond_init(&c_, &attr);
    pthread_condattr_destroy(&attr);
    if (rc != 0) {
        mutex_fatal("CondVar", rc);
    }
}

CondVar::~CondVar() {
    pthread_cond_destroy(&c_);
}

void CondVar::wait(std::unique_lock<Mutex> &lk) noexcept {
    const int rc = pthread_cond_wait(&c_, lk.mutex()->native_handle());
    if (rc != 0) {
        mutex_fatal("CondVar::wait", rc);
    }
}

Status CondVar::wait_for(std::unique_lock<Mutex> &lk, std::chrono::milliseconds timeout) noexcept {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    const auto ms = timeout.count() < 0 ? 0 : timeout.count();
    ts.tv_sec += static_cast<time_t>(ms / 1000);
    ts.tv_nsec += static_cast<long>((ms % 1000) * 1000000);
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    const int rc = pthread_cond_timedwait(&c_, lk.mutex()->native_handle(), &ts);
    if (rc == ETIMEDOUT) {
        return Status::TimedOut;
    }
    if (rc != 0) {
        return detail::from_errno(rc);
    }
    return Status::Ok;
}

void CondVar::notify_one() noexcept {
    (void)pthread_cond_signal(&c_);
}

void CondVar::notify_all() noexcept {
    (void)pthread_cond_broadcast(&c_);
}

} // namespace para
