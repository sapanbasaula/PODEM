#include "podem.h"

//Three valued Deductive Fault Simulator (1,0,X)
// The approach of dfs is pessimistic for the purpose of this program. If the good value of a node is unknown , the faultList is cleared for the particular node.
// At the primary output, the faults propagated as unknowns are ignored.

std::set<unsigned long long> dfs(std::vector<NODE> &G,std::string &testVec,std::vector<unsigned long long> &PI_order, std::unordered_map<std::string,PIO> &PO, std::vector<FAULT_NODE> &faultList){
	std::set<unsigned long long> retVal;
	TEST_VECTOR vector;
	vector = assignPIValue(testVec,PI_order);
	logicSimulation(G,vector);
	faultListPropagation(G,faultList);
	detectedFaultList(G,PO,retVal);
	resetFaultList(G);
	return retVal;
}

void logicSimulation(std::vector<NODE> &G , TEST_VECTOR &testVec){
	for(auto &test: testVec){
		G[test.first].cVal = test.second; 	
	}
	gateLogicEvaluation(G);
}

void gateLogicEvaluation(std::vector<NODE> &G){
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
	}
}

void detectedFaultList(std::vector<NODE> &G , std::unordered_map<std::string,PIO> &PO, std::set<unsigned long long> &faultList){
	for(auto &po : PO){
		for(int i=0 ; i<3; i++){
			if(i==2) continue;
			for(auto &fault: G[po.second.id].fList[i]){
				faultList.insert(fault);		
			}	
		}
	}
}

void resetFaultList(std::vector<NODE> &G){
	for(auto &node : G){
		for(int i=0; i<3; i++){
			node.fList[i].clear();		
		}
	}
}

TEST_VECTOR assignPIValue(std::string &testVector ,std::vector<unsigned long long> &PI_order){
	TEST_VECTOR retVal;
	int i=0;
	for(auto &pi : PI_order){
		retVal.push_back(std::make_pair(pi,strToValue(testVector[i])));	
		i++;
	}
	return retVal;
}

VALUE_TYPE strToValue(char &ch){
	VALUE_TYPE retVal;
	if(ch == '0'){
		retVal = ZERO;	
	}else if(ch == '1'){
		retVal = ONE;	
	}else if(ch == 'x'){
		retVal = UNKNOWN;	
	}else{
		retVal = INVALID;	
	}
	return retVal;
}

void addNodeFault(std::vector<FAULT_NODE> &faultList , NODE &node){
	if(node.cVal == ONE){
		node.fList[0].insert(faultList[node.id*2+SA0].faultId);
	}else if(node.cVal == ZERO){
		node.fList[1].insert(faultList[node.id*2+SA1].faultId);	
	}else if(node.cVal == UNKNOWN){
		node.fList[0].clear();	
		node.fList[1].clear();
		node.fList[2].clear();
	}else{
		std::cout<<node.cVal<<std::endl;
		std::cout<<"UNDEFINED LOGIC VALUE AT NODE\n";
		exit(1);	
	}
}

void correctFaultList(std::set<unsigned long long> &temp , NODE &node){
	temp = node.fList[0];
	node.fList[0] = node.fList[1];
	node.fList[1] = temp;
	node.fList[2] = node.fList[2];
}

