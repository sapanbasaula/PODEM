#include "podem.h"

int main(int argc,char *argv[]){
	std::ifstream benchFile;
	openCircuitFile(benchFile,argc,argv);
	std::unordered_map<std::string,NODE> circuitDescription;
	LIST_OF_PI listPI;
	LIST_OF_PO listPO;
	circuitDescription = readCircuitStructuralDescription(benchFile,listPI,listPO);
	benchFile.close();
	
	addFromNodes(circuitDescription,listPO);
	printCircuit(circuitDescription);
	std::vector<NODE> GRAPH;

	std::unordered_map<std::string,PIO> PI;
	std::unordered_map<std::string,PIO> PO;
	std::tie(GRAPH,PI,PO) = topologicalSort(circuitDescription,listPI,listPO);
	calculateTestabilityMeasures(GRAPH);
	printCircuitDescription(GRAPH);
	
	std::vector<std::string> PI_testFile;
	std::vector<std::string> testVector;
	readTestVectorFile(testVector, argv , PI_testFile);
	
	std::vector<unsigned long long> PI_order;
	
	changeToTopologicalId(PI,PI_testFile,PI_order);
	std::vector<FAULT_NODE> FAULTLIST;
	createFaultList(GRAPH,FAULTLIST);
	generateCompactPattern(GRAPH,PO,FAULTLIST);
//	compactTestSet(GRAPH,PI_order,PO,FAULTLIST,testVector);
//	show(detectedFaultList);
//	getTestVector(GRAPH,PO);
	return 0;
}

