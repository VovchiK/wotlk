/*
* Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU General Public License as published by the
* Free Software Foundation; either version 2 of the License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
* FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
* more details.
*
* You should have received a copy of the GNU General Public License along
* with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "ScriptPCH.h"
#include "ruby_sanctum.h"

/*
UPDATE `creature_template` SET `ScriptName`= 'boss_halion' WHERE `entry`=39863;
UPDATE `creature_template` SET `ScriptName`= 'npc_halion_twilight' WHERE `entry`=40142;
UPDATE `creature_template` SET `ScriptName`= 'npc_halion_controller' WHERE `entry`=40146;
DELETE FROM `creature` WHERE `id`=40146;
DELETE FROM `creature_text` WHERE `entry`=39863;
INSERT INTO `creature_text` (`entry`,`groupid`,`id`,`text`,`type`,`language`,`probability`,`emote`,`duration`,`sound`,`comment`) VALUES
(39863,0,0, 'Meddlesome insects! You are too late. The Ruby Sanctum is lost!',14,0,100,1,0,17499, 'Halion');
*/

enum Texts
{
    SAY_INTRO = 0, // Meddlesome insects! You are too late. The Ruby Sanctum is lost! (17499, 1)
};

enum Spells
{
    // Halion
    SPELL_FLAME_BREATH = 74525,
    SPELL_CLEAVE = 74524,
    // Halion Controller
    SPELL_COSMETIC_FIRE_PILLAR = 76006,
    SPELL_FIERY_EXPLOSION = 76010,
};

enum Events
{
    // Halion
    EVENT_ACTIVATE_FIREWALL = 1,
    EVENT_CLEAVE = 2,
    EVENT_TAIL_LASH = 3,
    EVENT_FLAME_BREATH = 4,
    EVENT_METEOR_STRIKE = 5,
    EVENT_FIERY_COMBUSTION = 6,
    // Halion Controller
    EVENT_START_INTRO = 7,
    EVENT_INTRO_PROGRESS_1 = 8,
    EVENT_INTRO_PROGRESS_2 = 9,
    EVENT_INTRO_PROGRESS_3 = 10,
};

enum Phases
{
    PHASE_ALL = 0,
    PHASE_ONE = 1,
    PHASE_TWO = 2,
    PHASE_THREE = 3,

    PHASE_ONE_MASK = 1 << PHASE_ONE,
    PHASE_TWO_MASK = 1 << PHASE_TWO,
    PHASE_THREE_MASK = 1 << PHASE_THREE,
};

Position const HalionSpawnPos = {3156.67f, 533.8108f, 72.98822f, 3.159046f};

class boss_halion : public CreatureScript
{
    public:
        boss_halion() : CreatureScript("boss_halion") { }

        struct boss_halionAI : public BossAI
        {
            boss_halionAI(Creature* creature) : BossAI(creature, DATA_HALION)
            {
            }

            void Reset()
            {
                _Reset();
                events.SetPhase(PHASE_ONE);
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_ACTIVATE_FIREWALL, 10000);
                events.ScheduleEvent(EVENT_CLEAVE, urand(8000, 10000));
                events.ScheduleEvent(EVENT_TAIL_LASH, urand(12000, 15000));
                events.ScheduleEvent(EVENT_FLAME_BREATH, urand(10000, 12000));
                events.ScheduleEvent(EVENT_METEOR_STRIKE, urand(20000, 25000));
                events.ScheduleEvent(EVENT_FIERY_COMBUSTION, urand(15000, 18000));

                //events.ScheduleEvent(EVENT_FLAME_BREATH,urand(19000, 28000)*IN_MILLISECONDS, 0, phase);
                //events.ScheduleEvent(EVENT_CLEAVE,urand(8000, 10000)*IN_MILLISECONDS, 0, phase);
                //events.ScheduleEvent(EVENT_TAIL_LASH,urand(12000, 15000)*IN_MILLISECONDS, 0, phase);
                //events.ScheduleEvent(EVENT_METEOR_STRIKE,urand(40000, 45000)*IN_MILLISECONDS,0,phase);
                //events.ScheduleEvent(EVENT_FIERY_COMBUSTION,urand(25000, 28000)*IN_MILLISECONDS,0,phase);
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void UpdateAI(uint32 const diff)
            {
                if ((events.GetPhaseMask() & PHASE_ONE_MASK) && !UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_FLAME_BREATH:
                            DoCast(me, SPELL_FLAME_BREATH);
                            break;
                        case EVENT_CLEAVE:
                            DoCastVictim(SPELL_CLEAVE);
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetRubySanctumAI<boss_halionAI>(creature);
        }
};

class npc_halion_controller : public CreatureScript
{
    public:
        npc_halion_controller() : CreatureScript("npc_halion_controller") { }

        struct npc_halion_controllerAI : public ScriptedAI
        {
            npc_halion_controllerAI(Creature* creature) : ScriptedAI(creature),
                _instance(creature->GetInstanceScript())
            {
                me->SetPhaseMask(me->GetPhaseMask() | 0x20, true);
            }

            void Reset()
            {
                _events.Reset();
                me->SetReactState(REACT_PASSIVE);
            }

            void DoAction(int32 const action)
            {
                if (action == ACTION_INTRO_HALION)
                {
                    me->setActive(true);
                    _events.ScheduleEvent(EVENT_START_INTRO, 2000);
                    _events.ScheduleEvent(EVENT_INTRO_PROGRESS_1, 6000);
                    _events.ScheduleEvent(EVENT_INTRO_PROGRESS_2, 10000);
                    _events.ScheduleEvent(EVENT_INTRO_PROGRESS_3, 14000);
                }
            }
            void UpdateAI(uint32 const diff)
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_START_INTRO:
                            DoCast(me, SPELL_COSMETIC_FIRE_PILLAR, true);
                            break;
                        case EVENT_INTRO_PROGRESS_1:
                            for (uint8 i = 2; i < 4; ++i)
                                if (GameObject* tree = ObjectAccessor::GetGameObject(*me, _instance->GetData64(DATA_BURNING_TREE_1 + i)))
                                    _instance->HandleGameObject(_instance->GetData64(DATA_BURNING_TREE_1 + i), true, tree);
                            break;
                        case EVENT_INTRO_PROGRESS_2:
                            for (uint8 i = 0; i < 2; ++i)
                                if (GameObject* tree = ObjectAccessor::GetGameObject(*me, _instance->GetData64(DATA_BURNING_TREE_1 + i)))
                                    _instance->HandleGameObject(_instance->GetData64(DATA_BURNING_TREE_1 + i), true, tree);
                            break;
                        case EVENT_INTRO_PROGRESS_3:
                            DoCast(me, SPELL_FIERY_EXPLOSION);
                            if (Creature* halion = me->SummonCreature(NPC_HALION, HalionSpawnPos))
                                halion->AI()->Talk(SAY_INTRO);
                            me->setActive(false);
                            break;
                        default:
                            break;
                    }
                }
            }

        private:
            EventMap _events;
            InstanceScript* _instance;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_halion_controllerAI(creature);
        }
};

void AddSC_boss_halion()
{
    new boss_halion();
    new npc_halion_controller();
}