#ifndef BOSS_ANUBREKHAN_H_
#define BOSS_ANUBREKHAN_H_

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellInfo.h"
#include "naxxramas.h"

namespace Anubrekhan {
    
enum AnubrekhanSays
{
    ANUBREKHAN_SAY_AGGRO                       = 0,
    ANUBREKHAN_SAY_GREET                       = 1,
    ANUBREKHAN_SAY_SLAY                        = 2,
    ANUBREKHAN_EMOTE_LOCUST                    = 3
};

enum AnubrekhanGuardSays
{
    ANUBREKHAN_EMOTE_SPAWN                     = 1,
    ANUBREKHAN_EMOTE_SCARAB                    = 2
};

enum AnubrekhanSpells
{
    ANUBREKHAN_SPELL_IMPALE_10                 = 28783,
    ANUBREKHAN_SPELL_IMPALE_25                 = 56090,
    ANUBREKHAN_SPELL_LOCUST_SWARM_10           = 28785,
    ANUBREKHAN_SPELL_LOCUST_SWARM_25           = 54021,
    ANUBREKHAN_SPELL_SUMMON_CORPSE_SCRABS_5    = 29105,
    ANUBREKHAN_SPELL_SUMMON_CORPSE_SCRABS_10   = 28864,
    ANUBREKHAN_SPELL_BERSERK                   = 26662
};

enum AnubrekhanEvents
{
    ANUBREKHAN_EVENT_IMPALE                    = 1,
    ANUBREKHAN_EVENT_LOCUST_SWARM              = 2,
    ANUBREKHAN_EVENT_BERSERK                   = 3,
    ANUBREKHAN_EVENT_SPAWN_GUARD               = 4
};

enum AnubrekhanMisc
{
    ANUBREKHAN_NPC_CORPSE_SCARAB               = 16698,
    ANUBREKHAN_NPC_CRYPT_GUARD                 = 16573,

    ANUBREKHAN_ACHIEV_TIMED_START_EVENT        = 9891
};

class boss_anubrekhan : public CreatureScript
{
public:
    boss_anubrekhan() : CreatureScript("boss_anubrekhan") { }

    CreatureAI* GetAI(Creature* pCreature) const override
    {
        return GetNaxxramasAI<boss_anubrekhanAI>(pCreature);
    }

    struct boss_anubrekhanAI : public BossAI
    {
        explicit boss_anubrekhanAI(Creature* c) : BossAI(c, BOSS_ANUB), summons(me)
        {
            pInstance = c->GetInstanceScript();
            sayGreet = false;
        }

        InstanceScript* pInstance;
        EventMap events;
        SummonList summons;
        bool sayGreet;

