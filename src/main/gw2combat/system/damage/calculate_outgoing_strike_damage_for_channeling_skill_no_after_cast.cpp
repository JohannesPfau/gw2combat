#include "gw2combat/system/system.hpp"

#include "gw2combat/component/animation/animation.hpp"
#include "gw2combat/component/boon/quickness.hpp"
#include "gw2combat/component/damage/multipliers/outgoing_strike_damage_multiplier.hpp"
#include "gw2combat/component/damage/outgoing_strike_damage.hpp"
#include "gw2combat/component/skills/channeling_skill.hpp"

namespace gw2combat::system {

void calculate_outgoing_strike_damage_for_channeling_skill_no_after_cast(context& ctx) {
    ctx.registry
        .view<component::outgoing_strike_damage_multiplier,
              component::animation,
              component::channeling_skill>()
        .each([&](const entt::entity entity,
                  const component::outgoing_strike_damage_multiplier&
                      outgoing_strike_damage_multiplier,
                  const component::animation& animation,
                  component::channeling_skill& channeling_skill) {
            skills::skill skill = channeling_skill.skill;
            assert(skill.type == skills::skill::type::CHANNELING_NO_AFTER_CAST);

            bool has_quickness = ctx.registry.any_of<component::quickness>(entity);
            tick_t skill_hit_rate = animation.skill.cast_duration[has_quickness] / skill.hits;
            bool skill_hits_this_tick =
                ctx.current_tick == 0 ||
                ctx.current_tick >= channeling_skill.last_hit_tick + skill_hit_rate;
            if (skill_hits_this_tick) {
                channeling_skill.last_hit_tick = ctx.current_tick;

                auto& outgoing_strike_damage =
                    ctx.registry.get_or_emplace<component::outgoing_strike_damage>(entity);
                outgoing_strike_damage.strikes.emplace_back(strike{
                    entity,
                    outgoing_strike_damage_multiplier.multiplier * skill.damage_coefficient});
            }
        });
}

}  // namespace gw2combat::system