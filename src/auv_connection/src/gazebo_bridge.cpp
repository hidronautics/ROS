#include "ros/ros.h"

#include "geometry_msgs/Twist.h"
#include <auv_common/VelocityCmd.h>
#include <auv_common/DepthCmd.h>

static const std::string GAZEBO_VELOCITY_TOPIC = "/cmd_vel";

static const std::string VELOCITY_SERVICE = "velocity_service";

static const std::string DEPTH_SERVICE = "depth_service";

ros::Publisher velocityPublisher;

bool movementCallback(auv_common::VelocityCmd::Request& velocityRequest,
                       auv_common::VelocityCmd::Response& velocityResponse);

bool depthCallback(auv_common::DepthCmd::Request& depthRequest,
                      auv_common::DepthCmd::Response& depthResponse);


int main(int argc, char **argv)
{
    ros::init(argc, argv, "gazebo_bridge");
    ros::NodeHandle nodeHandle;

    velocityPublisher = nodeHandle.advertise<geometry_msgs::Twist>(GAZEBO_VELOCITY_TOPIC, 100);

    ros::ServiceServer velocity_srv = nodeHandle.advertiseService(VELOCITY_SERVICE, movementCallback);
    ros::ServiceServer depth_srv = nodeHandle.advertiseService(DEPTH_SERVICE, depthCallback);

    ros::spin();

    return 0;
}


bool movementCallback(auv_common::VelocityCmd::Request& velocityRequest,
                       auv_common::VelocityCmd::Response& velocityResponse) {
    ros::Rate pollRate(100);
    while (velocityPublisher.getNumSubscribers() == 0)
        pollRate.sleep();
    velocityPublisher.publish(velocityRequest.twist);

    velocityResponse.success.data = true;

    return true;
}

bool depthCallback(auv_common::DepthCmd::Request& depthRequest,
                   auv_common::DepthCmd::Response& depthResponse) {
    /* Currently not supported in simulation */
    depthResponse.success.data = true;
    return true;
}