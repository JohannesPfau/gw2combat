#ifndef GW2COMBAT_UTILS_SKILL_UTILS_HPP
#define GW2COMBAT_UTILS_SKILL_UTILS_HPP

#include "basic_utils.hpp"
#include "entity_utils.hpp"
#include "io_utils.hpp"

#include "component/actor/actor_skills.hpp"
#include "component/equipment/bundle.hpp"
#include "component/equipment/weapons.hpp"
#include "component/skill/ammo.hpp"
#include "component/skill/is_skill.hpp"
#include "component/temporal/cooldown_component.hpp"

#include "configuration/skill.hpp"

namespace gw2combat::utils {

[[nodiscard]] static inline entity_t get_skill_entity(const actor::skill_t& skill,
                                                      entity_t actor_entity,
                                                      registry_t& registry) {
    return registry.get<component::actor_skills>(actor_entity).skill_key_to_entity_map.at(skill);
}

[[nodiscard]] static inline configuration::skill_t&
get_skill_configuration(const actor::skill_t& skill, entity_t actor_entity, registry_t& registry) {
    return registry.get<component::is_skill>(utils::get_skill_entity(skill, actor_entity, registry))
        .skill_configuration;
}

static inline void assert_can_cast_skill(const actor::skill_t& skill,
                                         entity_t actor_entity,
                                         registry_t& registry) {
    if (registry.any_of<component::owner_component>(actor_entity)) {
        return;
    }

    auto skill_entity = utils::get_skill_entity(skill, actor_entity, registry);
    // if (registry.any_of<component::internal_cooldown_component>(skill_entity)) {
    //     throw std::runtime_error(fmt::format("skill {} is on internal cooldown", skill));
    // }

    auto bundle_ptr = registry.try_get<component::bundle_component>(actor_entity);
    auto& skill_ammo = registry.get<component::ammo>(skill_entity);
    if (skill_ammo.current_ammo <= 0 && !(skill == "Weapon Swap" && bundle_ptr)) {
        utils::log_component<component::cooldown_component>(registry);
        throw std::runtime_error(fmt::format("actor {} skill {} doesn't have any more ammo",
                                             get_entity_name(actor_entity, registry),
                                             to_string(skill)));
    }

    auto& skill_configuration = utils::get_skill_configuration(skill, actor_entity, registry);
    if (skill_configuration.required_bundle.empty()) {
        if (skill_configuration.weapon_type != actor::weapon_type::INVALID &&
            skill_configuration.weapon_type != actor::weapon_type::EMPTY_HANDED &&
            skill_configuration.weapon_type != actor::weapon_type::MAIN_HAND &&
            skill_configuration.weapon_type != actor::weapon_type::KIT_CONJURE &&
            skill_configuration.weapon_type != actor::weapon_type::TOME) {
            auto& weapons = registry.get<component::equipped_weapons>(actor_entity).weapons;
            auto current_weapon_set = registry.get<component::current_weapon_set>(actor_entity).set;
            bool skill_available_on_weapon =
                std::any_of(weapons.begin(), weapons.end(), [&](const component::weapon_t& weapon) {
                    return weapon.set == current_weapon_set &&
                           weapon.type == skill_configuration.weapon_type;
                });
            if (!skill_available_on_weapon) {
                throw std::runtime_error(
                    fmt::format("skill {} not available on this weapon set", skill));
            }
        }
    } else {
        if (!bundle_ptr) {
            throw std::runtime_error(fmt::format(
                "skill {} requires bundle {}", skill, skill_configuration.required_bundle));
        } else if (skill_configuration.required_bundle != bundle_ptr->name) {
            throw std::runtime_error(fmt::format("skill {} requires bundle {}, but current have {}",
                                                 skill,
                                                 skill_configuration.required_bundle,
                                                 bundle_ptr->name));
        }
    }
}

[[nodiscard]] static inline bool skill_has_tag(const configuration::skill_t& skill,
                                               const actor::skill_tag_t& skill_tag) {
    return std::find(skill.tags.cbegin(), skill.tags.cend(), skill_tag) != skill.tags.cend();
}

static inline void put_skill_on_cooldown(const actor::skill_t& skill,
                                         entity_t actor_entity,
                                         registry_t& registry) {
    auto skill_entity = utils::get_skill_entity(skill, actor_entity, registry);

    auto& skill_ammo = registry.get<component::ammo>(skill_entity);
    if (skill_ammo.current_ammo <= 0) {
        utils::log_component<component::cooldown_component>(registry);
        throw std::runtime_error(fmt::format("actor {} skill {} doesn't have any more ammo",
                                             get_entity_name(actor_entity, registry),
                                             to_string(skill)));
    }
    --skill_ammo.current_ammo;

    auto& skill_configuration = utils::get_skill_configuration(skill, actor_entity, registry);
    if (!registry.any_of<component::cooldown_component>(skill_entity)) {
        registry.emplace<component::cooldown_component>(
            skill_entity, component::cooldown_component{skill_configuration.cooldown});
    }
}

}  // namespace gw2combat::utils

#endif  // GW2COMBAT_UTILS_SKILL_UTILS_HPP
