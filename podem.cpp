#include "podem.h"

VALUE_TYPE TT_AND[5][5] = {{ZERO,ZERO,ZERO,ZERO,ZERO},{ZERO,ONE,UNKNOWN,D,D_BAR},{ZERO,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN},{ZERO,D,UNKNOWN,D,ZERO},{ZERO,D_BAR,UNKNOWN,ZERO,D_BAR}};
VALUE_TYPE TT_OR[5][5] = {{ZERO,ONE,UNKNOWN,D,D_BAR},{ONE,ONE,ONE,ONE,ONE},{UNKNOWN,ONE,UNKNOWN,UNKNOWN,UNKNOWN},{D,ONE,UNKNOWN,D,ONE},{D_BAR,ONE,UNKNOWN,ONE,D_BAR}};
VALUE_TYPE TT_XOR[5][5] = {{ZERO,ONE,UNKNOWN,D,D_BAR},{ONE,ZERO,UNKNOWN,D_BAR,D},{UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN},{D,D_BAR,UNKNOWN,ZERO,ONE},{D_BAR,D,UNKNOWN,ONE,ZERO}};
VALUE_TYPE TT_NOT[5] = {ONE,ZERO,UNKNOWN,D_BAR,D};

std::random_device rd;	
std::mt19937 engine(rd());
std::bernoulli_distribution getRandomBit;
P_SETT pSett;

void generateCompactPattern(std::vector<NODE> &G,std::unordered_map<std::string,PIO> &PO, std::vector<FAULT_NODE> &faultList){
	TEST_VECTOR testVector;
	std::vector<PATTERN> testSet;
	std::vector<unsigned long long> abortedFaults;
	std::string tVec;
	PATTERN temp;
	int val;
	std::set<unsigned long long> dList;	
	for(auto &fault:faultList){
		if(fault.dCount == 0){
			pSett.LUI = fault.nodeId;
			pSett.stuckAtFault = fault.stuckAtFault;
			
			testVector = generateTestPattern(G,PO);
			if(testVector.front().first == 0 && testVector.front().second == INVALID){
				abortedFaults.push_back(fault.faultId);
				continue;			
			}
			logicSimulation(G,testVector);
			resetFaultList(G);
			faultListPropagation(G,faultList);
			detectedFaultList(G,PO,dList);
			stringTestVector(tVec,testVector);
			temp.testVector = tVec;
			temp.faults = dList;
			show(temp.faults);
			temp.eCount = 0;
			dList.clear();
			rve(faultList,testSet,temp);
			tVec.clear();
		}		
	}
	showFaultCount(faultList);
	if(!abortedFaults.empty()){
		std::cout<<"Aborted Faults:";
		for(auto &ab:abortedFaults){
			std::cout<<ab<<" ";	
		}
		std::cout<<"\n";
	}
	std::cout<<std::endl;
	std::cout<<"FAULT COVERAGE:"<<(((double)(faultList.size() - abortedFaults.size()) / faultList.size()) * 100) <<"\n";
	std::cout<<"TEST SET SIZE:"<<testSet.size()<<std::endl;
}

void stringTestVector(std::string &tVec , TEST_VECTOR &testVector){
	for(auto &search:testVector){
		tVec += valToChar(search.second);			
	}
}

char valToChar(VALUE_TYPE &val){
	char retVal;
	if(val == ONE){
		retVal='1';	
	}else if(val == ZERO){
		retVal='0';	
	}else if(val == UNKNOWN){
		retVal= 'x';	
	}else{
		std::cout<<"Wrong Value at TEST_VECTOR";
		exit(1);	
	}
	return retVal;
}


