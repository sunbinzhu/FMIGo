export BUILD_DIR=$(pwd)/build
export FMUS_DIR=$(pwd)/umit-fmus
export PATH=$PATH:$BUILD_DIR/install/bin
export SERVER=fmi-tcp-server
export MASTER=fmi-tcp-master
export MPI_SERVER=fmi-mpi-server
export MPI_MASTER=fmi-mpi-master

if [[ "`uname`" = "Windows_NT" || "`uname`" = "MINGW64"* ]]
then
    export WIN=1
else
    export WIN=0
    #Produce core dumps
    ulimit -c unlimited
    ulimit -n 2048  #should be some value greater than 2*max(Nseq), probably a power of two too
fi