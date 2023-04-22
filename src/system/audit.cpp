#include "audit.hpp"

#include "utils/effect_utils.hpp"
#include "utils/entity_utils.hpp"
#include "utils/skill_utils.hpp"

#include "component/actor/casting_skill.hpp"
#include "component/actor/combat_stats.hpp"
#include "component/actor/finished_casting_skill.hpp"
#include "component/actor/is_actor.hpp"
#include "component/actor/is_downstate.hpp"
#include "component/audit/audit_component.hpp"
#include "component/damage/effects_pipeline.hpp"
#include "component/damage/incoming_damage.hpp"
#include "component/effect/is_effect.hpp"
#include "component/effect/is_unique_effect.hpp"
#include "component/effect/source_actor.hpp"
#include "component/effect/source_skill.hpp"
#include "component/equipment/bundle.hpp"
#include "component/skill/is_skill.hpp"
#include "component/temporal/animation_component.hpp"
#include "component/temporal/duration_component.hpp"
#include "component/temporal/has_alacrity.hpp"

namespace gw2combat::system {

std::vector<audit::skill_cooldown_t> get_skill_cooldowns(registry_t& registry) {
    std::vector<audit::skill_cooldown_t> skill_cooldowns;
    registry.view<component::cooldown_component, component::is_skill>().each(
        [&](entity_t skill_entity,
            const component::cooldown_component& cooldown_component,
            const component::is_skill& is_skill) {
            bool has_alacrity = registry.any_of<component::has_alacrity>(
                registry.get<component::owner_component>(skill_entity).entity);
            skill_cooldowns.emplace_back(audit::skill_cooldown_t{
                .skill = is_skill.skill_configuration.skill_key,
                .duration = cooldown_component.duration[has_alacrity],
            });
        });
    return skill_cooldowns;
}

[[maybe_unused]] std::unordered_map<std::basic_string<char>,
                                    std::unordered_map<actor::attribute_t, double>>
get_actor_attributes(registry_t& registry) {
    std::unordered_map<std::basic_string<char>, std::unordered_map<actor::attribute_t, double>>
        actor_attributes;
    registry.view<component::is_actor, component::relative_attributes>().each(
        [&](entity_t actor_entity, const component::relative_attributes& relative_attributes) {
            actor_attributes[utils::get_entity_name(actor_entity, registry)] =
                relative_attributes.entity_and_attribute_to_value_map.at(actor_entity);
        });
    return actor_attributes;
}

audit::tick_event_t create_tick_event(const decltype(audit::tick_event_t::event)& event,
                                      entity_t actor_entity,
                                      registry_t& registry) {
    return audit::tick_event_t{
        .time_ms = utils::get_current_tick(registry),
        .actor = utils::get_entity_name(actor_entity, registry),
        .event = event,
        .skill_cooldowns = get_skill_cooldowns(registry),
        //.actor_attributes = get_actor_attributes(registry),
    };
}

void audit_bundles(registry_t& registry) {
    auto& audit_component = registry.get<component::audit_component>(utils::get_singleton_entity());
    registry.view<component::equipped_bundle>().each(
        [&](entity_t actor_entity, const component::equipped_bundle& equipped_bundle) {
            audit_component.old_events.emplace_back(audit::old_equipped_bundle_event_t{
                .time_ms = utils::get_current_tick(registry),
                .actor = utils::get_entity_name(actor_entity, registry),
                .bundle = equipped_bundle.name,
            });
            audit_component.events.emplace_back(create_tick_event(
                audit::equipped_bundle_event_t{
                    .bundle = equipped_bundle.name,
                },
                actor_entity,
                registry));
        });
    registry.view<component::dropped_bundle>().each(
        [&](entity_t actor_entity, const component::dropped_bundle& dropped_bundle) {
            audit_component.old_events.emplace_back(audit::old_dropped_bundle_event_t{
                .time_ms = utils::get_current_tick(registry),
                .actor = utils::get_entity_name(actor_entity, registry),
                .bundle = dropped_bundle.name,
            });
            audit_component.events.emplace_back(create_tick_event(
                audit::dropped_bundle_event_t{
                    .bundle = dropped_bundle.name,
                },
                actor_entity,
                registry));
        });
}

void audit_actor_downstate(registry_t& registry) {
    auto& audit_component = registry.get<component::audit_component>(utils::get_singleton_entity());
    registry.view<component::is_downstate>().each([&](entity_t actor_entity) {
        audit_component.old_events.emplace_back(audit::old_actor_downstate_event_t{
            .time_ms = utils::get_current_tick(registry),
            .actor = utils::get_entity_name(actor_entity, registry),
        });
        audit_component.events.emplace_back(
            create_tick_event(audit::actor_downstate_event_t{}, actor_entity, registry));
    });
}

void audit_effect_expiration(registry_t& registry) {
    auto& audit_component = registry.get<component::audit_component>(utils::get_singleton_entity());
    registry.view<component::duration_expired>().each([&](entity_t entity) {
        auto actor_entity = utils::get_owner(entity, registry);
        auto source_actor = registry.get<component::source_actor>(entity);
        auto source_skill = registry.get<component::source_skill>(entity);
        auto effect = registry.any_of<component::is_effect>(entity)
                          ? nlohmann::json{registry.get<component::is_effect>(entity).effect}[0]
                                .get<std::string>()
                          : "";
        auto unique_effect =
            registry.any_of<component::is_unique_effect>(entity)
                ? registry.get<component::is_unique_effect>(entity).unique_effect.unique_effect_key
                : "";
        audit_component.old_events.emplace_back(audit::old_effect_expired_event_t{
            .time_ms = utils::get_current_tick(registry),
            .actor = utils::get_entity_name(actor_entity, registry),
            .source_actor = utils::get_entity_name(source_actor.entity, registry),
            .source_skill = source_skill.skill,
            .effect = effect,
            .unique_effect = unique_effect,
        });
        audit_component.events.emplace_back(create_tick_event(
            audit::effect_expired_event_t{
                .source_actor = utils::get_entity_name(source_actor.entity, registry),
                .source_skill = source_skill.skill,
                .effect = effect,
                .unique_effect = unique_effect,
            },
            actor_entity,
            registry));
    });
}

void audit_actor_created(registry_t& registry) {
    auto& audit_component = registry.get<component::audit_component>(utils::get_singleton_entity());
    registry.view<component::actor_created>().each([&](entity_t actor_entity) {
        audit_component.old_events.emplace_back(audit::old_actor_created_event_t{
            .time_ms = utils::get_current_tick(registry),
            .actor = utils::get_entity_name(actor_entity, registry),
        });
        audit_component.events.emplace_back(
            create_tick_event(audit::actor_created_event_t{}, actor_entity, registry));
    });
}

void audit_combat_stats_update(registry_t& registry) {
    auto& audit_component = registry.get<component::audit_component>(utils::get_singleton_entity());
    registry.view<component::combat_stats_updated, component::combat_stats>().each(
        [&](entity_t actor_entity, const component::combat_stats& combat_stats) {
            audit_component.old_events.emplace_back(audit::old_combat_stats_update_event_t{
                .time_ms = utils::get_current_tick(registry),
                .actor = utils::get_entity_name(actor_entity, registry),
                .updated_health = combat_stats.health,
            });
            audit_component.events.emplace_back(create_tick_event(
                audit::combat_stats_update_event_t{
                    .updated_health = combat_stats.health,
                },
                actor_entity,
                registry));
        });
}

void audit_effect_applications(registry_t& registry) {
    auto& audit_component = registry.get<component::audit_component>(utils::get_singleton_entity());
    registry.view<component::incoming_effects_component>().each(
        [&](entity_t actor_entity,
            const component::incoming_effects_component& incoming_effects_component) {
            for (auto&& [source_entity, effect_application] :
                 incoming_effects_component.effect_applications) {
                auto actual_source_entity = utils::get_owner(source_entity, registry);
                auto source_actor = utils::get_entity_name(actual_source_entity, registry);
                auto effect = effect_application.effect == actor::effect_t::INVALID
                                  ? ""
                                  : nlohmann::json{effect_application.effect}[0].get<std::string>();
                auto& application_source_relative_attributes =
                    registry.get<component::relative_attributes>(actual_source_entity);
                int effective_duration = effect_application.effect == actor::effect_t::INVALID
                                             ? effect_application.base_duration_ms
                                             : utils::get_effective_effect_duration(
                                                   effect_application.base_duration_ms,
                                                   effect_application.effect,
                                                   application_source_relative_attributes,
                                                   actor_entity);
                audit_component.old_events.emplace_back(audit::old_effect_application_event_t{
                    .time_ms = utils::get_current_tick(registry),
                    .actor = utils::get_entity_name(actor_entity, registry),
                    .source_actor = source_actor,
                    .source_skill = effect_application.source_skill,
                    .effect = effect,
                    .unique_effect = effect_application.unique_effect.unique_effect_key,
                    .num_stacks = effect_application.num_stacks,
                    .duration_ms = effective_duration,
                });
                audit_component.events.emplace_back(create_tick_event(
                    audit::effect_application_event_t{
                        .source_actor = source_actor,
                        .source_skill = effect_application.source_skill,
                        .effect = effect,
                        .unique_effect = effect_application.unique_effect.unique_effect_key,
                        .num_stacks = effect_application.num_stacks,
                        .duration_ms = effective_duration,
                    },
                    actor_entity,
                    registry));
            }
        });
}

void audit_skill_casts(registry_t& registry) {
    auto& audit_component = registry.get<component::audit_component>(utils::get_singleton_entity());
    registry.view<component::casting_skill, component::animation_component>().each(
        [&](entity_t actor_entity,
            const component::casting_skill& casting_skill,
            const component::animation_component& animation_component) {
            if (animation_component.progress[0] + animation_component.progress[1] > 1) {
                return;
            }

            audit_component.old_events.emplace_back(audit::old_skill_cast_begin_event_t{
                .time_ms = utils::get_current_tick(registry),
                .actor = utils::get_entity_name(actor_entity, registry),
                .skill = registry.get<component::is_skill>(casting_skill.skill_entity)
                             .skill_configuration.skill_key,
            });
            audit_component.events.emplace_back(create_tick_event(
                audit::skill_cast_begin_event_t{
                    .skill = registry.get<component::is_skill>(casting_skill.skill_entity)
                                 .skill_configuration.skill_key,
                },
                actor_entity,
                registry));
        });

    registry.view<component::finished_casting_skill>().each(
        [&](entity_t actor_entity,
            const component::finished_casting_skill& finished_casting_skill) {
            audit_component.old_events.emplace_back(audit::old_skill_cast_end_event_t{
                .time_ms = utils::get_current_tick(registry),
                .actor = utils::get_entity_name(actor_entity, registry),
                .skill = registry.get<component::is_skill>(finished_casting_skill.skill_entity)
                             .skill_configuration.skill_key,
            });
            audit_component.events.emplace_back(create_tick_event(
                audit::skill_cast_end_event_t{
                    .skill = registry.get<component::is_skill>(finished_casting_skill.skill_entity)
                                 .skill_configuration.skill_key,
                },
                actor_entity,
                registry));
        });
}

void audit_damage(registry_t& registry) {
    auto& audit_component = registry.get<component::audit_component>(utils::get_singleton_entity());
    registry.view<component::incoming_damage>().each(
        [&](entity_t actor_entity, const component::incoming_damage& incoming_damage) {
            for (auto& incoming_damage_event : incoming_damage.incoming_damage_events) {
                auto old_damage_type = [&]() {
                    if (incoming_damage_event.effect != actor::effect_t::INVALID) {
                        switch (incoming_damage_event.effect) {
                            case actor::effect_t::BURNING:
                                return audit::old_damage_event_t::damage_type_t::BURNING;
                            case actor::effect_t::BLEEDING:
                                return audit::old_damage_event_t::damage_type_t::BLEEDING;
                            case actor::effect_t::TORMENT:
                                return audit::old_damage_event_t::damage_type_t::TORMENT_STATIONARY;
                            case actor::effect_t::POISON:
                                return audit::old_damage_event_t::damage_type_t::POISON;
                            case actor::effect_t::CONFUSION:
                                return audit::old_damage_event_t::damage_type_t::CONFUSION;
                            case actor::effect_t::BINDING_BLADE:
                                return audit::old_damage_event_t::damage_type_t::BINDING_BLADE;
                            default:
                                throw std::runtime_error("Unknown source for damage!");
                        }
                    } else if (!incoming_damage_event.skill.empty()) {
                        return audit::old_damage_event_t::damage_type_t::STRIKE;
                    } else {
                        throw std::runtime_error("Unknown source for damage!");
                    }
                }();
                auto damage_type = [&]() {
                    if (incoming_damage_event.effect != actor::effect_t::INVALID) {
                        switch (incoming_damage_event.effect) {
                            case actor::effect_t::BURNING:
                                return audit::damage_event_t::damage_type_t::BURNING;
                            case actor::effect_t::BLEEDING:
                                return audit::damage_event_t::damage_type_t::BLEEDING;
                            case actor::effect_t::TORMENT:
                                return audit::damage_event_t::damage_type_t::TORMENT_STATIONARY;
                            case actor::effect_t::POISON:
                                return audit::damage_event_t::damage_type_t::POISON;
                            case actor::effect_t::CONFUSION:
                                return audit::damage_event_t::damage_type_t::CONFUSION;
                            case actor::effect_t::BINDING_BLADE:
                                return audit::damage_event_t::damage_type_t::BINDING_BLADE;
                            default:
                                throw std::runtime_error("Unknown source for damage!");
                        }
                    } else if (!incoming_damage_event.skill.empty()) {
                        return audit::damage_event_t::damage_type_t::STRIKE;
                    } else {
                        throw std::runtime_error("Unknown source for damage!");
                    }
                }();
                auto skill_to_attribute_damage_to = [&]() {
                    if (incoming_damage_event.skill.empty()) {
                        return std::string{"unknown_skill"};
                    } else {
                        auto& skill_configuration =
                            utils::get_skill_configuration(incoming_damage_event.skill,
                                                           incoming_damage_event.source_entity,
                                                           registry);
                        return skill_configuration.attribute_damage_to_skill.empty()
                                   ? skill_configuration.skill_key
                                   : skill_configuration.attribute_damage_to_skill;
                    }
                }();
                audit_component.old_events.emplace_back(audit::old_damage_event_t{
                    .time_ms = incoming_damage_event.tick,
                    .actor = utils::get_entity_name(actor_entity, registry),
                    .source_actor =
                        utils::get_entity_name(incoming_damage_event.source_entity, registry),
                    .source_skill = skill_to_attribute_damage_to,
                    .damage_type = old_damage_type,
                    .damage = utils::round_to_nearest_even(incoming_damage_event.value),
                });
                audit_component.events.emplace_back(create_tick_event(
                    audit::damage_event_t{
                        .source_actor =
                            utils::get_entity_name(incoming_damage_event.source_entity, registry),
                        .source_skill = skill_to_attribute_damage_to,
                        .damage_type = damage_type,
                        .damage = utils::round_to_nearest_even(incoming_damage_event.value),
                    },
                    actor_entity,
                    registry));
            }
        });
}

void audit(registry_t& registry) {
    audit_actor_created(registry);
    audit_skill_casts(registry);
    audit_bundles(registry);
    audit_effect_applications(registry);
    audit_damage(registry);
    audit_combat_stats_update(registry);
    audit_effect_expiration(registry);
    audit_actor_downstate(registry);
}

audit::report_t get_audit_report(registry_t& registry) {
    audit::report_t audit_report;
    registry.view<component::audit_component>().each(
        [&](const component::audit_component& audit_component) {
            std::copy(audit_component.old_events.cbegin(),
                      audit_component.old_events.cend(),
                      std::back_inserter(audit_report.events));
            std::copy(audit_component.events.cbegin(),
                      audit_component.events.cend(),
                      std::back_inserter(audit_report.tick_events));
        });
    return audit_report;
}

}  // namespace gw2combat::system
