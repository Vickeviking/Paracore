#include <core/mutex.hpp>
#include <src/core/internal.hpp>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <ctime>

namespace para {

namespace {

/* Ett fel från pthread_mutex_lock i ett ERRORCHECK-bygge betyder att
 * programmet är trasigt på ett sätt ingen returkod hade räddat: rekursivt
 * lås, eller unlock från en tråd som inte håller låset. Att returnera en
 * status hade bara flyttat kraschen. Abort med besked, och låt testriggen
 * rapportera SIGNAL med testets namn. */
[[noreturn]] void mutex_fatal(const char *op, int rc) noexcept {
    const char *why = "okänt fel";
    if (rc == EDEADLK) {
        why = "rekursivt lås — tråden håller redan det här låset";
    } else if (rc == EPERM) {
        why = "unlock från en tråd som inte håller låset";
    } else if (rc == EINVAL) {
        why = "ogiltigt lås (destruerat? aldrig initierat?)";
    }
    std::fprintf(stderr,
                 "\nparacore: Mutex::%s misslyckades (%d): %s\n"
                 "          Debugbygget kör med PTHREAD_MUTEX_ERRORCHECK just för att\n"
                 "          fånga det här i stället för att låta det bli en deadlock.\n\n",
                 op, rc, why);
    std::abort();
}

} // namespace

Mutex::Mutex() noexcept {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
#if defined(PARA_MUTEX_CHECKED)
    /* Debugbygget: fånga rekursivt lås och unlock-från-fel-tråd som ett FEL,
     * i stället för att låta dem bli en deadlock du felsöker på natten.
     *
     * std::mutex har ingen motsvarighet — att låsa den rekursivt är
     * odefinierat beteende och i praktiken en hängning. Det är ena halvan av
     * varför Paracore har en egen. */
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
    /* MONOTONIC, inte REALTIME: en NTP-justering mitt i en timeout får inte
     * förlänga eller förkorta väntan. Samma skäl som bench::now_ns().
     *
     * std::condition_variable::wait_until mäter mot system_clock och har
     * precis det problemet. Det är andra halvan av varför Paracore har egna. */
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