TEST_VECTOR generateTestPattern(std::vector<NODE> &G,std::unordered_map<std::string,PIO> &PO){
	D_TREE DT;
	TASK_TYPE task = INVALID_TASK;
	TEST_VECTOR testVector;
	bool aborted = false;
	unsigned long long id,tempId;
	VALUE_TYPE val,tempVal;
	std::chrono::steady_clock::time_point timeStart;
	std::chrono::steady_clock::time_point timeCurrent;
	auto COMPARATOR = [&G](const DFRONTIER_NODE &lhs, const DFRONTIER_NODE &rhs){ return G[lhs.id].CO < G[rhs.id].CO;};
	std::set<DFRONTIER_NODE,decltype(COMPARATOR)> dFrontier(COMPARATOR);
	initializeCircuitWithUnknowns(G);
	timeStart = std::chrono::steady_clock::now();
	initialObjectiveAndBackTrace(G,DT,PO);
	while(!DT.empty()){
		timeCurrent = std::chrono::steady_clock::now();
		if(std::chrono::duration_cast<std::chrono::milliseconds> (timeCurrent-timeStart).count() > 100){
			aborted = true;
			break;		
		}
		task = decideTask(G,PO,dFrontier);
		if(task == STOP_SUCCESS){
//			std::cout<<"Test Pattern Found"<<std::endl;
			showTestVector(G,PO);
			testVector = fillTestVector(G);
			return testVector;	
		}else if(task == BACKTRACK){
			std::tie(id,val) = backtrack(G,DT);
			if(!DT.empty()){
				implication(G,PO,id,val);			
			}		
		}else{
			std::tie(tempId,tempVal) = objectiveSelection(G,dFrontier,task);
			std::tie(id,val) = backtrace(G,DT,tempId,tempVal);
			implication(G,PO,id,val);		
		}	
	}
	testVector.push_back(std::make_pair(0,INVALID));
	return testVector; 
}

void initialObjectiveAndBackTrace(std::vector<NODE> &G , D_TREE &DT,std::unordered_map<std::string,PIO> &PO){
	unsigned long long nodeId,pi_id;
	VALUE_TYPE val,pi_val;
	nodeId = pSett.LUI;
	if(pSett.stuckAtFault == SA0){
		val = ONE;	
	}else if(pSett.stuckAtFault == SA1){
		val = ZERO;	
	}else{
		std::cout<<"FAULT TYPE INVALID";
		exit(1);	
	}
	std::tie(pi_id,pi_val) = backtrace(G,DT,nodeId,val);
	if(pi_id == -1 && pi_val == INVALID){
		std::cout<<"NO POSSIBLE PATH TO INPUT FOUND";	
		exit(1);
	}
	implication(G,PO,pi_id,pi_val);
}

template<class COMPARATOR>
TASK_TYPE decideTask(std::vector<NODE> &G, std::unordered_map<std::string,PIO> &PO, std::set<DFRONTIER_NODE,COMPARATOR> &dFrontier){
	TASK_TYPE task = INVALID_TASK;
	if(sensitizedLUI(G)){
		if(validChangeAtPO(G,PO)){
			task = STOP_SUCCESS;		
		}else{
			if(dFrontierExists(G,dFrontier) && xPathCheck(G,dFrontier)){
				task = PROPAGATE_TO_OUTPUT;
			}else{
				task = 	BACKTRACK;		
			}		
		}	
	}else{
		if(LUIatUnknown(G)){
			task = SENSITIZE_LUI;
		}else{
			task = BACKTRACK;
		}
	}
	return task;
}

bool validChangeAtPO(std::vector<NODE> &G ,std::unordered_map<std::string,PIO> &PO){
	for(auto &po : PO){
		if(G[po.second.id].cVal == D || G[po.second.id].cVal == D_BAR){
			return true;		
		}	
	}
	return false;
}

bool sensitizedLUI(std::vector<NODE> &G){
	if(G[pSett.LUI].cVal == D || G[pSett.LUI].cVal == D_BAR ){
		return true;	
	}else{
		return false;
	}
}

bool LUIatUnknown(std::vector<NODE> &G){
	if(G[pSett.LUI].cVal == UNKNOWN){
		return true;
	}else{
		return false;
	}
}

void initializeCircuitWithUnknowns(std::vector<NODE> &G){
	for(auto &search : G){
		search.cVal = UNKNOWN;	
	}
}

template<class COMPARATOR>
bool dFrontierExists(std::vector<NODE> &G, std::set<DFRONTIER_NODE,COMPARATOR> &dFrontier){
	DFRONTIER_NODE temp;
	dFrontier.clear();
	generateDfrontier(G,dFrontier,pSett.LUI,temp);
/*	for(auto &df : dFrontier){
		std::cout<<"*"<<df.id<<"#";	
	}
	std::cout<<"\n";*/
	if(!dFrontier.empty()){
		return true;	
	}else{
		return false;	
	}
}

