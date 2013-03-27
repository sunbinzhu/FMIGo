#ifndef JACOBI_H
#define JACOBI_H

// Takes a Jacobi co-simulation step
int jacobiStep( double time,
                double communicationTimeStep,
                int numFMUs,
                fmi1_import_t ** fmus,
                int numConnections,
                connection connections[MAX_CONNECTIONS]);

#endif /* JACOBI_H */