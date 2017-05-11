#include <iostream>
#include <stdio.h>
#ifdef WIN32
#include "master/getopt.h"
#else
#include <getopt.h>
#endif
#include <deque>
#include <fstream>
#include "common/common.h"
#ifdef USE_MPI
#include <mpi.h>
#endif
#include <sstream>
#include "common/CSV-parser.h"

#include "master/parseargs.h"

using namespace fmitcp_master;
using namespace common;
using namespace std;

static void printHelp(){
  info("Check manpage for help, \"man fmigo-master\"\n");
}

static void printInvalidArg(char option){
  error("Invalid argument of -%c\n",option);
  printHelp();
}

fmi2_base_type_enu_t fmitcp_master::type_from_char(string type) {
    if (type.size() != 1) {
        fatal("Bad type: %s\n", type.c_str());
    }

    switch (type[0]) {
    default:
    case 'r': return fmi2_base_type_real;
    case 'i': return fmi2_base_type_int;
    case 'b': return fmi2_base_type_bool;
    case 's': return fmi2_base_type_str;
    }
}

template<typename T> void checkFMUIndex(T it, int i, size_t numFMUs) {
    if(it->fromFMU < 0 || (size_t)it->fromFMU >= numFMUs){
        fatal("Connection %d connects from FMU %d, which does not exist.\n", i, it->fromFMU);
    }
    if(it->toFMU < 0 || (size_t)it->toFMU >= numFMUs){
        fatal("Connection %d connects to FMU %d, which does not exist.\n", i, it->toFMU);
    }
}

static vector<char*> make_char_vector(vector<string>& vec) {
    vector<char*> ret;
    for (size_t x = 0; x < vec.size(); x++) {
        ret.push_back((char*)vec[x].c_str());
    }
    return ret;
}

//template to handle not being able to use ifstream and istream at the same time
//maybe there's a better way, but this works
template<typename T> void add_args_internal(T& ifs, vector<string>& argvstore, vector<char*>& argv2, int position) {
    string token;

    //read tokens, insert into argvstore/argv2
    while (ifs >> token) {
        if (token == "-a") {
            fatal("Found -a token in argument file, which might lead to recursive list of arguments. Stopping.\n");
        }

        argvstore.insert(argvstore.begin() + position, token);
        position++;
    }

    argv2 = make_char_vector(argvstore);
}

static void add_args(vector<string>& argvstore, vector<char*>& argv2, string filename, int position) {
    if (filename == "-") {
        add_args_internal(std::cin, argvstore, argv2, position);
    } else {
        ifstream ifs(filename.c_str());
        if (!ifs) {
            fatal("Couldn't open %s to parse more arguments!\n", filename.c_str());
        }
        add_args_internal(ifs, argvstore, argv2, position);
    }
}

//split a delim separated string, with backslash escaping
//delim=':' example: "s,0,0,C\:\\foo\\bar:s,0,1,D\:\\bluh\\" -> "s,0,0,C:\foo\bar", "s,0,1,D:\bluh\"
//trailing single backslashes will be pruned ("...\" -> "...")
static deque<string> escapeSplit(string str, char delim) {
  deque<string> ret;
  ostringstream oss;
  bool escaped = false;

  for (char c : str) {
    if (escaped) {
      if (c != ',' && c != ':' && c != '\\') {
        fatal("Only comma, colon and backslash (\",:\\\") may be escaped in program options (\"%s\")\n", str.c_str());
      }
      oss << c;
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == delim) {
      ret.push_back(oss.str());
      oss.str("");
      oss.clear();
    } else {
      oss << c;
    }
  }

  if (escaped) {
    fatal("Trailing backslash in program option (\"%s\")\n", str.c_str());
  }

  //push remaining string
  ret.push_back(oss.str());
  return ret;
}