template<class COMPARATOR>
void generateDfrontier(std::vector<NODE> &G , std::set<DFRONTIER_NODE,COMPARATOR> &dFrontier, unsigned long long &id, DFRONTIER_NODE &temp){
	int dCount = 0;
	for(auto &search :G[id].foutAfterTop){
		if(G[search].cVal == UNKNOWN){
			for (auto & fin: G[search].finAfterTop){
				if(G[fin].cVal == D || G[fin].cVal == D_BAR){
					dCount++;
				}			
			}
			
			if(dCount == 1){
				temp.xPath = false;
				temp.id = search;
				dFrontier.insert(temp);
			}
			dCount = 0;
				
		}else if(G[search].cVal == D || G[search].cVal == D_BAR){
			generateDfrontier(G,dFrontier,search,temp);		
		}
	}
	return;
}

template<class COMPARATOR>
bool xPathCheck(std::vector<NODE> &G, std::set<DFRONTIER_NODE,COMPARATOR> &dFrontier){
	std::set<unsigned long long> visitedNodes;	
	bool foundPath=false;
	for(auto& search : dFrontier){
		search.xPath = lookForPossiblePathToPO(G,search.id,visitedNodes);
		if(search.xPath){
			break;
		}
	}
	for(auto& search : dFrontier){
		if(search.xPath){
			foundPath = true;		
		}	
	}
	return foundPath;
}

bool lookForPossiblePathToPO(std::vector<NODE> &G, unsigned long long id, std::set<unsigned long long> &visitedNodes){
	bool xPathFound = false;
	if(G[id].po){
		if(G[id].cVal == UNKNOWN){
			return true;		
		}else{
			return false;
		}
	}
	for(auto & fout : G[id].foutAfterTop){
		if(visitedNodes.find(fout) != visitedNodes.end()){
			continue;
		}	
		visitedNodes.insert(fout);
		xPathFound = lookForPossiblePathToPO(G,fout,visitedNodes);
		if(xPathFound){
			return true;
		}
	}
	return false;
}

void implication(std::vector<NODE> &G, std::unordered_map<std::string , PIO> &PO, unsigned long long piId, VALUE_TYPE piValue){
	int check;
	G[piId].cVal = piValue;
	performLogicSimulation(G);

/*	for (auto &search : G){
		std::cout<<search.nodeName <<"\t"<<search.type<<"\t"<< search.cVal<<std::endl;
	}
	std::cout<<"***********************"<<std::endl;
	std::cin>>check;*/
	
}

void performLogicSimulation(std::vector<NODE> &G){
	GATE_TYPE gate;
	VALUE_TYPE result;
	for(auto &search : G){
		if( search.type != INPT){
			result = INVALID;	
			for(auto &fin :search.finAfterTop){
				VALUE_TYPE &val = G[fin].cVal;
				result = gateLogicEval(search.type, result, val);
			}
			gate = search.type;
			if(gate == NOT || gate == NAND || gate == NOR || gate == XNOR){
				search.cVal = TT_NOT[result];		
			} else{
				search.cVal = result;
			}
		}
		if(search.id == pSett.LUI){
			if(search.cVal == TT_NOT[pSett.stuckAtFault]){
				if(search.cVal == ONE && pSett.stuckAtFault == SA0){
					search.cVal = D;				
				}else if(search.cVal == ZERO && pSett.stuckAtFault == SA1){
					search.cVal = D_BAR;				
				}else{
					std::cout<<"Wrong Values at Fault Sensitization "<<std::endl;
					exit(1);				
				}			
			}
		}
	}
}

