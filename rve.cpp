#include "podem.h"

void compactTestSet(std::vector<NODE> &G, std::vector<unsigned long long> &PI_order, std::unordered_map<std::string,PIO> &PO, std::vector<FAULT_NODE> &faultList, std::vector<std::string> &testVector){
	PATTERN temp;
	std::vector<PATTERN> testSet;
	std::set<unsigned long long> detectedFaultList;
	for(auto &tVec : testVector){
		detectedFaultList= dfs(G,tVec,PI_order,PO,faultList);
//		show(detectedFaultList);
		temp.testVector = tVec;
		std::cout<<temp.testVector<<"\n";
		temp.faults = detectedFaultList;
		show(temp.faults);
		temp.eCount = 0;
		rve(faultList,testSet,temp);
//		showFaultCount(faultList);
		
	}
	for(auto &test: testSet){
		std::cout<<test.testVector<<":\n";
		show(test.faults);
		std::cout<<"NE:"<<test.eCount<<"\n";		
	}
	std::cout<<"********"<<testSet.size()<<"**************\n";
}

void showFaultCount(std::vector<FAULT_NODE> &faultList){
	for(auto &flt : faultList){
		std::cout<<flt.faultId<<" : "<<flt.dCount<<"\n";	
	}
}

void rve(std::vector<FAULT_NODE> &faultList ,std::vector<PATTERN> &testSet, PATTERN &tPattern){
	long long id,nextId;
	id=testSet.size();
	testSet.push_back(tPattern);
	for(auto &dFault : tPattern.faults){
		(faultList[dFault].dCount)++;
		if(faultList[dFault].dCount == 1){
			(testSet[id].eCount)++;
		}else if(faultList[dFault].dCount == 2){
			nextId=findPattern(testSet,tPattern.testVector,dFault);
			(testSet[nextId].eCount)--;
			if(testSet[nextId].eCount == 0){
				deleteFromTestSet(testSet,faultList,testSet[nextId]);
				id -= 1;  //reduce the count to account for deleted vector		
			}		
		}	
	}
	if(testSet[id].eCount == 0){
		deleteFromTestSet(testSet,faultList,testSet[id]);	
	}
}


long long findPattern(std::vector<PATTERN> &testSet, std::string &tPattern, const unsigned long long &dFault){
	long long count,retVal;
	count=0;
	retVal=-1;
	for(auto &tVec : testSet){
		if(tVec.testVector == tPattern){ 
			count++;
			continue;
		}
		std::set<unsigned long long>::iterator search = tVec.faults.find(dFault);
		if(search != tVec.faults.end()){
			retVal = count;
			break;
		}
		count++;
	}
	if(retVal == -1){
		std::cout<<tPattern<<"****"<<dFault<<"\n";
		std::cout<<"ERROR IN RVE"<<"\n";
		exit(1);	
	}
	return retVal;
}

void deleteFromTestSet(std::vector<PATTERN> &testSet, std::vector<FAULT_NODE> &faultList, PATTERN &tPattern){
	std::set<unsigned long long> fList;
	unsigned long long id;
	std::string tempPat;
	tempPat = tPattern.testVector;
	fList = tPattern.faults;
	testSet.erase(std::remove_if(testSet.begin(),testSet.end(),[&tPattern](PATTERN const &val){return val.testVector == tPattern.testVector;}),testSet.end());
	for(auto &dFault:fList){
		(faultList[dFault].dCount)--;
		if(faultList[dFault].dCount == 1){
			id = findPattern(testSet,tempPat,dFault);
			(testSet[id].eCount)++;		
		}	
	}
}

