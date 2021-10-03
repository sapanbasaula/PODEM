#include "podem.h"

std::unordered_map<std::string, NODE> readCircuitStructuralDescription(std::ifstream & benchFile, LIST_OF_PI & listPI, LIST_OF_PO & listPO){
	std::unordered_map<std::string,NODE> circuitDescription;
	NODE tempNode;
	std::string currentLine;
	std::istringstream lineStream;
	std::string myString;
	
	GATE_TYPE tempGate;
	while(std::getline(benchFile,currentLine)){
		initializeNode(tempNode);
		if(checkEmptyLine(currentLine) || (currentLine.find("#") != std::string::npos)){
			continue;	
		}else{
			lineStream.str(std::string());
			lineStream.clear();
			lineStream.str(currentLine);
			if(currentLine.find("INPUT(") != std::string::npos){
				std::getline(lineStream,myString,'(');
				std::getline(lineStream,myString,')');
				removeSpaces(myString);
				const auto & search = circuitDescription.find(myString);
				if( search != circuitDescription.end()){
					search->second.type = INPT;
				}else{
					tempNode.type = INPT;
					tempNode.nodeName = myString;
					circuitDescription.insert({myString,tempNode});
				}	
				listPI.push_back(make_pair(myString,UNKNOWN));
			}else if(currentLine.find("OUTPUT(") != std::string::npos){
				std::getline(lineStream,myString,'(');
				std::getline(lineStream,myString,')');
				removeSpaces(myString);
				const auto & search = circuitDescription.find(myString);
				if( search != circuitDescription.end()){
					search->second.po = 1;
				}else{
					tempNode.nodeName = myString;
					tempNode.po = 1;
					circuitDescription.insert({myString,tempNode});
				}	
				listPO.push_back(myString);	
			}else{
				std::getline(lineStream,myString,'=');
				removeSpaces(myString);
				tempNode.nodeName = myString;

				std::getline(lineStream,myString,'(');
				removeSpaces(myString);
				
				tempGate = stringToGate(myString);
				if(tempGate == UNDEF){
					std::cout<<"UNDEFINED GATE TYPE IN THE CIRCUIT FILE";				
				}else{
					switch(tempGate){
						case INPT: 
							break;
						case AND:
						case NAND:
						case OR:
						case NOR:
						case XOR:
						case XNOR:
						case BUFF:
						case NOT:{
							tempNode.type = tempGate;
							const auto & insert = circuitDescription.insert( {tempNode.nodeName,tempNode});	
							if(!insert.second){  //if the element already exists
								insert.first->second.type = tempNode.type;
							}
							while(std::getline(lineStream,myString,',')){
								myString.erase(std::remove(myString.begin(),myString.end(),')'),myString.end());
								removeSpaces(myString);
								const auto & search1 = circuitDescription.find(myString);
								if(search1 != circuitDescription.end()){
									search1->second.fout.push_back(tempNode.nodeName);
									search1->second.nfo++;
								}else{
									NODE newNode;	
									initializeNode(newNode);
									newNode.nodeName = myString;
									newNode.fout.push_back(tempNode.nodeName);
									newNode.nfo++;
									circuitDescription.insert({myString,newNode});
								}
								insert.first->second.fin.push_back(myString);
								insert.first->second.nfi++;
							}
							break;
												
						}
						case FROM:
							break;
						case DFF:{
							std::getline(lineStream,myString,',');
							myString.erase(std::remove(myString.begin(),myString.end(),')'),myString.end());
							removeSpaces(myString);
							NODE newNode1;
							initializeNode(newNode1);
							newNode1.nodeName = myString;
							newNode1.po = 1;
							newNode1.fout.push_back(tempNode.nodeName);
							newNode1.nfo++;
							const auto & search2 = circuitDescription.insert({myString,newNode1});
							search2.first->second.po = 1;
							const auto & search3 = circuitDescription.insert({tempNode.nodeName,tempNode});
							search3.first->second.type = INPT;
							listPI.push_back(make_pair(tempNode.nodeName,UNKNOWN));
							listPO.push_back(myString);
							break;
						}
						default:
							exit(1);
							
					}
				}
				
				
			}
		}	
	}
	return circuitDescription;
}

