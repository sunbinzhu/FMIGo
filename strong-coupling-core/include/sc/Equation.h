#ifndef EQUATION_H
#define EQUATION_H

#include "sc/Connector.h"
#include "sc/JacobianElement.h"
#include "stdio.h"

namespace sc {

/// Base class for equations. Constrains two instances of Connector.
class Equation {

private:
    Connector * m_connA;
    Connector * m_connB;
    JacobianElement m_G_A;
    JacobianElement m_G_B;
    double m_g;
    double m_relativeVelocity;

public:
    //index in system (row/column in S)
    int m_index;
    std::vector<Connector*> m_connectors;

    Equation();
    Equation(Connector*,Connector*);
    virtual ~Equation();

    void setDefault();
    void setConnectors(Connector *,Connector *);

    // for figuring out which jacobians we need
    bool m_isSpatial, m_isRotational;

    /// Get constraint violation, g
    double getViolation() const {
        return m_g;
    }
    void setViolation(double g);
    void setDefaultViolation();

    /// Get constraint velocity, G*W
    void setRelativeVelocity(double);

    double getVelocity() const {
        return  m_G_A.multiply(m_connA->m_velocity, m_connA->m_angularVelocity) +
                m_G_B.multiply(m_connB->m_velocity, m_connB->m_angularVelocity) + m_relativeVelocity;
    }

    double getFutureVelocity() const {
        return  m_G_A.multiply(m_connA->m_futureVelocity, m_connA->m_futureAngularVelocity) +
                m_G_B.multiply(m_connB->m_futureVelocity, m_connB->m_futureAngularVelocity);
    }

    void setG(  double,double,double,
                double,double,double,
                double,double,double,
                double,double,double);
    void setG(  const Vec3& spatialA,
                const Vec3& rotationalA,
                const Vec3& spatialB,
                const Vec3& rotationalB);

    bool haveOverlappingFMUs(Equation *other) const;
    JacobianElement& jacobianElementForConnector(Connector *conn) {
        if (conn == m_connA) {
            return m_G_A;
        } else if (conn == m_connB) {
            return m_G_B;
        } else {
            fprintf(stderr, "Attempted to jacobianElementForConnector() with connector which is not part of the Equation\n");
            exit(1);
        }
    }
};

}

#endif