void faultListPropagation(std::vector<NODE> &G , std::vector<FAULT_NODE> &faultList){
	VALUE_TYPE net1Val,net2Val;
	unsigned long long fin;
	std::set<unsigned long long> s1;
	for(auto &node :G){
		switch(node.type){
			case INPT:
				addNodeFault(faultList,node);
				break;
			case NOT:
				copyFaninFaults(G,node.id);
				correctFaultList(s1,node); 
				addNodeFault(faultList,node);
				break; 
			case BUFF:
			case FROM:
				copyFaninFaults(G,node.id);
				addNodeFault(faultList,node);
				break;
			case AND:
				fin = node.finAfterTop.front();
				net1Val = G[fin].cVal;
				for(auto &nextFin : node.finAfterTop){
					if(nextFin == fin) continue;
					net2Val = G[nextFin].cVal;
					deduceFaultList(G[fin].fList,G[nextFin].fList,G[node.id].fList,node.type,net1Val,net2Val);
					fin = node.id;
					net1Val = TT_AND[net1Val][net2Val];
				}
				addNodeFault(faultList,node);
				break;
			case NAND:
				fin = node.finAfterTop.front();
				net1Val = G[fin].cVal;
				for(auto &nextFin : node.finAfterTop){
					if(nextFin == fin) continue;
					net2Val = G[nextFin].cVal;
					deduceFaultList(G[fin].fList,G[nextFin].fList,G[node.id].fList,node.type,net1Val,net2Val);
					fin = node.id;
					net1Val = TT_AND[net1Val][net2Val];
				}
				correctFaultList(s1,node); 
				addNodeFault(faultList,node);
				break; 
			case OR:
				fin = node.finAfterTop.front();
				net1Val = G[fin].cVal;
				for(auto &nextFin : node.finAfterTop){
					if(nextFin == fin) continue;
					net2Val = G[nextFin].cVal;
					deduceFaultList(G[fin].fList,G[nextFin].fList,G[node.id].fList,node.type,net1Val,net2Val);
					fin = node.id;
					net1Val = TT_OR[net1Val][net2Val];
				}
				addNodeFault(faultList,node);
				break;
			case NOR:
				fin = node.finAfterTop.front();
				net1Val = G[fin].cVal;
				for(auto &nextFin : node.finAfterTop){
					if(nextFin == fin) continue;
					net2Val = G[nextFin].cVal;
					deduceFaultList(G[fin].fList,G[nextFin].fList,G[node.id].fList,node.type,net1Val,net2Val);
					fin = node.id;
					net1Val = TT_OR[net1Val][net2Val];
				}
				correctFaultList(s1,node); 
				addNodeFault(faultList,node);
				break; 
			case XOR:
				fin = node.finAfterTop.front();
				net1Val = G[fin].cVal;
				for(auto &nextFin : node.finAfterTop){
					if(nextFin == fin) continue;
					net2Val = G[nextFin].cVal;
					deduceFaultList(G[fin].fList,G[nextFin].fList,G[node.id].fList,node.type,net1Val,net2Val);
					fin = node.id;
					net1Val = TT_XOR[net1Val][net2Val];
				}
				addNodeFault(faultList,node);
				break;
			case XNOR:
				fin = node.finAfterTop.front();
				net1Val = G[fin].cVal;
				for(auto &nextFin : node.finAfterTop){
					if(nextFin == fin) continue;
					net2Val = G[nextFin].cVal;
					deduceFaultList(G[fin].fList,G[nextFin].fList,G[node.id].fList,node.type,net1Val,net2Val);
					fin = node.id;
					net1Val = TT_XOR[net1Val][net2Val];
				}
				correctFaultList(s1,node); 
				addNodeFault(faultList,node);
				break; 
			default:
				break;		
		}	
	}
}

