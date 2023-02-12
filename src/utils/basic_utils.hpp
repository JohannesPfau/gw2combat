#ifndef GW2COMBAT_UTILS_BASIC_UTILS_HPP
#define GW2COMBAT_UTILS_BASIC_UTILS_HPP

#include <random>

#include "common.hpp"

namespace gw2combat::utils {

template <typename T>
[[nodiscard]] static inline std::string to_string(T t) {
    return nlohmann::json{t}[0].dump();
}

[[nodiscard]] static inline double get_random_0_100() {
    static std::random_device random_device;
    static unsigned int rng_seed = random_device();
    static std::mt19937 generator(rng_seed);
    static std::uniform_real_distribution distribution(0.0, 100.0);
    return distribution(generator);
}

[[nodiscard]] static inline bool check_random_success(double upper_bound) {
    return get_random_0_100() < upper_bound;
}

[[nodiscard]] static inline tick_t get_current_tick(registry_t& registry) {
    return registry.ctx().get<const tick_t>();
}

}  // namespace gw2combat::utils

#endif  // GW2COMBAT_UTILS_BASIC_UTILS_HPP