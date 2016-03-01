/*
 * StrongMaster.h
 *
 *  Created on: Aug 12, 2014
 *      Author: thardin
 */

#ifndef STRONGMASTER_H_
#define STRONGMASTER_H_

#include "WeakMasters.h"
#include "sc/Solver.h"

namespace fmitcp_master {
class WeakConnection;
class StrongMaster : public JacobiMaster {
    sc::Solver m_strongCouplingSolver;
    map<FMIClient*, vector<int> > clientWeakRefs;

    void getDirectionalDerivative(FMIClient *client, sc::Vec3 seedVec, std::vector<int> accelerationRefs, std::vector<int> forceRefs);
public:
    StrongMaster(std::vector<FMIClient*> slaves, std::vector<WeakConnection*> weakConnections, sc::Solver strongCouplingSolver);
    void prepare();
    void runIteration(double t, double dt);
};
}


#endif /* STRONGMASTER_H_ */
