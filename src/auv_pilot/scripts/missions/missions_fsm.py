#! /usr/bin/env python

import rospy
import smach
import smach_ros
from common import common_states
import gate_mission
from drums_mission import drums_mission_fsm
from auv_common.msg import DropperAction, DropperGoal

def create_missions_fsm():

    if rospy.has_param('/gateFsmMode'):
        gate_fsm_mode = rospy.get_param('/gateFsmMode')
    else:
        if not rospy.has_param('~gateFsmMode'):
            rospy.logerr('Gate FSM mode not specified, available modes: timings, simple, vision')
            raise
        gate_fsm_mode = rospy.get_param('~gateFsmMode')
    rospy.loginfo('Gate FSM mode: ' + gate_fsm_mode)

    if rospy.has_param('/drumsEnabled'):
        drums_enabled = bool(rospy.get_param('/drumsEnabled'))
    else:
        drums_enabled = rospy.get_param('~drumsEnabled', False)
    rospy.loginfo('Drums enabled: ' + str(drums_enabled))


    sm = smach.StateMachine(outcomes=['MISSIONS_OK', 'MISSIONS_FAILED'])

    with sm:
        if gate_fsm_mode == 'timings' and not drums_enabled:
            smach.StateMachine.add('GATE_MISSION', gate_mission.create_gate_fsm_by_timings(), transitions={'GATE_OK': 'MISSIONS_OK', 'GATE_FAILED': 'MISSIONS_FAILED'})
        elif gate_fsm_mode == 'simple' and not drums_enabled:
            smach.StateMachine.add('GATE_MISSION', gate_mission.create_gate_fsm_centering_simple(), transitions={'GATE_OK': 'MISSIONS_OK', 'GATE_FAILED': 'MISSIONS_FAILED'})
        elif gate_fsm_mode == 'vision' and not drums_enabled:
            smach.StateMachine.add('GATE_MISSION', gate_mission.create_gate_fsm_centering_vision(), transitions={'GATE_OK': 'MISSIONS_OK', 'GATE_FAILED': 'MISSIONS_FAILED'})

        # TODO THIS IS ONLY FOR DEBUGGING
        if drums_enabled:
            if gate_fsm_mode == 'timings':
                smach.StateMachine.add('GATE_MISSION', gate_mission.create_gate_fsm_by_timings(), transitions={'GATE_OK': 'DRUMS_MISSION', 'GATE_FAILED': 'MISSIONS_FAILED'})
            elif gate_fsm_mode == 'simple':
                smach.StateMachine.add('GATE_MISSION', gate_mission.create_gate_fsm_centering_simple(), transitions={'GATE_OK': 'DRUMS_MISSION', 'GATE_FAILED': 'MISSIONS_FAILED'})
            elif gate_fsm_mode == 'vision':
                smach.StateMachine.add('GATE_MISSION', gate_mission.create_gate_fsm_centering_vision(), transitions={'GATE_OK': 'DRUMS_MISSION', 'GATE_FAILED': 'MISSIONS_FAILED'})

            smach.StateMachine.add('DRUMS_MISSION', drums_mission_fsm.create_drums_fsm(), transitions={'DRUMS_OK': 'BALL_DROP', 'DRUMS_FAILED': 'MISSIONS_FAILED'})

            smach.StateMachine.add('BALL_DROP', smach_ros.SimpleActionState('dropper', DropperAction, goal=DropperGoal()),
                                    {'succeeded':'MISSIONS_OK', 'preempted':'MISSIONS_FAILED', 'aborted':'MISSIONS_FAILED'})

    return sm

