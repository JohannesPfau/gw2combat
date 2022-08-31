#ifndef GW2COMBAT_BUILD_HPP
#define GW2COMBAT_BUILD_HPP

#include "gw2combat/common.hpp"

#include "attributes.hpp"
#include "base_class.hpp"
#include "nourishment_enhancement.hpp"
#include "rune.hpp"
#include "skill.hpp"
#include "trait.hpp"
#include "weapon.hpp"
#include "unique_effect.hpp"

namespace gw2combat::actor {

struct build_t {
    [[nodiscard]] static build_t read(const std::string& path);

    base_class_t base_class;
    std::vector<trait_line_t> trait_lines;
    std::vector<std::string> traits;
    attributes_configuration_t attributes;
    std::vector<weapon> available_weapon_configurations;
    std::vector<std::string> equipped_slot_skill_names;
    std::vector<unique_effect_t> permanent_unique_effects;
    rune_t rune;
    nourishment_t nourishment;
    enhancement_t enhancement;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(build_t,
                                                base_class,
                                                trait_lines,
                                                traits,
                                                attributes,
                                                available_weapon_configurations,
                                                equipped_slot_skill_names,
                                                permanent_unique_effects,
                                                rune,
                                                nourishment,
                                                enhancement)

}  // namespace gw2combat::actor

#endif  // GW2COMBAT_BUILD_HPP