        void SummonCryptGuards()
        {
            if (Is25ManRaid())
            {
                me->SummonCreature(ANUBREKHAN_NPC_CRYPT_GUARD, 3299.732f, -3502.489f, 287.077f, 2.378f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
                me->SummonCreature(ANUBREKHAN_NPC_CRYPT_GUARD, 3299.086f, -3450.929f, 287.077f, 3.999f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
            }
        }

        void Reset() override
        {
            BossAI::Reset();
            events.Reset();
            summons.DespawnAll();
            SummonCryptGuards();
            if (pInstance)
            {
                if (GameObject* go = me->GetMap()->GetGameObject(pInstance->GetGuidData(DATA_ANUB_GATE)))
                {
                    go->SetGoState(GO_STATE_ACTIVE);
                }
            }
        }

        void JustSummoned(Creature* cr) override
        {
            if (me->IsInCombat())
            {
                cr->SetInCombatWithZone();
                if (cr->GetEntry() == ANUBREKHAN_NPC_CRYPT_GUARD)
                {
                    cr->AI()->Talk(ANUBREKHAN_EMOTE_SPAWN, me);
                }
            }
            summons.Summon(cr);
        }

        void SummonedCreatureDies(Creature* cr, Unit*) override
        {
            if (cr->GetEntry() == ANUBREKHAN_NPC_CRYPT_GUARD)
            {
                cr->CastSpell(cr, ANUBREKHAN_SPELL_SUMMON_CORPSE_SCRABS_10, true, nullptr, nullptr, me->GetGUID());
                cr->AI()->Talk(ANUBREKHAN_EMOTE_SCARAB);
            }
        }

        void SummonedCreatureDespawn(Creature* cr) override
        {
            summons.Despawn(cr);
        }

        void JustDied(Unit*  killer) override
        {
            BossAI::JustDied(killer);
            summons.DespawnAll();
            if (pInstance)
            {
                pInstance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ANUBREKHAN_ACHIEV_TIMED_START_EVENT);
            }
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() != TYPEID_PLAYER)
                return;

            Talk(ANUBREKHAN_SAY_SLAY);
            victim->CastSpell(victim, ANUBREKHAN_SPELL_SUMMON_CORPSE_SCRABS_5, true, nullptr, nullptr, me->GetGUID());
            if (pInstance)
            {
                pInstance->SetData(DATA_IMMORTAL_FAIL, 0);
            }
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            me->CallForHelp(30.0f);
            Talk(ANUBREKHAN_SAY_AGGRO);
            if (pInstance)
            {
                if (GameObject* go = me->GetMap()->GetGameObject(pInstance->GetGuidData(DATA_ANUB_GATE)))
                {
                    go->SetGoState(GO_STATE_READY);
                }
            }
            events.ScheduleEvent(ANUBREKHAN_EVENT_IMPALE, 15s);
            events.ScheduleEvent(ANUBREKHAN_EVENT_LOCUST_SWARM, 70s, 120s);
            events.ScheduleEvent(ANUBREKHAN_EVENT_BERSERK, 10min);
            if (!summons.HasEntry(ANUBREKHAN_NPC_CRYPT_GUARD))
            {
                SummonCryptGuards();
            }
            if (!Is25ManRaid())
            {
                events.ScheduleEvent(ANUBREKHAN_EVENT_SPAWN_GUARD, 15s, 20s);
            }
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!sayGreet && who->GetTypeId() == TYPEID_PLAYER)
            {
                Talk(ANUBREKHAN_SAY_GREET);
                sayGreet = true;
            }
            ScriptedAI::MoveInLineOfSight(who);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!me->IsInCombat() && sayGreet)
            {
                for (SummonList::iterator itr = summons.begin(); itr != summons.end(); ++itr)
                {
                    if (pInstance)
                    {
                        if (Creature* cr = pInstance->instance->GetCreature(*itr))
                        {
                            if (cr->IsInCombat())
                                DoZoneInCombat();
                        }
                    }
                }
            }

            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case ANUBREKHAN_EVENT_IMPALE:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0))
                    {
                        me->CastSpell(target, RAID_MODE(ANUBREKHAN_SPELL_IMPALE_10, ANUBREKHAN_SPELL_IMPALE_25), false);
                    }
                    events.Repeat(20s);
                    break;
                case ANUBREKHAN_EVENT_LOCUST_SWARM:
                    Talk(ANUBREKHAN_EMOTE_LOCUST);
                    me->CastSpell(me, RAID_MODE(ANUBREKHAN_SPELL_LOCUST_SWARM_10, ANUBREKHAN_SPELL_LOCUST_SWARM_25), false);
                    events.ScheduleEvent(ANUBREKHAN_EVENT_SPAWN_GUARD, 3s);
                    events.Repeat(90s);
                    break;
                case ANUBREKHAN_EVENT_SPAWN_GUARD:
                    me->SummonCreature(ANUBREKHAN_NPC_CRYPT_GUARD, 3331.217f, -3476.607f, 287.074f, 3.269f, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60000);
                    break;
                case ANUBREKHAN_EVENT_BERSERK:
                    me->CastSpell(me, ANUBREKHAN_SPELL_BERSERK, true);
                    break;
            }
            DoMeleeAttackIfReady();
        }
    };
};

}

#endif