#include <math.h>
#include <string.h>
#include <iostream>
#include "BinaryHeap.h"
#include "mex.h"

#define IND(r, c) ((r)+(c)*height)
#define MAX(x, y) (((x)>(y))?(x):(y))
#define MIN(x, y) (((x)<(y))?(x):(y))
#define DBG(x) {printf("-%i-\n",x);}

using namespace std;

struct Pos {
public:
    int row, col, width, height, index;
    double *grid;
    inline double getT() const { return grid[row+col*height];}
    Pos() :row(-1), col(-1), width(-1), height(-1), grid(0), index(0){}
    Pos(int r, int c, int w, int h, double *g) : row(r), col(c), width(w), height(h), grid(g), index(0) {}
    
    inline bool operator<(const Pos& rhs) const  {return  getT()<rhs.getT();}
};


inline void update(int i, int j, int width, int height, double* T, double *F) {
    double T1=MIN(T[IND(i-1, j)], T[IND(i+1, j)]);
    double T2=MIN(T[IND(i,j-1)], T[IND(i, j+1)]);
    double t;
      //  double T1=MIN(T[IND(MAX(i-1, 0), j)], T[IND(MIN(i+1, height-1), j)]);
  //  double T2=MIN(T[IND(i, MAX(j-1, 0))], T[IND(i, MIN(j+1, width-1))]);

    
    if (abs(T1-T2)<F[IND(i, j)])
        t=(T1+T2+sqrt(2*F[IND(i, j)]*F[IND(i, j)]-(T1-T2)*(T1-T2)))/2;
    else
        t=MIN(T1, T2)+F[IND(i, j)];
    T[IND(i, j)]=MIN(T[IND(i, j)], t);
}


//function [T] = fmm (F, src, t0, display)
//rhs: double*F, int width, int height, int r0,int c0, double t0, bool display
void mexFunction(int nlhs, mxArray *plhs[ ], int nrhs, const mxArray *prhs[ ]) {
    
    BinaryHeap<Pos> Q;
    
    double *F=mxGetPr(prhs[0]);
    int height=mxGetScalar(prhs[1]);
    int width=mxGetScalar(prhs[2]);
    int r0=mxGetScalar(prhs[3])-1;//MATLAB has indexes starting from 1 whereas c/c++ from 0
    int c0=mxGetScalar(prhs[4])-1;
    double t0=mxGetScalar(prhs[5]);
    bool display=mxGetScalar(prhs[6]);
    plhs[0] = mxCreateDoubleMatrix(height, width, mxREAL);
    double *T = mxGetPr(plhs[0]);
    Pos* *S=(Pos* *)mxMalloc(width*height*sizeof(Pos*));
    Pos* GREEN=new Pos();
    Pos* BURNT = new Pos();
    Pos *u,*s,*tmp;
   
	double inf=exp(100.0);
	for (int i=0;i<width*height;i++) {
		S[i]=GREEN;
        if (F[i]>exp(100.0)) S[i]=BURNT;
		T[i]=inf;
	}

	T[IND(r0,c0)]=t0;

	//Add first position to the Q
	tmp=new Pos(r0, c0, width, height, T);
	Q.insert(tmp);
	S[IND(r0, c0)]=BURNT;

	int R[]={-1,1,0,0};
	int C[]={0,0,-1,1};

	int count=0;
    double Ttmp;
	while (Q.size()) {
		if (display ) printf("\nQ:%i\n",Q.size());
		//compare might need changing around
		Q.deleteMin(u); //Stores removed smallest element in u
		S[IND(u->row, u->col)]=BURNT;


		for (int ind=0;ind<4;ind++)
		{
			int r=u->row+R[ind],c=u->col+C[ind];

				if (S[IND(r, c)] != BURNT){
                Ttmp=T[IND(r,c)];
					update(r, c, width, height, T, F);

                    if (Ttmp!=T[IND(r,c)]) {
					if (S[IND(r, c)]==GREEN) {
						tmp=new Pos(r, c, width, height, T);
						S[IND(r, c)]=Q.insert(tmp);
					}
					else  //It is burning
						Q.reorder(S[IND(r, c)]);						
				}
                }
		}
		delete u;
	}

	delete GREEN;
	delete BURNT;
	mxFree(S);	
}