void deduceFaultList(std::set<unsigned long long> *net1Fault, std::set<unsigned long long> *net2Fault , std::set<unsigned long long> *nodeFault, GATE_TYPE type, VALUE_TYPE &net1Val, VALUE_TYPE &net2Val){
	std::set<unsigned long long> s1,s2,s3,s4,s5,s6;
	std::set<unsigned long long> f1[3],f2[3],f3[3],f4[3];
	switch(type){
		case NAND:
		case AND:
			if(net1Val == ZERO && net2Val == ZERO){
				std::set_intersection(net1Fault[1].begin(),net1Fault[1].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s1,s1.begin()));
				std::set_intersection(net1Fault[2].begin(),net1Fault[2].end(),net2Fault[2].begin(),net2Fault[2].end(),std::inserter(s2,s2.begin()));
			 	std::set_intersection(net1Fault[1].begin(),net1Fault[1].end(),net2Fault[2].begin(),net2Fault[2].end(),std::inserter(s3,s3.begin()));	
				std::set_intersection(net1Fault[2].begin(),net1Fault[2].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s4,s4.begin()));
				std::set_union(s2.begin(),s2.end(),s3.begin(),s3.end(),std::inserter(s5,s5.begin()));
				std::set_union(s5.begin(),s5.end(),s4.begin(),s4.end(),std::inserter(s6,s6.begin()));
				nodeFault[1].clear();
				nodeFault[2].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[1],nodeFault[1].begin()));
				std::copy(s6.begin(),s6.end(),std::inserter(nodeFault[2],nodeFault[2].begin()));
			}else if(net1Val == ZERO && net2Val == UNKNOWN){
				std::set_intersection(net1Fault[1].begin(),net1Fault[1].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s1,s1.begin()));
				std::set_difference(net1Fault[2].begin(),net1Fault[2].end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s2,s2.begin()));
				std::set_difference(net1Fault[1].begin(),net1Fault[1].end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s3,s3.begin()));
				std::set_difference(s3.begin(),s3.end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s4,s4.begin()));
				std::set_union(s2.begin(),s2.end(),s4.begin(),s4.end(),std::inserter(s5,s5.begin()));
				nodeFault[1].clear();
				nodeFault[2].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[1],nodeFault[1].begin()));
				std::copy(s5.begin(),s5.end(),std::inserter(nodeFault[2],nodeFault[2].begin()));				
			}else if(net1Val == ONE && net2Val == ZERO){
				std::set_difference(net2Fault[1].begin(),net2Fault[1].end(),net1Fault[0].begin(),net1Fault[0].end(),std::inserter(s1,s1.begin()));
				std::set_difference(s1.begin(),s1.end(),net1Fault[2].begin(),net1Fault[2].end(),std::inserter(s2,s2.begin()));
			 	std::set_difference(net2Fault[2].begin(),net2Fault[2].end(),net1Fault[0].begin(),net1Fault[0].end(),std::inserter(s3,s3.begin()));			
				std::set_intersection(net2Fault[1].begin(),net2Fault[1].end(),net1Fault[2].begin(),net1Fault[2].end(),std::inserter(s4,s4.begin()));
				std::set_union(s3.begin(),s3.end(),s4.begin(),s4.end(),std::inserter(s5,s5.begin()));
				nodeFault[1].clear();
				nodeFault[2].clear();
				std::copy(s2.begin(),s2.end(),std::inserter(nodeFault[1],nodeFault[1].begin()));
				std::copy(s5.begin(),s5.end(),std::inserter(nodeFault[2],nodeFault[2].begin()));	
			}else if(net1Val == ONE && net2Val == ONE){
				std::set_union(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s1,s1.begin()));
				std::set_difference(net1Fault[2].begin(),net1Fault[2].end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s2,s2.begin()));
				std::set_difference(net2Fault[2].begin(),net2Fault[2].end(),net1Fault[0].begin(),net1Fault[0].end(),std::inserter(s3,s3.begin()));
				std::set_union(s3.begin(),s3.end(),s2.begin(),s2.end(),std::inserter(s4,s4.begin()));
				nodeFault[0].clear();
				nodeFault[2].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[0],nodeFault[0].begin()));
				std::copy(s4.begin(),s4.end(),std::inserter(nodeFault[2],nodeFault[2].begin()));
			}else if(net1Val == ONE && net2Val == UNKNOWN){
				std::set_union(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s1,s1.begin()));
				std::set_difference(net2Fault[1].begin(),net2Fault[1].end(),net1Fault[0].begin(),net1Fault[0].end(),std::inserter(s2,s2.begin()));
				std::set_difference(s2.begin(),s2.end(),net1Fault[2].begin(),net1Fault[2].end(),std::inserter(s3,s3.begin()));
				nodeFault[0].clear();
				nodeFault[1].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[0],nodeFault[0].begin()));
				std::copy(s3.begin(),s3.end(),std::inserter(nodeFault[1],nodeFault[1].begin()));
			}else if(net1Val == UNKNOWN && net2Val == UNKNOWN){
				std::set_union(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s1,s1.begin()));
				std::set_intersection(net1Fault[1].begin(),net1Fault[1].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s2,s2.begin()));
				nodeFault[0].clear();
				nodeFault[1].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[0],nodeFault[0].begin()));
				std::copy(s2.begin(),s2.end(),std::inserter(nodeFault[1],nodeFault[1].begin()));
			}else if((net1Val == UNKNOWN && net2Val == ZERO) || (net1Val == ZERO && net2Val == ONE) || (net1Val == UNKNOWN && net2Val == ONE)){
				deduceFaultList(net2Fault,net1Fault,nodeFault,type,net2Val,net1Val);			
			}else{
				std::cout<<"WRONG INPUT VALUES AT THE GATES";
				exit(1);			
			}
			break;
		case NOR:
		case OR:
			if(net1Val == ZERO && net2Val == ZERO){
				std::set_union(net1Fault[1].begin(),net1Fault[1].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s1,s1.begin()));
				std::set_difference(net1Fault[2].begin(),net1Fault[2].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s2,s2.begin()));
				std::set_difference(net2Fault[2].begin(),net2Fault[2].end(),net1Fault[1].begin(),net1Fault[1].end(),std::inserter(s3,s3.begin()));
				std::set_union(s3.begin(),s3.end(),s2.begin(),s2.end(),std::inserter(s4,s4.begin()));
				nodeFault[1].clear();
				nodeFault[2].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[1],nodeFault[1].begin()));
				std::copy(s4.begin(),s4.end(),std::inserter(nodeFault[2],nodeFault[2].begin()));
			}else if(net1Val == ZERO && net2Val == UNKNOWN){
				std::set_union(net1Fault[1].begin(),net1Fault[1].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s1,s1.begin()));
				std::set_difference(net2Fault[0].begin(),net2Fault[0].end(),net1Fault[1].begin(),net1Fault[1].end(),std::inserter(s2,s2.begin()));
				std::set_difference(s2.begin(),s2.end(),net1Fault[2].begin(),net1Fault[2].end(),std::inserter(s3,s3.begin()));
				nodeFault[1].clear();
				nodeFault[0].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[1],nodeFault[1].begin()));
				std::copy(s3.begin(),s3.end(),std::inserter(nodeFault[0],nodeFault[0].begin()));
			}else if(net1Val == ONE && net2Val == ZERO){
				std::set_difference(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s1,s1.begin()));
				std::set_difference(s1.begin(),s1.end(),net2Fault[2].begin(),net2Fault[2].end(),std::inserter(s2,s2.begin()));
			 	std::set_difference(net1Fault[2].begin(),net1Fault[2].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s3,s3.begin()));			
				std::set_intersection(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[2].begin(),net2Fault[2].end(),std::inserter(s4,s4.begin()));
				std::set_union(s3.begin(),s3.end(),s4.begin(),s4.end(),std::inserter(s5,s5.begin()));
				nodeFault[0].clear();
				nodeFault[2].clear();
				std::copy(s2.begin(),s2.end(),std::inserter(nodeFault[0],nodeFault[0].begin()));
				std::copy(s5.begin(),s5.end(),std::inserter(nodeFault[2],nodeFault[2].begin()));	
			}else if(net1Val == ONE && net2Val == ONE){
				std::set_intersection(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s1,s1.begin()));
				std::set_intersection(net1Fault[2].begin(),net1Fault[2].end(),net2Fault[2].begin(),net2Fault[2].end(),std::inserter(s2,s2.begin()));
			 	std::set_intersection(net2Fault[0].begin(),net2Fault[0].end(),net1Fault[2].begin(),net1Fault[2].end(),std::inserter(s3,s3.begin()));	
				std::set_intersection(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[2].begin(),net2Fault[2].end(),std::inserter(s4,s4.begin()));
				std::set_union(s2.begin(),s2.end(),s3.begin(),s3.end(),std::inserter(s5,s5.begin()));
				std::set_union(s5.begin(),s5.end(),s4.begin(),s4.end(),std::inserter(s6,s6.begin()));
				nodeFault[0].clear();
				nodeFault[2].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[0],nodeFault[0].begin()));
				std::copy(s6.begin(),s6.end(),std::inserter(nodeFault[2],nodeFault[2].begin()));
			}else if(net1Val == ONE && net2Val == UNKNOWN){
				std::set_intersection(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s1,s1.begin()));
				std::set_difference(net1Fault[2].begin(),net1Fault[2].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s2,s2.begin()));
				std::set_difference(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s3,s3.begin()));
				std::set_difference(s3.begin(),s3.end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s4,s4.begin()));
				std::set_union(s2.begin(),s2.end(),s4.begin(),s4.end(),std::inserter(s5,s5.begin()));
				nodeFault[0].clear();
				nodeFault[2].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[0],nodeFault[0].begin()));
				std::copy(s5.begin(),s5.end(),std::inserter(nodeFault[2],nodeFault[2].begin()));
			}else if(net1Val == UNKNOWN && net2Val == UNKNOWN){
				std::set_union(net1Fault[1].begin(),net1Fault[1].end(),net2Fault[1].begin(),net2Fault[1].end(),std::inserter(s1,s1.begin()));
				std::set_intersection(net1Fault[0].begin(),net1Fault[0].end(),net2Fault[0].begin(),net2Fault[0].end(),std::inserter(s2,s2.begin()));
				nodeFault[1].clear();
				nodeFault[0].clear();
				std::copy(s1.begin(),s1.end(),std::inserter(nodeFault[1],nodeFault[1].begin()));
				std::copy(s2.begin(),s2.end(),std::inserter(nodeFault[0],nodeFault[0].begin()));
			}else if((net1Val == UNKNOWN && net2Val == ZERO) || (net1Val == ZERO && net2Val == ONE) || (net1Val == UNKNOWN && net2Val == ONE)){
				deduceFaultList(net2Fault,net1Fault,nodeFault,type,net2Val,net1Val);		
			}else{
				std::cout<<"WRONG INPUT VALUES AT THE GATES";
				exit(1);			
			}
			break;
		case XOR:
			copyCorrectedFaultList(f1,net2Fault);
			deduceFaultList(net1Fault,f1,f3,AND,net1Val,TT_NOT[net2Val]);
			copyCorrectedFaultList(f2,net1Fault);
			deduceFaultList(f2,net2Fault,f4,AND,TT_NOT[net1Val],net2Val);
			deduceFaultList(f3,f4,nodeFault,OR,TT_AND[net1Val][TT_NOT[net2Val]],TT_AND[TT_NOT[net1Val]][net1Val]);
			break;
		default:
			break;			
	}
}