void fmitcp_master::parseArguments( int argc,
                    char *argv[],
                    std::vector<std::string> *fmuFilePaths,
                    std::vector<connection> *connections,
                    std::vector<std::deque<std::string> > *params,
                    double* tEnd,
                    double* timeStepSize,
                    jm_log_level_enu_t *loglevel,
                    char* csv_separator,
                    std::string *outFilePath,
                    int* quietMode,
                    enum FILEFORMAT * fileFormat,
                    enum METHOD * method,
                    int * realtimeMode,
                    int * printXML,
                    std::vector<int> *stepOrder,
                    std::vector<int> *fmuVisibilities,
                    vector<strongconnection> *strongConnections,
                    string *hdf5Filename,
                    string *fieldnameFilename,
                    bool *holonomic,
                    double *compliance,
                    int *command_port,
                    int *results_port,
                    bool *paused,
                    bool *solveLoops,
                    bool *useHeadersInCSV,
                    fmigo_csv_fmu *csv_fmu
        ) {
    int index, c;
    opterr = 0;

#ifdef USE_MPI
    //world = master at 0, FMUs at 1..N
    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
#endif

    //repack argv into argvstore, to which the entries in argv2 point
    vector<string> argvstore;

    for (int x = 0; x < argc; x++) {
        argvstore.push_back(string(argv[x]));
    }

    vector<char*> argv2 = make_char_vector(argvstore);

    while ((c = getopt (argv2.size(), argv2.data(), "xrl:vqht:c:d:s:o:p:f:m:g:w:C:5:F:NM:a:z:ZLHV:")) != -1){
        int n, skip, l, cont, i, numScanned, stop, vis;
        deque<string> parts;
        if (optarg) parts = escapeSplit(optarg, ':');

        switch (c) {

        case 'c':
            for (auto it = parts.begin(); it != parts.end(); it++) {
                connection conn;
                deque<string> values = escapeSplit(*it, ',');
                conn.slope = 1;
                conn.intercept = 0;
                conn.needs_type = true;
                int a = 0, b = 1, c = 2, d = 3; //positions of FMUFROM,VRFROM,FMUTO,VRTO in values

                if (values.size() == 8) {
                    //TYPEFROM,FMUFROM,VRFROM,TYPETO,FMUTO,VRTO,k,m
                    if (!isNumeric(values[2]) || !isNumeric(values[5])) {
                        fatal("TYPEFROM,FMUFROM,NAMEFROM,TYPETO,FMUTO,NAMETO,k,m syntax not allowed\n");
                    }
                    conn.needs_type = false;
                    conn.fromType = type_from_char(values[0]);
                    conn.toType   = type_from_char(values[3]);
                    conn.slope    = atof(values[6].c_str());
                    conn.intercept= atof(values[7].c_str());
                    a = 1; b = 2;  c = 4; d = 5;
                } else if (values.size() == 6) {
                    //TYPEFROM,FMUFROM,VRFROM,TYPETO,FMUTO,VRTO
                    //FMUFROM,NAMEFROM,FMUTO,NAMETO,k,m
                    if (isNumeric(values[1])) {
                        conn.needs_type = false;
                        conn.fromType = type_from_char(values[0]);
                        conn.toType   = type_from_char(values[3]);
                        a = 1; b = 2;  c = 4; d = 5;
                    } else {
                        conn.slope    = atof(values[4].c_str());
                        conn.intercept= atof(values[5].c_str());
                    }
                } else  if (values.size() == 5) {
                    //TYPE,FMUFROM,VRFROM,FMUTO,VRTO
                    //TYPE,FMUFROM,NAMEFROM,FMUTO,NAMETO (undocumented, not recommended)
                    if (!isNumeric(values[1]) || !isNumeric(values[4])) {
                        warning("TYPE,FMUFROM,NAMEFROM,FMUTO,NAMETO syntax not recommended\n");
                    }
                    conn.needs_type = false;
                    conn.fromType = conn.toType = type_from_char(values[0]);
                    values.pop_front();
                } else if (values.size() == 4) {
                    //FMUFROM,VRFROM,FMUTO,VRTO
                    //FMUFROM,NAMEFROM,FMUTO,NAMETO
                    if (isNumeric(values[1]) != isNumeric(values[3])) {
                        fatal("Must specify VRs or names, not both (-c %s,%s,%s,%s)\n",
                            values[0].c_str(), values[1].c_str(), values[2].c_str(), values[3].c_str()
                        );
                    }

                    //assume real if VRs, request type if names
                    conn.fromType = conn.toType = type_from_char("r");
                    conn.needs_type = !isNumeric(values[1]);
                } else {
                    fatal("Bad param: %s\n", it->c_str());
                }

                conn.fromFMU      = atoi(values[a].c_str());
                conn.toFMU        = atoi(values[c].c_str());
                conn.fromOutputVRorNAME = values[b];
                conn.toInputVRorNAME    = values[d];

                connections->push_back(conn);
            }
            break;

        case 'C':
            //strong connections
            for (auto it = parts.begin(); it != parts.end(); it++) {
                deque<string> values = escapeSplit(*it, ',');
                strongconnection sc;

                if (values.size() < 3) {
                    fatal("Bad strong connection specification: %s\n", it->c_str());
                }

                sc.type    = values[0];
                sc.fromFMU = atoi(values[1].c_str());
                sc.toFMU   = atoi(values[2].c_str());

                for (auto it2 = values.begin() + 3; it2 != values.end(); it2++) {
                    sc.vrORname.push_back(it2->c_str());
                }

                strongConnections->push_back(sc);
            }
            break;

        case 'd':
            numScanned = sscanf(optarg,"%lf", timeStepSize);
            if(numScanned <= 0){
                printInvalidArg(c);
                exit(1);
            }
            break;

        case 'f':
            if(strcmp(optarg,"csv") == 0){
                *fileFormat = csv;
            } else if( strcmp(optarg,"tikz") == 0){
                *fileFormat = tikz;
            } else if (!strcmp(optarg, "none")) {
                *fileFormat = none;
            } else {
                fatal("File format \"%s\" not recognized.\n",optarg);
            }
            break;

        case 'l':
            *loglevel = logOptionToJMLogLevel(optarg);
            break;

        case 'm':
            if(strcmp(optarg,"jacobi") == 0){
                *method = jacobi;
            } else if(strcmp(optarg,"gs") == 0){
                *method = gs;
            } else if(strcmp(optarg,"me") == 0){
                *method = me;
            } else {
                fatal("Method \"%s\" not recognized. Use \"jacobi\" or \"gs\".\n",optarg);
            }
            break;

        case 't':
            numScanned = sscanf(optarg, "%lf", tEnd);
            if(numScanned <= 0){
                printInvalidArg(c);
                exit(1);
            }
            break;

        case 'g':
            // Step order spec
            n=0;
            skip=0;
            l=strlen(optarg);
            cont=1;
            i=0;
            int scannedInt;
            stop = 2;
            while(cont && (n=sscanf(&optarg[skip],"%d", &scannedInt))!=-1 && skip<l){
                // Now skip everything before the n'th comma
                char* pos = strchr(&optarg[skip],',');
                if(pos==NULL){
                    stop--;
                    if(stop == 0){
                        cont=0;
                        break;
                    }
                    stepOrder->push_back(scannedInt);
                    break;
                }
                stepOrder->push_back(scannedInt);
                skip += pos-&optarg[skip]+1; // Dunno why this works... See http://www.cplusplus.com/reference/cstring/strchr/
                i++;
            }
            break;

        case 'h':
            printHelp();
            exit(1);

        case 'r':
            *realtimeMode = 1;
            break;

        case 's':
            if(strlen(optarg)==1 && isprint(optarg[0])){
                *csv_separator = optarg[0];
            } else {
                printInvalidArg('s');
                exit(1);
            }
            break;

        case 'o':
            *outFilePath = optarg;
            break;

        case 'q':
            *quietMode = 1;
            break;

        case 'v':
            printf("%s\n",FMITCPMASTER_VERSION);
            exit(1);

        case 'p':
            for (auto it = parts.begin(); it != parts.end(); it++) {
                deque<string> values = escapeSplit(*it, ',');

                //expect [type,]FMU,VR,value
                if (values.size() < 3 || values.size() > 4) {
                    fatal("Parameters must have exactly 3 or 4 parts: %s\n", it->c_str());
                }

                params->push_back(values);
            }
            break;

        case 'x':
            *printXML = 1;
            break;

        case 'w':
            //visibilities
            for (auto it = parts.begin(); it != parts.end(); it++) {
                fmuVisibilities->push_back(atoi(it->c_str()));
            }
            break;

        case '5':
            *hdf5Filename = optarg;
            break;

        case 'H':
            *useHeadersInCSV = true;
            break;

        case 'F':
            warning("-F option is deprecated and will be removed soon\n");
            *fieldnameFilename = optarg;
            break;

        case 'N':
            *holonomic = false;
            break;

        case 'M':
            *compliance = atof(optarg);
            break;

        case 'a':
#ifdef USE_MPI
            if (world_rank == 0) {
                //only chomp stdin on the master node
                add_args(argvstore, argv2, optarg, optind);
            }
#else
            add_args(argvstore, argv2, optarg, optind);
#endif
            break;

        case 'z':
            if (parts.size() != 2) {
                fatal("-z must have exactly two parts (got %s which has %li parts)\n", optarg, parts.size());
            }
            //using ZMQ output disabling printing by default, unless the user follows -z with -f csv or -f tikz
            *fileFormat = none;
            *command_port = atoi(parts[0].c_str());
            *results_port = atoi(parts[1].c_str());
            break;

        case 'Z':
            info("Starting master in paused state\n");
            *paused = true;
            break;

        case 'L':
            *solveLoops = true;
            break;

        case '?':

            if(isprint(optopt)){
                if(strchr("cdsopfm", optopt)){
                    fatal("Option -%c requires an argument.\n", optopt);
                } else {
                    fatal("Unknown option: -%c\n", optopt);
                }
            } else {
                fatal("Unknown option character: \\x%x\n", optopt);
            }

        case 'V':{
          //for (auto it = parts.begin(); it != parts.end(); it++)
        fprintf(stderr,"string %s",optarg);
            {
              fprintf(stderr," have CSV \n");
              // fprintf(stderr," %s\n",parts.at(0).c_str());
             deque<string> values = escapeSplit(optarg, ',');
              if(values.size() != 2){
                fatal("Error: Option \"-V\" requires two argument: \"-V fmuid,path/to/input.csv\"\n");
              }

              //fmigo_csv_fmu csv_matrix;

              (*csv_fmu)[atoi(values.at(0).c_str())] = fmigo_CSV_matrix(values.at(1),',');
              //printCSVmatrix(csv_matrix[values.at(0)]);
            }
            break;
        }
        default:
            fatal("abort %c...\n",c);
        }
    }

    // Parse FMU paths in the end of the command line
    for (index = optind; index < (int)argv2.size(); index++) {
        fmuFilePaths->push_back(argv2[index]);
    }

    size_t numFMUs = fmuFilePaths->size();

    if (numFMUs == 0){
        error("No FMUs given. Aborting...\n");
        printHelp();
        exit(1);
    }

#ifdef USE_MPI
    if ((size_t)world_size != numFMUs + 1) {
        //only complain for the first node
        if (world_rank == 0) {
            error("Need exactly n+1 processes, where n is the number of FMUs (%zu)\n", numFMUs);
            info("Try re-running with mpiexec -np %zu fmigo-mpi [rest of command line]\n", numFMUs+1 );
        }
        exit(1);
    }
#endif

    // Check if connections refer to nonexistant FMU index
    int i = 0;
    for (auto it = connections->begin(); it != connections->end(); it++, i++) {
        checkFMUIndex(it, i, numFMUs);
    }

    i = 0;
    for (auto it = strongConnections->begin(); it != strongConnections->end(); it++, i++) {
        checkFMUIndex(it, i, numFMUs);
    }

    // Default step order is all FMUs in their current order
    if (stepOrder->size() == 0){
        for(c=0; c<(int)numFMUs; c++){
            stepOrder->push_back(c);
        }
    } else if (stepOrder->size() != numFMUs) {
        fatal("Step order/FMU count mismatch: %zu vs %zu\n", stepOrder->size(), numFMUs);
    }

    return; // OK
}