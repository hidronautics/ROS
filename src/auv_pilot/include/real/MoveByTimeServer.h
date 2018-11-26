#ifndef AUV_PILOT_MOVEBYTIMESERVER_H
#define AUV_PILOT_MOVEBYTIMESERVER_H

#include <common/MoveActionServerBase.h>

class MoveByTimeServer : MoveActionServerBase {

protected:

    void executeCallback(const auv_common::MoveGoalConstPtr& goal);

public:

    MoveByTimeServer(const std::string& actionName);
    ~MoveByTimeServer() = default;

};

#endif //AUV_PILOT_MOVEBYTIMESERVER_H