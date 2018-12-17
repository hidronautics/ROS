
#include <MoveActionServerBase.h>

#include "../include/MoveActionServerBase.h"

MoveActionServerBase::MoveActionServerBase(const std::string &actionName, const std::string velocityTopic):
        actionServer(nodeHandle, actionName, boost::bind(&MoveActionServerBase::goalCallback, this, _1), false),
        velocityPublishTopic(velocityTopic) {
    velocityPublisher = nodeHandle.advertise<geometry_msgs::Twist>(velocityTopic, VELOCITY_TOPIC_QUEUE_SIZE);
    actionServer.start();
}

geometry_msgs::Twist MoveActionServerBase::createTwist(float x, float y, float z, float roll, float pitch, float yaw) {
    geometry_msgs::Twist twist;
    twist.linear.x = x;
    twist.linear.y = y;
    twist.linear.z = z;
    twist.angular.x = roll;
    twist.angular.y = pitch;
    twist.angular.z = yaw;
    return twist;
}

geometry_msgs::Twist MoveActionServerBase::createLinearTwist(float x, float y, float z) {
    return createTwist(x, y, z, 0.0f, 0.0f, 0.0f);
}

geometry_msgs::Twist MoveActionServerBase::createAngularTwist(float roll, float pitch, float yaw) {
    return createTwist(0.0f, 0.0f, 0.0f, roll, pitch, yaw);
}

geometry_msgs::Twist MoveActionServerBase::createRotationTwist(int direction, float velocity) {
    switch (direction) {
        case auv_common::RotateGoal::DIRECTION_ROLL:
            return createAngularTwist(velocity, 0.0f, 0.0f);
        case auv_common::RotateGoal::DIRECTION_PITCH:
            return createAngularTwist(0.0f, velocity, 0.0f);
        case auv_common::RotateGoal::DIRECTION_YAW:
            return createAngularTwist(0.0f, 0.0f, velocity);
        default:
            return createAngularTwist(0.0f, 0.0f, 0.0f);
    }
}

geometry_msgs::Twist MoveActionServerBase::createDirectionTwist(int direction, float velocity) {
    switch (direction) {
        case auv_common::MoveGoal::DIRECTION_FORWARD:
            return createLinearTwist(velocity, 0.0f, 0.0f); // Workaraound for current model
        case auv_common::MoveGoal::DIRECTION_BACKWARDS:
            return createLinearTwist(-velocity, 0.0f, 0.0f);
        case auv_common::MoveGoal::DIRECTION_RIGHT:
            return createLinearTwist(0, velocity, 0.0f);
        case auv_common::MoveGoal::DIRECTION_LEFT:
            return createLinearTwist(0, -velocity, 0.0f);
        default:
            return createLinearTwist(0.0f, 0.0f, 0.0f);
    }
}

void MoveActionServerBase::safePublish(const geometry_msgs::Twist& twist) {
    ros::Rate pollRate(VELOCITY_TOPIC_POLL_RATE);
    while (velocityPublisher.getNumSubscribers() == 0)
        pollRate.sleep();
    velocityPublisher.publish(twist);
}