void copyCorrectedFaultList(std::set<unsigned long long> *net1Fault, std::set<unsigned long long> *net2Fault ){
	net1Fault[0]=net2Fault[1]; 
	net1Fault[1]=net2Fault[0];
	net1Fault[2]=net2Fault[2];
}

void copyFaninFaults(std::vector<NODE> &G , unsigned long long id){
	for(auto &fin : G[id].finAfterTop){
		std::copy(G[fin].fList[0].begin(),G[fin].fList[0].end(),std::inserter(G[id].fList[0],G[id].fList[0].begin()));
		std::copy(G[fin].fList[1].begin(),G[fin].fList[1].end(),std::inserter(G[id].fList[1],G[id].fList[1].begin()));
		std::copy(G[fin].fList[2].begin(),G[fin].fList[2].end(),std::inserter(G[id].fList[2],G[id].fList[2].begin()));
	}
}

void createFaultList(std::vector<NODE> &G,std::vector<FAULT_NODE> &fList){
	FAULT_NODE temp;
	unsigned long long id;
	for(auto &node: G){
		for(int i=0;i<2;i++){
			id=fList.size();
			temp.faultId = id;
			temp.nodeId = node.id;
			temp.nodeName = node.nodeName;
			temp.dCount = 0;
			if( i == 0){
				temp.stuckAtFault = SA0;
			}else{
				temp.stuckAtFault = SA1;			
			}
			fList.push_back(temp);
		}
	}
}