VALUE_TYPE gateLogicEval(const GATE_TYPE &gate, const VALUE_TYPE &previousVal, const VALUE_TYPE &val){
	VALUE_TYPE result;
	if(previousVal == INVALID){
		return val;	
	}else if(gate == NOT || gate == BUFF || gate == FROM){
		result = val;	
	}else if(gate == AND || gate == NAND){
		result = TT_AND[previousVal][val];
	}else if(gate == OR || gate == NOR){
		result = TT_OR[previousVal][val];
	}else if(gate == XOR || gate == XNOR){
		result = TT_XOR[previousVal][val];
	}else {
		result = INVALID;	
	}
	return result;
}

template<class COMPARATOR>
OBJECTIVE_TUPLE objectiveSelection(std::vector<NODE> &G , std::set<DFRONTIER_NODE,COMPARATOR> &dFrontier, TASK_TYPE &task){
	unsigned long long id,dfId;
	VALUE_TYPE val;
	OBJECTIVE_TUPLE retVal;
	if(task == PROPAGATE_TO_OUTPUT){
		for(auto &df: dFrontier){
			if(df.xPath){
				dfId = df.id;
				break;
			}
		}
		//dfId = dFrontier.begin()->id;
		val = selectNonControllingValue(G,dfId,G[dfId].type);
		for(auto &search : G[dfId].finAfterTop){
			if(G[search].cVal == UNKNOWN){
				id = search;
				break;
			}	
		}
	}else if(task == SENSITIZE_LUI){
		id = pSett.LUI;
		if(pSett.stuckAtFault == SA0){
			val = ONE;	
		}else if(pSett.stuckAtFault == SA1){
			val = ZERO;	
		}else{
			std::cout<<"FAULT TYPE INVALID";
			exit(1);	
		}
	}else{
		std::cout<<"OBJECTIVE UNDEFINED";
		exit(1);
	}
	retVal = std::make_tuple(id,val);
	return retVal;
}

BACKTRACE_TUPLE backtrace(std::vector<NODE> &G, D_TREE &DT, unsigned long long nodeId, VALUE_TYPE cVal){
	unsigned long long id;
	DT_NODE dTree;
	VALUE_TYPE val=cVal;
	BACKTRACE_TUPLE retVal;
	unsigned long long previousId;
	while(G[nodeId].type != INPT){
		previousId = nodeId;
		if(G[nodeId].type == NAND || G[nodeId].type == NOR || G[nodeId].type == NOT){
			val = TT_NOT[cVal];		
		}
		if(isControllingValueAtOut(cVal,G[nodeId].type)){
			for(auto &fin : G[nodeId].finAfterTop){
				if(G[fin].cVal == UNKNOWN && nodeWithHardestControl(G,cVal,fin,nodeId)){
					nodeId = fin;	
					break;				
				}
			}
		}else if(G[nodeId].type == XOR){
			std::tie(nodeId,val) = specialGate(G,cVal,nodeId);
					
		}else{
			for(auto &fin : G[nodeId].finAfterTop){
				if(G[fin].cVal == UNKNOWN && nodeWithEasiestControl(G,cVal,fin,nodeId)){
					nodeId = fin;	
					break;				
				}
			}		
		}
		cVal = val;	
		if(previousId == nodeId){
			std::cout<<"BACKTRACE CANNOT FIND PATH TO INPUT";
			retVal = std::make_tuple(-1,INVALID);
			return retVal;		
		}
	}
	id = nodeId;
	dTree.id = id;
	dTree.val = val;
	dTree.state = ALT_AVAIL;
	DT.push(dTree);
	retVal = std::make_tuple(id,val);
	return retVal;
}

BACKTRACK_TUPLE backtrack(std::vector<NODE> &G, D_TREE &DT){
	unsigned long long id;
	VALUE_TYPE val;
	BACKTRACK_TUPLE retVal;
	if(!DT.empty()){
		while(noPossibleValues(DT)){
			G[DT.top().id].cVal = UNKNOWN;
			DT.pop();
			if(DT.empty()){
				break;			
			}		
		}	
	}
	if(DT.empty()){
		retVal = std::make_tuple(0,INVALID);	
	}else{
		assignAlternateValue(DT);
		id = DT.top().id;
		val = DT.top().val;
		retVal = std::make_tuple(id,val);
	}
	return retVal;
}

