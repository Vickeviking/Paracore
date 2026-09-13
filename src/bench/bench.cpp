/* bench.cpp
 *
 * MODUL 6 fyller den här filen. now_ns() är dock redan riktig: testriggens
 * watchdog och varenda framtida mätning behöver en monoton klocka, och den
 * är inte där kursen ligger.
 */
#include <bench/bench.hpp>

#include <ctime>
#include <ostream>

namespace para::bench {

std::uint64_t now_ns() noexcept {
    timespec ts{};
    /* MONOTONIC, inte REALTIME: en NTP-justering mitt i en mätning ska inte
     * kunna göra en operation "negativt lång". Det händer, och det är en
     * mardröm att felsöka i efterhand.
     *
     * (std::chrono::steady_clock gör samma sak och hade dugt. clock_gettime
     * ligger kvar för att det är EN rad mindre indirektion när du tittar på
     * assemblern i modul 6 — mätriggens egen overhead är en siffra du ska
     * kunna redovisa.) */
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL +
           static_cast<std::uint64_t>(ts.tv_nsec);
}

Status write_header(std::ostream &out, const Config &cfg) {
    (void)out;
    (void)cfg;
    return Status::NotBuilt;
}

} // namespace para::bench
