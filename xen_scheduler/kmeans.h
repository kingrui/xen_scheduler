#ifndef KMEANS_H
#define KMEANS_H
#define KMEANS_H_DEBUG

class node{
public:
	const static int n =2;						//node dimensions
	static double* w;							//weight for each dimension
	static double* initilizeW();				//initilize w

	double *value;								//node values, store in array 
	double getDistance(node &point);			//get the distance between two nodes
	double getValue();							//get the distance to zero pointer
	bool   equals(node &point);					//test if two pinter eauals in all dimensions

	node(double value[]);						//constructor
	~node();									//destructor

};


class kmeans
{
private:
	int K;										//K clusters
	int minDistance;							//minimun distance required between two cluster center
	static const int maxRound = 100;			//max iterate number

public:
	~kmeans(void);
	kmeans(int K,int minDistance);				//constructor
	void runKmeans(node *elements[],int element_class[],int elementCount);
};

#endif