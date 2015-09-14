#include "kmeans.h"
#include <cmath>
#include <iostream>
using namespace std;

//#define KMEANS_DEBUG
node::node(double value[]){
	this->value = value;
}

double* node::initilizeW(){
	static double* w = new double[node::n];
	/*
	for(int i=0;i<node::n;i++){
		w[i] = 1.0;
	}
	*/
	w[0] = 1.0;
	w[1] = 1.0;
	return w;
}

double* node::w = node::initilizeW();

node::~node(){}

double node::getDistance(node &point){
	double distance = 0;
	for(int i=0;i<n;i++){
		distance += w[i]*(point.value[i]-this->value[i])*(point.value[i]-this->value[i]);
	}
	return sqrt(distance);
}

double node::getValue(){
	double distance = 0;
	for(int i=0;i<n;i++){
		distance += w[i]*this->value[i]*this->value[i];
	}
	return sqrt(distance);
}

bool node::equals(node &pointer){
	for(int i=0;i<n;i++){
		if(this->value[i]!=pointer.value[i]) return false;
	}
	return true;
}

kmeans::kmeans(int K,int minDistance){
	this->K = K;
	this->minDistance = minDistance;
}

void kmeans::runKmeans(node *elements[],int element_class[],int elementCount){
	node **u = new node*[K];//store center
	int *countU = new int[K];//the number of every class
	int dimension  = elements[0]->n;//get the dimensions of node
	if(K>elementCount){
		cout<<"Error: K is bigger than n"<<endl;
		return;
	}

	u[0] = u[K-1] = elements[0];
	for(int i=1;i<elementCount ;i++)//set v0 the min pointer and vk-1 the max pointer
	{
		if(u[0]->getValue()>elements[i]->getValue())
			u[0] = elements[i];
		if(u[K-1]->getValue()<elements[i]->getValue())
			u[K-1] = elements[i];
	}

	for(int i=1;i<K-1;i++){
		int r = rand()%(K-1);
		u[i] = elements[r];
	}

	node **newU = new node*[K];//for newer center
	int count = 0;
	while(true){
		if(count++>maxRound)
			break;
		for(int i=0;i<elementCount;i++){
			//initialy set u0 is the nearest
			double min = elements[i]->getDistance(*u[0]);
			element_class[i] = 0;

			//find the nearest pointer
			for(int j=1;j<K;j++){
				double dis = elements[i]->getDistance(*u[j]);
				if(dis < min){
					min = dis;
					element_class[i] = j;
				}
				else if(dis ==min){
					int r = rand()%100;
					if(r<50){
						min = dis;
						element_class[i] = j;
					}
				}
			}
		}
#ifdef KMEANS_DEBUG
	cout<<"count"<<count<<":"<<endl;
	for(int i=0;i<K;i++){
		cout<<u[i]->value[0]<<","<<u[i]->value[1]<<endl;
	}
#endif


		//initialize countU[]
		for(int i=0;i<K;i++){
			countU[i] = 0;
			double *value = new double[dimension];
			for(int j = 0;j<dimension;j++){
				value[j] = 0;
			}
			newU[i] = new node(value);
		}

		

		//update u for every cluster
		for(int i=0;i<elementCount;i++){
			countU[element_class[i]]++;
			for(int j=0;j<dimension;j++){
				newU[element_class[i]]->value[j]+= elements[i]->value[j];
			}
			
		}

		
		bool isConverge = true;
		for(int i=0;i<K;i++){
			if(countU[i]!=0){
				for(int j=0;j<dimension;j++){
					newU[i]->value[j]/= countU[i];
				}
				
			}
				
			if(!newU[i]->equals(*u[i])){
				isConverge = false;
			}
			u[i] = newU[i];
		}

		//if two clusters' distance is less than minDistance, then we merge them
		bool need_merge = false;
		for(int i=0;i<K;i++){
			for(int j=0;j<K-1;j++){
				if(i==j) continue;
				if(u[i]->getDistance(*u[j])<=minDistance){
					if(i!=K-1){
						u[i] = u[K-1];
					}
					need_merge = true;
					break;
				}
			}
			if(need_merge){
				isConverge = false;
				if(K>=2){
					K--;
					break;
				}
				break;
			}

		}
		if(isConverge) break;
	}
}

kmeans::~kmeans(void){}