bool noPossibleValues(D_TREE &DT){
	DT_STATE state;
	state = DT.top().state;
	if(state == ALT_AVAIL){
		return false;	
	}else if( state == ALT_USED){
		return true;	
	}else{
		std::cout<<"IMPROPER DECISION TREE NODE STATE";
		exit(1);
	}
}

void assignAlternateValue(D_TREE &DT){
	VALUE_TYPE val;
	val = DT.top().val;
	if(val == ZERO){
		DT.top().val = ONE;	
	}else if( val == ONE){
		DT.top().val = ZERO;	
	}else{
		std::cout<<"ATERNATE VALUE ASSIGNMENT NOT POSSIBLE";
		exit(1);
	}
	DT.top().state = ALT_USED;
}

VALUE_TYPE selectNonControllingValue(std::vector<NODE> &G , unsigned long long id, GATE_TYPE &gate){
	if(gate == AND || gate == NAND){
		return ONE;	
	}else if( gate == OR || gate == NOR){
		return ZERO;	
	}else if( gate == XOR || gate == XNOR){
		if(G[id].CC0 < G[id].CC1){
			return ONE;		
		}else{
			return ZERO;		
		}
	}else{
		std::cout<<"GATE TYPE NOT FOUND: "<<gate;
		exit(1);	
	}
}

bool nodeWithHardestControl(std::vector<NODE> &G , VALUE_TYPE cVal, unsigned long long fin,unsigned long long nodeId){
	int high=0;
	if(G[nodeId].type == AND || G[nodeId].type == NAND){
		if(G[fin].cVal == UNKNOWN){
			high = G[fin].CC1;
		}
		for(auto &search: G[nodeId].finAfterTop){
			if(G[search].cVal == UNKNOWN ){
				if(high < G[search].CC1){
					return false;			
				}		
			}
			
		}
		if(high == 0) return false;
		return true;
	}else{
		if(G[fin].cVal == UNKNOWN){
			high = G[fin].CC0;
		}
		for(auto &search: G[nodeId].finAfterTop){
			if(G[search].cVal == UNKNOWN ){
				if(high < G[search].CC0){
					return false;			
				}		
			}
			
		}
		if(high == 0) return false;
		return true;
	}
}

bool nodeWithEasiestControl(std::vector<NODE> &G , VALUE_TYPE cVal, unsigned long long fin,unsigned long long nodeId){
	int low=0;
	if(G[nodeId].type == OR || G[nodeId].type == NOR){
		if(G[fin].cVal == UNKNOWN){
			low = G[fin].CC1;
		}
		for(auto &search: G[nodeId].finAfterTop){
			if(G[search].cVal == UNKNOWN ){
				if(low > G[search].CC1){
					return false;			
				}		
			}
			
		}
		if(low == 0) return false;
		return true;
	}else{
		if(G[fin].cVal == UNKNOWN){
			low = G[fin].CC0;
		}
		for(auto &search: G[nodeId].finAfterTop){
			if(G[search].cVal == UNKNOWN ){
				if(low > G[search].CC0){
					return false;			
				}		
			}
			
		}
		if(low == 0) return false;
		return true;
	}
}

bool isControllingValueAtOut(VALUE_TYPE &cVal, GATE_TYPE &gate){
	if((gate == AND || gate == NOR ) && cVal == ONE){
		return true;	
	}else if((gate == OR || gate == NAND) && cVal == ZERO){
		return true;	
	}  // It won't matter for the single input gates 
	return false; // Difficult to say for XOR gates
}

void showTestVector(std::vector<NODE> &G,std::unordered_map<std::string,PIO> &PO){
	std::cout<<G[pSett.LUI].nodeName<<"/"<<pSett.stuckAtFault<<":\n";
	for(int i=0;i<G.size();i++){
		if(G[i].type == INPT){
//			std::cout<<G[i].nodeName<<":";
			if(G[i].cVal == D || G[i].cVal == ONE){
				std::cout<<"1";	
			}else if(G[i].cVal == D_BAR || G[i].cVal == ZERO){
				std::cout<<"0";				
			}else if(G[i].cVal == UNKNOWN){
				std::cout<<"x";			
			}else{
				std::cout<<"UNKNOWN VALUE AT INPUT";		
			}
		}	
	}
	
//	std::cout<<"\tNEXT STATE:";
//	for(auto &search : PO){
//		std::cout<<search.first<<":";
//		if(G[search.second.id].cVal == UNKNOWN){
//			std::cout<<"x";			
//		}else if(G[search.second.id].cVal == D){
//			std::cout<<"D";
//		}else if(G[search.second.id].cVal == D_BAR){
//			std::cout<<"D`";
//		}else{
//			std::cout<<G[search.second.id].cVal<<"\n";		
//		}
//	}
	std::cout<<std::endl;
}

