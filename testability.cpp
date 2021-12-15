#include "podem.h"

void calculateTestabilityMeasures(std::vector<NODE> & G){
	//CC0 and CC1 ->Combinational Controllability
	for(auto &search : G){
		switch(search.type){
			case INPT:{
				search.CC0 = search.CC1 = 1;
				break;
			}
			case AND:{
				search.CC0 = minControllabilityToZero(G , search) + 1;
				search.CC1 = faninControllabilitySumOfOne(G , search) + 1;
				break;
			}
			case NAND:{
				search.CC0 = faninControllabilitySumOfOne(G , search) + 1;
				search.CC1 = minControllabilityToZero(G , search) + 1;
				break;
			}
			case OR:{
				search.CC0 = faninControllabilitySumOfZero(G,search) + 1;
				search.CC1 = minControllabilityToOne(G,search) + 1;
				break;
			}
			case NOR:{
				search.CC0 = minControllabilityToOne(G,search) + 1;
				search.CC1 = faninControllabilitySumOfZero(G,search) + 1;
			}
			case XOR:{  //only for 2 input X-OR gate
				search.CC0 = min(faninControllabilitySumOfOne(G,search),faninControllabilitySumOfZero(G,search)) + 1;
				search.CC1 = min(faninControllabilitySumAB(G,search),faninControllabilitySumBA(G,search)) + 1;
			}
			case XNOR:{ 
				search.CC0 = min(faninControllabilitySumAB(G,search),faninControllabilitySumBA(G,search)) + 1;
				search.CC1 = min(faninControllabilitySumOfOne(G,search),faninControllabilitySumOfZero(G,search)) + 1;
			}
			case NOT:{
				search.CC0 = G[search.finAfterTop.back()].CC1 +1;
				search.CC1 = G[search.finAfterTop.back()].CC0 +1;
				break;
			}
			case BUFF:{
				search.CC0 = G[search.finAfterTop.back()].CC0 +1;
				search.CC1 = G[search.finAfterTop.back()].CC1 +1;
				break;
			}
			case FROM:{
				search.CC0 = G[search.finAfterTop.back()].CC0;
				search.CC1 = G[search.finAfterTop.back()].CC1;
				break;
			}
			default:
				break;
		}
	}
	//CO -> Combinational Observability
	int sum;
	for(int i = G.size(); i>=0 ; --i){
		if(G[i].po){
			G[i].CO = 0;
		}
		switch(G[i].type){
			case AND:
			case NAND:{
				checkFromNode(G,i);
				for(auto &search : G[i].finAfterTop){
					sum=0;
					for(auto &search1 : G[i].finAfterTop){
						if(search == search1 ){ continue;}
						else{
							sum += G[search1].CC1;
						}
					}
					G[search].CO = G[i].CO + sum + 1;
				}
				break;
			}
			case OR:
			case NOR:{
				checkFromNode(G,i);
				for(auto &search : G[i].finAfterTop){
					sum=0;
					for(auto &search1 : G[i].finAfterTop){
						if(search == search1 ){ continue;}
						else{
							sum += G[search1].CC0;
						}
					}
					G[search].CO = G[i].CO + sum + 1;
				}
				break;
			}
			case XOR: // 2-input XOR gate
			case XNOR:{
				checkFromNode(G,i);
				for(auto &search : G[i].finAfterTop){
					sum = 0;
					for(auto &search1 : G[i].finAfterTop){
						if(search == search1){ continue;}
						else{
							sum = min(G[search1].CC0,G[search1].CC1);
						}
					}
					G[search].CO = G[i].CO + sum + 1;
				}
			}
			case NOT:
			case BUFF:{
				checkFromNode(G,i);
				G[G[i].finAfterTop.back()].CO = G[i].CO + 1;
				break;
			}
			case FROM:{
				G[G[i].finAfterTop.back()].CO = G[i].CO;
				break;
			}
			default:
				break;
		}	
	}
	//Arrange fin in the order of increasing controllability value
	/*auto compare1 = [&G]( unsigned long long x , unsigned long long y){ return G[x].CC0 <= G[y].CC0;};
	auto compare2 = [&G]( unsigned long long x , unsigned long long y){ return G[x].CC1 <= G[y].CC1;};
	std::set<unsigned long long, decltype(compare1)> ascendOrderCC0(compare1);
	std::set<unsigned long long, decltype(compare2)> ascendOrderCC1(compare2);
	
	for(auto & search :G){
		for(auto &fin : G[search.id].finAfterTop){
			ascendOrderCC0.insert(fin);
			ascendOrderCC1.insert(fin);		
		}
		G[search.id].ascendOrderCC0.assign(ascendOrderCC0.begin(),ascendOrderCC0.end());
		ascendOrderCC0.clear();
		ascendOrderCC1.clear();
	}
	for (auto & search : G){
		for(auto & id : G[search.id].ascendOrderCC0){
			std::cout<< id << " ";		
		}
		std::cout<<std::endl;	
	}*/		
}

void checkFromNode(std::vector<NODE> &G , unsigned long long i){
	int min;
	if(!G[i].foutAfterTop.empty()){
		min = G[G[i].foutAfterTop.back()].CO;
		for(auto &fout :G[i].foutAfterTop){
			if(G[fout].type == FROM){
				if(min > G[fout].CO){
					min = G[fout].CO;
				}
				G[i].CO = min;
			}
			//break;
		}
	}
}

int min( int a , int b){
	if(a<b) return a;
	return b;
}

int faninControllabilitySumAB(std::vector<NODE> &G, NODE & n1){
	int i,sum,val1,val2;
	i=sum=0;
	for(auto &search: n1.finAfterTop){
		if(i==0){
			val1 = G[search].CC1;
		}else{
			val2 = G[search].CC0;
		}
		i++;
	}
	sum =val1 + val2;
	return sum;
}

int faninControllabilitySumBA(std::vector<NODE> &G, NODE & n1){
	int i,sum,val1,val2;
	i=sum=0;
	for(auto &search: n1.finAfterTop){
		if(i==0){
			val1 = G[search].CC0;
		}else{
			val2 = G[search].CC1;
		}
		i++;
	}
	sum =val1 + val2;
	return sum;
}

int minControllabilityToZero(std::vector<NODE> & G, NODE & n1){
	int min,val;
	min = G[n1.finAfterTop.front()].CC0;
	for(auto &search: n1.finAfterTop){
		val = G[search].CC0;
		if(val<min){
			min = val;
		}
	}
	return min;
}

int minControllabilityToOne(std::vector<NODE> & G, NODE & n1){
	int min,val;	
	min = G[n1.finAfterTop.front()].CC1;
	for(auto &search: n1.finAfterTop){
		val = G[search].CC1;
		if(val<min){
			min =val;
		}
	}
	return min;
}

int faninControllabilitySumOfOne(std::vector<NODE> & G, NODE & n1){
	int retVal=0;
	for( auto &search : n1.finAfterTop){
		retVal += G[search].CC1;
	}
	return retVal;
}

int faninControllabilitySumOfZero(std::vector<NODE> & G, NODE & n1){
	int retVal=0;
	for( auto &search : n1.finAfterTop){
		retVal += G[search].CC0;
	}
	return retVal;
}

