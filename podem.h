
#include <algorithm>
#include <functional>
#include <iomanip>
#include <chrono>
#include <iterator>
#include <map>
#include <set>
#include <stack>
#include <tuple>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <sstream>



enum GATE_TYPE{
	UNDEF = -1,
	INPT = 0,
	AND = 1,
	OR = 2,
	NAND = 3,
	NOR = 4,
	XOR = 5,
	XNOR = 6,
	NOT = 7,
	BUFF = 8,
	FROM =9,
	DFF = 10
};

enum VALUE_TYPE{
	INVALID = -1,
	ZERO = 0,
	ONE = 1,
	UNKNOWN = 2,
	D = 3,
	D_BAR = 4
};
enum DT_STATE{
	ALT_AVAIL,
	ALT_USED,
};

enum FAULT_TYPE{
	SA0 = 0,
	SA1 = 1
};

enum TASK_TYPE{
	STOP_SUCCESS = 0,
	PROPAGATE_TO_OUTPUT = 1,
	SENSITIZE_LUI = 2,
	BACKTRACK = 3,
	INVALID_TASK = -1
};

typedef struct NODE_TYPE{
	unsigned long long id;
	std::string nodeName;
	GATE_TYPE type;  //Gate type 
	unsigned int nfi,nfo;  //number of fanins and fanouts
	int po;
	int mark;
	VALUE_TYPE cVal,fVal;
	long long level;
	std::set<unsigned long long> conePI;
	std::vector<std::string> fin,fout;  //fanin and fanout members
	std::vector<unsigned long long> finAfterTop,foutAfterTop;
	int CC0,CC1,CO;  //Controllability and Observability
	//std::vector<unsigned long long>ascendOrderCC0,ascendOrderCC1; //todo check
	std::vector<std::pair<std::string,int>> eFault,dFault;	//todo
	std::set<unsigned long long> fList[3];
} NODE;


typedef struct PRIMARY_IO{
	unsigned long long id;
	VALUE_TYPE value;
} PIO;

typedef struct DECISION_TREE{
	unsigned long long id;
	VALUE_TYPE val;
	DT_STATE state = ALT_AVAIL;
} DT_NODE;

typedef struct DFRONTIER{
	unsigned long long id;
	mutable bool xPath;
}DFRONTIER_NODE;

typedef struct PODEM_SETTING{
	unsigned long long LUI;
	FAULT_TYPE stuckAtFault;
} P_SETT;

typedef struct FAULT_INFO{
	unsigned long long faultId;
	unsigned long long nodeId;
	std::string nodeName;
	FAULT_TYPE stuckAtFault;
	int dCount;
}FAULT_NODE;

typedef struct TEST_PATTERN_INFO{
	std::string testVector;
	std::set<unsigned long long> faults;
	int eCount;
} PATTERN;

typedef std::vector<std::pair<std::string,VALUE_TYPE>> LIST_OF_PI;
typedef std::vector<std::string> LIST_OF_PO;
typedef std::tuple<std::vector<NODE>, std::unordered_map<std::string,PIO>, std::unordered_map<std::string,PIO> > CIRCUIT_TUPLE;
typedef std::tuple<unsigned long long, VALUE_TYPE> BACKTRACE_TUPLE;
typedef std::tuple<unsigned long long, VALUE_TYPE> OBJECTIVE_TUPLE;
typedef std::tuple<unsigned long long, VALUE_TYPE> BACKTRACK_TUPLE;
typedef std::stack<DT_NODE> D_TREE;
typedef std::vector<std::pair<unsigned long long , VALUE_TYPE>> TEST_VECTOR;

//Truth Table 
extern VALUE_TYPE TT_AND[5][5];
extern VALUE_TYPE TT_OR[5][5];
extern VALUE_TYPE TT_XOR[5][5];
extern VALUE_TYPE TT_NOT[5];