void readTestVectorFile(std::vector<std::string> &testVector , char **argv , std::vector<std::string> &PI_order){
	std::ifstream testFile;
	std::string myString,currentLine;
	std::istringstream lineStream;
	lineStream.str(std::string());
	lineStream.clear();
	lineStream.str(argv[1]);
	std::getline(lineStream,myString,'.');
	myString += ".test";
	testFile.open(myString);
	while(std::getline(testFile,currentLine)){
		if(checkEmptyLine(currentLine)){
			continue;		
		}else{ 
			lineStream.str(std::string());
			lineStream.clear();
			lineStream.str(currentLine);
			if(currentLine.find("*") != std::string::npos){
				if(currentLine.find("inputs") != std::string::npos){
					while(std::getline(testFile,currentLine)){
						if(checkEmptyLine(currentLine)) break;
						lineStream.str(std::string());
						lineStream.clear();
						lineStream.str(currentLine);
						while(std::getline(lineStream,myString,' ')){
							removeSpaces(myString);
							PI_order.push_back(myString);					
						}			
					}
				}		
			}else{
				if(currentLine.find(":") != std::string::npos){
					std::getline(lineStream,myString,':');
					std::getline(lineStream,myString,' ');	
					std::getline(lineStream,myString,' ');
					testVector.push_back(myString);		
				}		
			}
		}	
	}
	testFile.close();
}

void changeToTopologicalId(std::unordered_map<std::string,PIO> &PI,std::vector<std::string> &PI_testFile, std::vector<unsigned long long> &PI_order){
	for(auto &pi : PI_testFile){
		auto search = PI.find(pi);
		if(search != PI.end()){
			PI_order.push_back(search->second.id);	
		}	
	}
	
}

void showFaultList(std::vector<NODE> &G){
	for(auto &node : G){
		std::cout<<"Cval:"<<node.cVal<<"\n";
		for(int i=0 ;i<3 ;++i){
			std::cout<<node.nodeName<<"["<<i<<"]"<<":\n";
			for(auto &list : node.fList[i]){
				std::cout<<list<<" ";		
			}	
			std::cout<<"\n";
		}
	}
}

void show(std::set<unsigned long long> &faultList){
	std::cout<<"DETECTED FAULTS:";
	for(auto &fault: faultList){
		std::cout<<fault<<" ";
	}
	std::cout<<"\n";
}
