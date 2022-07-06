#include "entity.hpp"

#include "gw2combat/effect.hpp"

#include "gw2combat/component/boon/aegis.hpp"
#include "gw2combat/component/boon/fury.hpp"
#include "gw2combat/component/boon/might.hpp"
#include "gw2combat/component/boon/quickness.hpp"
#include "gw2combat/component/boon/resolution.hpp"
#include "gw2combat/component/character_input.hpp"
#include "gw2combat/component/condition/burning.hpp"
#include "gw2combat/component/condition/vulnerability.hpp"
#include "gw2combat/component/condition_tick_status.hpp"
#include "gw2combat/component/dynamic_attributes.hpp"
#include "gw2combat/component/profession/guardian/trait/fiery_wrath.hpp"
#include "gw2combat/component/profession/guardian/virtue_of_justice.hpp"
#include "gw2combat/component/static_attributes.hpp"
#include "gw2combat/component/targeting.hpp"

namespace gw2combat {

std::unique_ptr<entt::entity> singleton_entity;

entt::entity build_core_guard_no_gear_no_traits_gs(entt::registry& registry) {
    auto entity = registry.create();

    auto viper_armor_rest_zerk_crit_food =
        component::static_attributes{.power = 3004,
                                     .precision = 2032,
                                     .toughness = 1000,
                                     .vitality = 1235,
                                     .concentration = 0,
                                     .condition_damage = 1287,
                                     .expertise = 207,
                                     .ferocity = 1090,
                                     .healing_power = 0,
                                     .armor = 2271,
                                     .boon_duration_pct = 0.0,
                                     .critical_chance_pct = 79.1,
                                     .critical_damage_pct = 222.6,
                                     .condition_duration_pct = 13.8,
                                     .max_health = 13995};
    auto full_zerk_crit_food = component::static_attributes{.power = 3067,
                                                            .precision = 2140,
                                                            .toughness = 1000,
                                                            .vitality = 1235,
                                                            .concentration = 0,
                                                            .condition_damage = 161,
                                                            .expertise = 0,
                                                            .ferocity = 1435,
                                                            .healing_power = 0,
                                                            .armor = 2271,
                                                            .boon_duration_pct = 0.0,
                                                            .critical_chance_pct = 84.2,
                                                            .critical_damage_pct = 243.6,
                                                            .condition_duration_pct = 0.0,
                                                            .max_health = 14615};
    auto full_zerk = component::static_attributes{.power = 3067,
                                                  .precision = 2040,
                                                  .toughness = 1000,
                                                  .vitality = 1235,
                                                  .concentration = 0,
                                                  .condition_damage = 161,
                                                  .expertise = 0,
                                                  .ferocity = 1335,
                                                  .healing_power = 0,
                                                  .armor = 2271,
                                                  .boon_duration_pct = 0.0,
                                                  .critical_chance_pct = 79.5,
                                                  .critical_damage_pct = 238.9,
                                                  .condition_duration_pct = 0.0,
                                                  .max_health = 14615};
    registry.emplace<component::static_attributes>(entity, full_zerk);
    registry.emplace<component::dynamic_attributes>(
        entity,
        component::dynamic_attributes{
            .weapon_strength = 1100, .max_endurance = 100, .endurance_gain_pct = 0});

    registry.emplace<component::virtue_of_justice>(
        entity,
        component::virtue_of_justice{.consecutive_successful_hits = 0,
                                     .apply_burning_on_tick = tick_t{3'000'000'000}});
    registry.emplace<component::fiery_wrath>(entity);

    registry.emplace<component::character_input>(
        entity, component::character_input{component::character_input::command{0}, tick_t{0}});

    {  // NOTE: Setting up some default boons for testing
        registry.emplace<component::might>(entity, component::might{});
        for (int i = 0; i < 25; ++i) {
            registry.get<component::might>(entity).stacks.emplace(
                effect{entity, tick_t{9'000'000}});
        }
        registry.emplace<component::fury>(entity,
                                          component::fury{effect{entity, tick_t{9'000'000}}});
        registry.emplace<component::quickness>(
            entity, component::quickness{effect{entity, tick_t{9'000'000}}});
        registry.emplace<component::resolution>(
            entity, component::resolution{effect{entity, tick_t{9'000'000}}});
        registry.emplace<component::aegis>(entity,
                                           component::aegis{effect{entity, tick_t{9'000'000}}});
    }

    return entity;
}

entt::entity build_medium_kitty_golem(entt::registry& registry) {
    auto entity = registry.create();

    registry.emplace<component::static_attributes>(
        entity,
        component::static_attributes{
            0, 0, 0, 0, 0, 0, 0, 0, 0, 2597, 0.0, 0.0, 0.0, 0.0, 4'000'000});
    registry.emplace<component::dynamic_attributes>(entity, component::dynamic_attributes{0, 0, 0});

    registry.emplace<component::character_input>(
        entity, component::character_input{component::character_input::command{0}, tick_t{0}});

    {  // NOTE: Setting up some default conditions for testing
        registry.emplace<component::vulnerability>(entity, component::vulnerability{});
        for (int i = 0; i < 25; ++i) {
            registry.get<component::vulnerability>(entity).stacks.emplace(
                effect{entity, tick_t{9'000'000}});
        }
    }

    return entity;
}

void init_entities(entt::registry& registry) {
    singleton_entity = std::make_unique<entt::entity>(registry.create());
    auto player1 = build_core_guard_no_gear_no_traits_gs(registry);
    auto golem = build_medium_kitty_golem(registry);

    // FIXME: Maybe make a golem boon/condi provider entity later for cleaner code
    registry.emplace<component::static_attributes>(
        *singleton_entity, component::static_attributes{.condition_damage = 211});
    registry.emplace<component::dynamic_attributes>(*singleton_entity,
                                                    component::dynamic_attributes{});
    registry.emplace<component::condition_tick_status>(*singleton_entity,
                                                       component::condition_tick_status{tick_t{0}});

    // NOTE: Hardcoded for testing
    registry.emplace<component::targeting>(player1, component::targeting{golem});
    auto& burning = registry.emplace<component::burning>(golem, component::burning{});
    for (int i = 0; i < 1; ++i) {
        burning.stacks.emplace(effect{*singleton_entity, 3500000});
    }
}

}  // namespace gw2combat