TEST_VECTOR fillTestVector(std::vector<NODE> &G){
	TEST_VECTOR testVector;
	for(auto &node : G){
		if(node.type == INPT){
			if(node.cVal == D){
				node.cVal = ONE;
			}else if(node.cVal == D_BAR){
				node.cVal = ZERO;			
			}
			testVector.push_back(std::make_pair(node.id,node.cVal));
		}	
	}
	return testVector;
}


void initAssignAndImplication(std::vector<NODE> &G , TEST_VECTOR &testOld){
	for(auto &test: testOld){
		G[test.first].cVal = test.second; 	
	}
	performLogicSimulation(G);
}

std::tuple<unsigned long long, VALUE_TYPE>  specialGate(std::vector<NODE> &G , VALUE_TYPE cVal,unsigned long long nodeId){
	std::tuple<unsigned long long, VALUE_TYPE> retVal;
	unsigned long long prevFin,temp;
	prevFin = G[nodeId].finAfterTop.front();
	for(auto &fin : G[nodeId].finAfterTop){
		if(prevFin == fin) continue;
		if(unassignedFin(G,nodeId)){
			retVal=chooseNodeWithHardControllability(G,cVal,nodeId);		
		}else{
			if(G[prevFin].cVal ==UNKNOWN){ 
				temp = fin;
				fin = prevFin;
				prevFin = temp;
			}
			switch(cVal){
				case ONE:
					if(G[prevFin].cVal == ZERO){
						retVal = std::make_tuple(fin,ONE);					
					}else if(G[prevFin].cVal == ONE){
						retVal = std::make_tuple(fin,ZERO);											
					}else{
						retVal = std::make_tuple(fin,ZERO); // If one of the input has D_VALUE, value cannot be propagated
					}
					break;
				case ZERO:
					if(G[prevFin].cVal == ZERO){
						retVal = std::make_tuple(fin,ZERO);						
					}else if(G[prevFin].cVal == ONE){
						retVal = std::make_tuple(fin,ONE);											
					}else{
						retVal = std::make_tuple(fin,ZERO);
					}
					break;	
				default:
					break;		
			}		
		}	
	}
	return retVal;
}

bool unassignedFin(std::vector<NODE> &G ,unsigned long long nodeId){
	for(auto &fin : G[nodeId].finAfterTop){
		if(G[fin].cVal != UNKNOWN){ return false;}
	}
	return true;
}

std::tuple<unsigned long long, VALUE_TYPE> chooseNodeWithHardControllability(std::vector<NODE> &G, VALUE_TYPE cVal,unsigned long long nodeId){
	std::tuple<unsigned long long, VALUE_TYPE> retVal;
	unsigned long long prevFin,val1,val2;
	prevFin = G[nodeId].finAfterTop.front();
	for(auto &fin : G[nodeId].finAfterTop){
		if(prevFin == fin) continue;
		switch(cVal){
			case ONE :
				val1 = G[prevFin].CC0 + G[fin].CC1;
				val2 = G[prevFin].CC1 + G[fin].CC0;
				if(val1 > val2){
					retVal = std::make_tuple(prevFin,ZERO);				
				}else{
					retVal = std::make_tuple(prevFin,ONE);
				}
				break;
			case ZERO :
				val1 = G[prevFin].CC0 + G[fin].CC0;
				val2 = G[prevFin].CC1 + G[fin].CC1;
				if(val1 > val2){
					retVal = std::make_tuple(prevFin,ZERO);				
				}else{
					retVal = std::make_tuple(prevFin,ONE);
				}
				break;
			default:
				break;
		}
	}
	return retVal;
}