void addFromNodes(std::unordered_map<std::string,NODE> &ckt , LIST_OF_PO &listPO){
	NODE tempNode;
	std::unordered_map<std::string,NODE> ckt1;
	std::stringstream stringStream;
	std::string fromName,newName,countStr;
	long long count=0;
	for(auto &search : ckt){
		NODE &n1 = search.second;
		if(n1.nfo > 1){
			auto &listFanout = n1.fout;
			for (auto &l1 : listFanout){
				
				initializeNode(tempNode);
				tempNode.type = FROM;
				tempNode.fin.push_back(n1.nodeName);
				tempNode.fout.push_back(l1);
				tempNode.nfi=1;
				tempNode.nfo=1;
				fromName = n1.nodeName + "->" + l1;
				newName = fromName;
				auto findCopy1 = ckt.find(fromName);
				auto findCopy2 = ckt1.find(fromName);
				while( findCopy1 != ckt.end() || findCopy2 != ckt1.end()){  // Considering a gate with same inputs it will have same fanin names --Need to differentiate
					stringStream << count;
					stringStream >> countStr;
					newName = fromName + "_" + countStr;
					stringStream.clear();
					count++;
					findCopy1 = ckt.find(newName);	
					findCopy2 = ckt1.find(newName);			
				}
				tempNode.nodeName = newName;
				const auto & search1 = ckt.find(l1);
				if(search1 != ckt.end()){
					
					auto  search2 = std::find(search1->second.fin.begin(),search1->second.fin.end(),n1.nodeName);
					if(search2 != (search1->second.fin).end()){
						search1->second.fin.erase(search2);
					}
					search1->second.fin.push_back(tempNode.nodeName);
					if(search1->second.type == INPT){
						continue;					
					}
				}else{
					std::cout<<"Couldnot find the FROM node"<<std::endl;
				}
				l1 = tempNode.nodeName;
				ckt1.insert({tempNode.nodeName,tempNode});
			}
		}
		if(n1.nfo > 0 && n1.po == 1){
			NODE n2;
			initializeNode(n2);
			n2.type = FROM;
			n2.nfi = 1;
			n2.nfo = 0;
			n2.po = 1;
			n2.fin.push_back(n1.nodeName);
			n2.nodeName = n1.nodeName + "->output";
			ckt1.insert({n2.nodeName,n2});
			n1.po = 0;
			n1.fout.push_back(n2.nodeName);
			n1.nfo++;
			auto search3 = std::find(listPO.begin(),listPO.end(),n1.nodeName);
			if( search3 != listPO.end()){
				listPO.erase(search3);
			}
			listPO.push_back(n2.nodeName);
		}
	}				
	ckt.insert(ckt1.begin(),ckt1.end());  //Merge the two maps
}

CIRCUIT_TUPLE topologicalSort(std::unordered_map<std::string,NODE> &ckt , LIST_OF_PI &listPI , LIST_OF_PO &listPO){
	std::unordered_map<std::string,NODE> ckt1 = ckt;
	std::vector<NODE> S;
	std::vector<std::string> L;
	std::vector<NODE> G;

	NODE n1;
	unsigned long long nodeId;
	long long level;
	
	removePI(ckt,ckt1,listPI,S);
	while(!S.empty()){
		n1 = S.back();
		S.pop_back();
		nodeId = L.size();
		L.push_back(n1.nodeName);
		updateFanIO(ckt,n1.nodeName,nodeId);
		level = n1.level;
		for(auto &search : n1.fout){
			const auto &search1 = ckt1.find(search);
			if(search1 != ckt.end()){
				if((search1->second.level < level +1)){
					search1->second.level = level + 1;
					ckt.at(search1->second.nodeName).level = search1->second.level;
				}
				if(search1->second.type == FROM){
					search1->second.level = level;
					ckt.at(search1->second.nodeName).level = search1->second.level;
				}
				auto &fin = search1->second.fin;
				fin.erase(std::remove(fin.begin(),fin.end(),n1.nodeName),fin.end());
				if(fin.empty()){
					S.push_back(search1->second);
					ckt1.erase(search);
				}
			}
		}
	}

	std::unordered_map<std::string, PIO> PI;
	std::unordered_map<std::string, PIO> PO;
	std::string nodeName;
	PIO p1;
	p1.value = UNKNOWN;
	for(auto &search : L){
		const auto & search1 = ckt.find(search);
		if( search1 != ckt.end()){
			nodeName = search1->second.nodeName;
			G.push_back(search1->second);
			p1.id = search1->second.id;
			if(search1->second.type == INPT){
				PI.insert({nodeName, p1});
				if(search1->second.nfo == 0 && search1->second.po ==1 ){
					PO.insert({nodeName, p1});
				}
			}else if (search1->second.po == 1){
				PO.insert({nodeName, p1});
			}
		}
	}
	for(auto &search : G){
		calculatePIConeset(G,search);
	}

	CIRCUIT_TUPLE returnVal = std::make_tuple(G,PI,PO);
	return returnVal;	
	
}

void calculatePIConeset(std::vector<NODE> &G ,NODE &n1){
	std::set<unsigned long long> s1;
	if( n1.type == INPT){
		n1.conePI.insert(n1.id);
	}else{
		for(auto &fin : n1.finAfterTop){
			s1.insert(G[fin].conePI.begin(),G[fin].conePI.end());
		}
		n1.conePI = s1;
	}
} 

void updateFanIO(std::unordered_map<std::string , NODE> &ckt , std::string &nodeName , unsigned long long &nodeId){
	updateFanin(ckt,nodeName,nodeId);
	updateFanout(ckt,nodeName,nodeId);
}

void updateFanin(std::unordered_map<std::string,NODE> &ckt , std::string &nodeName , unsigned long long &nodeId){
	const auto &search = ckt.find(nodeName);
	if(search != ckt.end()){
		search->second.id = nodeId;
		std::vector<std::string> &fout = search->second.fout;
		for(auto &search1 : fout){
			const auto &search3 = ckt.find(search1);
			if(search3 != ckt.end()){
				search3->second.finAfterTop.push_back(nodeId);
			}
		}
	}
}

