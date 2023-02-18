#ifndef GW2COMBAT_CONFIGURATION_SKILL_HPP
#define GW2COMBAT_CONFIGURATION_SKILL_HPP

#include "common.hpp"

#include "actor/skill.hpp"

#include "attribute_conversion.hpp"
#include "attribute_modifier.hpp"
#include "effect_application.hpp"
#include "weapon.hpp"
#include "condition.hpp"

namespace gw2combat::configuration {

struct skill_t {
    actor::skill_t skill_key;
    actor::weapon_type weapon_type = actor::weapon_type::INVALID;

    double damage_coefficient = 0.0;
    int ammo = 1;
    int recharge_duration = 0;
    int num_targets = 1;

    std::array<int, 2> cast_duration = {0, 0};
    std::array<int, 2> cooldown = {0, 0};

    std::array<std::vector<int>, 2> strike_on_tick_list = {std::vector<int>{}, std::vector<int>{}};
    std::array<std::vector<int>, 2> pulse_on_tick_list = {std::vector<int>{}, std::vector<int>{}};

    std::vector<effect_application_t> on_strike_effect_applications{};
    std::vector<effect_application_t> on_pulse_effect_applications{};

    std::vector<attribute_modifier_t> attribute_modifiers{};
    std::vector<attribute_conversion_t> attribute_conversions{};

    std::vector<actor::skill_t> child_skill_keys;

    [[nodiscard]] inline bool operator==(const skill_t& rhs) const {
        return this->skill_key == rhs.skill_key;
    }
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(skill_t,
                                                skill_key,
                                                weapon_type,
                                                damage_coefficient,
                                                ammo,
                                                recharge_duration,
                                                num_targets,
                                                cast_duration,
                                                cooldown,
                                                strike_on_tick_list,
                                                pulse_on_tick_list,
                                                on_strike_effect_applications,
                                                on_pulse_effect_applications,
                                                attribute_modifiers,
                                                attribute_conversions,
                                                child_skill_keys)

}  // namespace gw2combat::configuration

#endif  // GW2COMBAT_CONFIGURATION_SKILL_HPP