//Circuit File Parsing
std::unordered_map<std::string,NODE> readCircuitStructuralDescription(std::ifstream &, LIST_OF_PI &, LIST_OF_PO &);
void initializeNode(NODE &);
bool checkEmptyLine(std::string);
std::string removeLeftSpaces(std::string &);
std::string removeRightSpaces(std::string &);
void removeSpaces(std::string &);
GATE_TYPE stringToGate(std::string);
void openCircuitFile(std::ifstream &,int,char **);
void addFromNodes(std::unordered_map<std::string,NODE> & , LIST_OF_PO &);
//Topological Sort
CIRCUIT_TUPLE topologicalSort(std::unordered_map<std::string,NODE> &, LIST_OF_PI & , LIST_OF_PO &);
void updateFanIO(std::unordered_map<std::string , NODE> & , std::string &, unsigned long long &);
void updateFanin(std::unordered_map<std::string,NODE> & , std::string & , unsigned long long &);
void updateFanout(std::unordered_map<std::string,NODE> & , std::string & , unsigned long long &);
void removePI(std::unordered_map<std::string,NODE> &, std::unordered_map<std::string,NODE> &, LIST_OF_PI &, std::vector<NODE> &);
void calculatePIConeset(std::vector<NODE> & ,NODE &);
void changeToTopologicalId(std::unordered_map<std::string,PIO> &,std::vector<std::string> &, std::vector<unsigned long long> &);
//Testability
void calculateTestabilityMeasures(std::vector<NODE> &);
void checkFromNode(std::vector<NODE> & , unsigned long long );
int minControllabilityToZero(std::vector<NODE> & , NODE &);
int minControllabilityToOne(std::vector<NODE> & , NODE &);
int faninControllabilitySumOfOne(std::vector<NODE> &, NODE &);
int faninControllabilitySumOfZero(std::vector<NODE> &, NODE &);
int faninControllabilitySumAB(std::vector<NODE> &, NODE & );
int faninControllabilitySumBA(std::vector<NODE> &, NODE & );
int min( int  , int );
//Display results
void printCircuitDescription(std::vector<NODE> &);
void printCircuit(std::unordered_map<std::string,NODE> &);
void showTestVector(std::vector<NODE> &,std::unordered_map<std::string,PIO> &PO);
//PODEM
void getTestVector(std::vector<NODE> &,std::unordered_map<std::string,PIO> &);
TEST_VECTOR generateTestPattern(std::vector<NODE> &,std::unordered_map<std::string,PIO> &);
void initialObjectiveAndBackTrace(std::vector<NODE> & , D_TREE & ,std::unordered_map<std::string,PIO> &);
template<class COMPARATOR>
TASK_TYPE decideTask(std::vector<NODE> &, std::unordered_map<std::string,PIO> &, std::set<DFRONTIER_NODE,COMPARATOR> &);
bool validChangeAtPO(std::vector<NODE> & ,std::unordered_map<std::string,PIO> &);
bool sensitizedLUI(std::vector<NODE> &);
bool LUIatUnknown(std::vector<NODE> &);
void initializeCircuitWithUnknowns(std::vector<NODE> &);
template<class COMPARATOR>
bool dFrontierExists(std::vector<NODE> &, std::set<DFRONTIER_NODE,COMPARATOR> &);
template<class COMPARATOR>
void generateDfrontier(std::vector<NODE> & , std::set<DFRONTIER_NODE,COMPARATOR> &, unsigned long long &, DFRONTIER_NODE &);
template<class COMPARATOR>
bool xPathCheck(std::vector<NODE> &, std::set<DFRONTIER_NODE,COMPARATOR> &);
bool lookForPossiblePathToPO(std::vector<NODE> &, unsigned long long , std::set<unsigned long long> &);
void implication(std::vector<NODE> &, std::unordered_map<std::string , PIO> &, unsigned long long , VALUE_TYPE);
void performLogicSimulation(std::vector<NODE> &);
VALUE_TYPE gateLogicEval(const GATE_TYPE &, const VALUE_TYPE &, const VALUE_TYPE &);
template<class COMPARATOR>
OBJECTIVE_TUPLE objectiveSelection(std::vector<NODE> & , std::set<DFRONTIER_NODE,COMPARATOR> &,TASK_TYPE &);
BACKTRACE_TUPLE backtrace(std::vector<NODE> &, D_TREE &, unsigned long long , VALUE_TYPE );
BACKTRACK_TUPLE backtrack(std::vector<NODE> &, D_TREE &);
bool noPossibleValues(D_TREE &);
void assignAlternateValue(D_TREE &);
VALUE_TYPE selectNonControllingValue(std::vector<NODE> & ,unsigned long long, GATE_TYPE &);
bool nodeWithHardestControl(std::vector<NODE> & , VALUE_TYPE , unsigned long long,unsigned long long);
bool nodeWithEasiestControl(std::vector<NODE> & , VALUE_TYPE , unsigned long long,unsigned long long);
bool isControllingValueAtOut(VALUE_TYPE &, GATE_TYPE &);
TEST_VECTOR fillTestVector(std::vector<NODE> &);
std::tuple<unsigned long long, VALUE_TYPE>  specialGate(std::vector<NODE> &, VALUE_TYPE ,unsigned long long );
bool unassignedFin(std::vector<NODE> & ,unsigned long long );
std::tuple<unsigned long long, VALUE_TYPE> chooseNodeWithHardControllability(std::vector<NODE> &, VALUE_TYPE,unsigned long long);
// MTTF
void initAssignAndImplication(std::vector<NODE> & , TEST_VECTOR &);
//FAULT SIMULATION
std::set<unsigned long long>  dfs(std::vector<NODE> & ,std::string& , std::vector<unsigned long long> &, std::unordered_map<std::string,PIO> &, std::vector<FAULT_NODE> &);
void deduceFaultList(std::set<unsigned long long> *, std::set<unsigned long long> * , std::set<unsigned long long> *, GATE_TYPE , VALUE_TYPE &, VALUE_TYPE &);
void addNodeFault(std::vector<FAULT_NODE> & , NODE &);
void faultListPropagation(std::vector<NODE> & , std::vector<FAULT_NODE> &);
void correctFaultList(std::set<unsigned long long> & , NODE &);
void copyCorrectedFaultList(std::set<unsigned long long> *, std::set<unsigned long long> *);
void copyFaninFaults(std::vector<NODE> & , unsigned long long );
void createFaultList(std::vector<NODE> &,std::vector<FAULT_NODE> & );
void showFaultList(std::vector<NODE> &);
void readTestVectorFile(std::vector<std::string> & , char ** ,std::vector<std::string> &);
VALUE_TYPE strToValue(char &);
TEST_VECTOR assignPIValue(std::string & ,std::vector<unsigned long long> &);
void detectedFaultList(std::vector<NODE> & , std::unordered_map<std::string,PIO> &, std::set<unsigned long long> &);
void show(std::set<unsigned long long> &);
void resetFaultList(std::vector<NODE> &);
void logicSimulation(std::vector<NODE> & , TEST_VECTOR &);
void gateLogicEvaluation(std::vector<NODE> &);

//Compact Test Set
void compactTestSet(std::vector<NODE> &, std::vector<unsigned long long> &, std::unordered_map<std::string,PIO> &, std::vector<FAULT_NODE> &, std::vector<std::string> &);
void rve(std::vector<FAULT_NODE> & ,std::vector<PATTERN> &, PATTERN &);
long long findPattern(std::vector<PATTERN> &, std::string &, const unsigned long long &);
void deleteFromTestSet(std::vector<PATTERN> &, std::vector<FAULT_NODE> &, PATTERN &);
void showFaultCount(std::vector<FAULT_NODE> &);
void generateCompactPattern(std::vector<NODE> &,std::unordered_map<std::string,PIO> &, std::vector<FAULT_NODE> &);
void stringTestVector(std::string & , TEST_VECTOR &);
char valToChar(VALUE_TYPE &);