void updateFanout(std::unordered_map<std::string,NODE> &ckt , std::string &nodeName , unsigned long long &nodeId){
	const auto &search = ckt.find(nodeName);
	if(search != ckt.end()){
		std::vector<std::string> &fin = search->second.fin;
		for(auto &search1 : fin){
			const auto &search2 = ckt.find(search1);
			if(search2 != ckt.end()){
				search2->second.foutAfterTop.push_back(nodeId);			
			}		
		}
	}
}

void removePI(std::unordered_map<std::string,NODE> &ckt, std::unordered_map<std::string,NODE> &ckt1, LIST_OF_PI &listPI, std::vector<NODE> &PI){
	for(auto &search : listPI){
		const auto & search1 = ckt1.find(search.first);
		if(search1 != ckt.end()){
			ckt.at(search1->second.nodeName).level = 0;
			search1->second.level=0;
			PI.insert(PI.begin(),search1->second);
			ckt1.erase(search1);
		}else{
			std::cout<<"COULDNOT FIND PI";
		}
	}	
}

void initializeNode(NODE & tempNode){
	tempNode.nodeName.clear();
	tempNode.type = UNDEF;
	tempNode.nfi= tempNode.nfo = 0;
	tempNode.po = 0;
	tempNode.mark = 0;
	tempNode.cVal = tempNode.fVal=INVALID;
	tempNode.level = -1;
	tempNode.conePI.clear();
	tempNode.fin.clear();
	tempNode.fout.clear();
	tempNode.CC0 = tempNode.CC1 = tempNode.CO = -1;
}

bool checkEmptyLine(std::string currentLine){
	for (unsigned int index = 0; index < currentLine.length(); index++) {
        	if (!std::isspace(currentLine[index])) return false;
   	}
    	return true;		
}

std::string removeLeftSpaces(std::string & myString){
	myString.erase(myString.begin(),std::find_if(myString.begin(),myString.end(),std::not1(std::ptr_fun<int,int>(std::isspace))));
	return myString;
}

std::string removeRightSpaces(std::string & myString){
	myString.erase(std::find_if(myString.rbegin(),myString.rend(),std::not1(std::ptr_fun<int,int>(std::isspace))).base(),myString.end());
	return myString;
}

void removeSpaces(std::string & myString){
	removeLeftSpaces(myString);
	removeRightSpaces(myString);	
}

GATE_TYPE stringToGate(std::string s) {

    if (s == "INPUT" || s == "input")
        return INPT;
    else if (s == "AND" || s == "and")
        return AND;
    else if (s == "NAND" || s == "nand")
        return NAND;
    else if (s == "OR" || s == "or")
        return OR;
    else if (s == "NOR" || s == "nor")
        return NOR;
    else if (s == "XOR" || s == "xor")
        return XOR;
    else if (s == "XNOR" || s == "xnor")
        return XNOR;
    else if (s == "BUFF" || s == "buff")
        return BUFF;
    else if (s == "NOT" || s == "not")
        return NOT;
    else if (s == "FROM" || s == "from")
        return FROM;
    else if (s == "DFF" || s == "dff")
        return DFF;
    else
        return UNDEF;
}

void openCircuitFile(std::ifstream &benchFile, int argc, char **argv){
	if(argc < 2){
		std::cout<<"Please Provide Circuit Filename";
	}
	benchFile.open(argv[1]);
	if(!benchFile.is_open()){
		std::cout<<"ERROR IN OPENING FILE"<<argv[1]<<std::endl;	
	}
}


void printCircuitDescription(std::vector<NODE> &ckt){
	std::cout<<"ID\tLEVEL\tNODE_NAME\tCC0/CC1\tCO\tFIN\tFOUT\tPI_CONESET"<<std::endl;
	for(const auto &search : ckt){
		std::cout<<search.id<<"\t"<<search.level<<"\t"<<search.nodeName<<"\t\t"<<search.CC0<<"/"<<search.CC1<<"\t"<<search.CO<<"\t";
		for(auto &fin : search.finAfterTop){
			std::cout<<fin<<" ";
		}
		std::cout<<"\t";
		for(auto &fout : search.foutAfterTop){
			std::cout<<fout<<" ";
		}
		std::cout<<"\t";
		for(auto &fin : search.conePI){
			std::cout<<fin<<" ";
		}
		std::cout<<std::endl;
	}
}

void printCircuit(std::unordered_map<std::string,NODE> &ckt){
	std::cout<<"NODE_NAME\tNODE_TYPE\tFIN\t\tFOUT"<<std::endl;
	for(auto &search: ckt){
		std::cout<<search.first<<"\t\t"<<search.second.type<<"\t\t";
		for(auto &fin : search.second.fin){
			std::cout<<fin<<" ";
		}
		std::cout<<"\t\t";
		for(auto &fout : search.second.fout){
			std::cout<<fout<<" ";
		}
		std::cout<<std::endl;	
	}
}

