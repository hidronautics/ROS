#ifndef AUV_PILOT_REALTWISTFACTORY_H
#define AUV_PILOT_REALTWISTFACTORY_H

#include "TwistFactory.h"


/**
 * Twist factory for real apparatus
 */
class RealTwistFactory : public TwistFactory {

public:

    RealTwistFactory(float defaultVelocity);
    RealTwistFactory(TwistFactory& other);
    ~RealTwistFactory() = default;

    geometry_msgs::Twist createDirectionTwist(int direction, float velocity);
    geometry_msgs::Twist createDirectionTwist(int direction);

};


#endif //AUV_PILOT_REALTWISTFACTORY_H
