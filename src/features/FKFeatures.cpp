#include "features/FKFeatures.h"
#include "features/StringFeatures.h"
#include "lib/io.h"
#include <assert.h>

CFKFeatures::CFKFeatures(LONG size, CHMM* p, CHMM* n) : CRealFeatures(size)
{
	pos_prob=NULL;
	neg_prob=NULL;
	weight_a=-1;
	set_models(p,n);
}

 CFKFeatures::CFKFeatures(const CFKFeatures &orig): 
	CRealFeatures(orig), pos(orig.pos), neg(orig.neg), weight_a(orig.weight_a)
{ 
}

CFKFeatures::~CFKFeatures()
{
}

double CFKFeatures::deriv_a(double a, INT dimension)
{
	CStringFeatures<WORD> *Obs=pos->get_observations() ;
	double deriv=0.0 ;
	INT i=dimension ;

	if (dimension==-1)
	{
		for (i=0; i<Obs->get_num_vectors(); i++)
		{
			//double pp=pos->model_probability(i) ;
			//double pn=neg->model_probability(i) ;
			double pp=(pos_prob) ? pos_prob[i] : pos->model_probability(i);
			double pn=(neg_prob) ? neg_prob[i] : neg->model_probability(i);
			double sub=pp ;
			if (pn>pp) sub=pn ;
			pp-=sub ;
			pn-=sub ;
			pp=exp(pp) ;
			pn=exp(pn) ;
			double p=a*pp+(1-a)*pn ;
			deriv+=(pp-pn)/p ;

			/*double d1=(pp-pn)/p ;
			  pp=exp(pos->model_probability(i)) ;
			  pn=exp(neg->model_probability(i)) ;
			  p=a*pp+(1-a)*pn ;
			  double d2=(pp-pn)/p ;
			  fprintf(stderr, "d1=%e  d2=%e,  d1-d2=%e\n",d1,d2) ;*/
		} ;
	} else
	{
		double pp=pos->model_probability(i) ;
		double pn=neg->model_probability(i) ;
		double sub=pp ;
		if (pn>pp) sub=pn ;
		pp-=sub ;
		pn-=sub ;
		pp=exp(pp) ;
		pn=exp(pn) ;
		double p=a*pp+(1-a)*pn ;
		deriv+=(pp-pn)/p ;
	} ;

	return deriv ;
} ;


double CFKFeatures::set_opt_a(double a)
{
	if (a==-1)
	{
		CIO::message(M_INFO, "estimating a.\n");
		pos_prob=new double[pos->get_observations()->get_num_vectors()];
		neg_prob=new double[pos->get_observations()->get_num_vectors()];
		assert(pos_prob!=NULL);
		assert(neg_prob!=NULL);
		for (INT i=0; i<pos->get_observations()->get_num_vectors(); i++)
		{
			pos_prob[i]=pos->model_probability(i) ;
			neg_prob[i]=neg->model_probability(i) ;
		}

		double la=0;
		double ua=1;
		a=(la+ua)/2;
		while (ua-la>1e-6)
		{
			double da=deriv_a(a);
			if (da>0)
				la=a;
			if (da<=0)
				ua=a;
			a=(la+ua)/2;
			CIO::message(M_INFO, "opt_a: a=%1.3e  deriv=%1.3e  la=%1.3e  ua=%1.3e\n", a, da, la ,ua);
		}
		delete[] pos_prob;
		delete[] neg_prob;
		pos_prob=NULL;
		neg_prob=NULL;
	}

	weight_a=a;
	CIO::message(M_INFO, "setting opt_a: %g\n", a);
	return a;
}

void CFKFeatures::set_models(CHMM* p, CHMM* n)
{
	assert(p!=NULL && n!=NULL);

	pos=p; 
	neg=n;
	set_num_vectors(0);

	delete[] feature_matrix  ;
	feature_matrix=NULL ;

	CIO::message(M_INFO, "pos_feat=[%i,%i,%i,%i],neg_feat=[%i,%i,%i,%i]\n", pos->get_N(), pos->get_N(), pos->get_N()*pos->get_N(), pos->get_N()*pos->get_M(), neg->get_N(), neg->get_N(), neg->get_N()*neg->get_N(), neg->get_N()*neg->get_M()) ;

	if (pos && pos->get_observations())
		set_num_vectors(pos->get_observations()->get_num_vectors());
	if (pos && neg)
		num_features=1+pos->get_N()*(1+pos->get_N()+1+pos->get_M()) + neg->get_N()*(1+neg->get_N()+1+neg->get_M()) ;
}

CFeatures* CFKFeatures::duplicate() const
{
	return new CFKFeatures(*this);
}

REAL* CFKFeatures::compute_feature_vector(INT num, INT &len, REAL* target)
{
  REAL* featurevector=target;
  
  if (!featurevector)
	featurevector=new REAL[ 1+pos->get_N()*(1+pos->get_N()+1+pos->get_M()) + neg->get_N()*(1+neg->get_N()+1+neg->get_M()) ];
  
  if (!featurevector)
    return NULL;
  
  compute_feature_vector(featurevector, num, len);

  return featurevector;
}

void CFKFeatures::compute_feature_vector(REAL* featurevector, INT num, INT& len)
{
	INT i,j,p=0,x=num;

	double posx=pos->model_probability(x);
	double negx=neg->model_probability(x);

	len=1+pos->get_N()*(1+pos->get_N()+1+pos->get_M()) + neg->get_N()*(1+neg->get_N()+1+neg->get_M());

	featurevector[p++] = deriv_a(weight_a, x);
	double px=CMath::logarithmic_sum(posx+log(weight_a),negx+log(1-weight_a)) ;

	//first do positive model
	for (i=0; i<pos->get_N(); i++)
	{
		featurevector[p++]=weight_a*exp(pos->model_derivative_p(i, x)-px);
		featurevector[p++]=weight_a*exp(pos->model_derivative_q(i, x)-px);

		for (j=0; j<pos->get_N(); j++) {
			featurevector[p++]=weight_a*exp(pos->model_derivative_a(i, j, x)-px);
		}

		for (j=0; j<pos->get_M(); j++) {
			featurevector[p++]=weight_a*exp(pos->model_derivative_b(i, j, x)-px);
		} 

	}

	//then do negative
	for (i=0; i<neg->get_N(); i++)
	{
		featurevector[p++]= (1-weight_a)*exp(neg->model_derivative_p(i, x)-px);
		featurevector[p++]= (1-weight_a)* exp(neg->model_derivative_q(i, x)-px);

		for (j=0; j<neg->get_N(); j++) {
			featurevector[p++]= (1-weight_a)*exp(neg->model_derivative_a(i, j, x)-px);
		}

		for (j=0; j<neg->get_M(); j++) {
			featurevector[p++]= (1-weight_a)*exp(neg->model_derivative_b(i, j, x)-px);
		}
	}
}

REAL* CFKFeatures::set_feature_matrix()
{
	INT len=0;

	num_features=1+ pos->get_N()*(1+pos->get_N()+1+pos->get_M()) + neg->get_N()*(1+neg->get_N()+1+neg->get_M());

	num_vectors=pos->get_observations()->get_num_vectors();
	CIO::message(M_INFO, "allocating FK feature cache of size %.2fM\n", sizeof(double)*num_features*num_vectors/1024.0/1024.0);
	delete[] feature_matrix;
	feature_matrix=new REAL[num_features*num_vectors];

	CIO::message(M_INFO, "calculating FK feature matrix\n");

	for (INT x=0; x<num_vectors; x++)
	{
		if (!(x % (num_vectors/10+1)))
			printf("%02d%%.", (int) (100.0*x/num_vectors));
		else if (!(x % (num_vectors/200+1)))
			printf(".");

		compute_feature_vector(&feature_matrix[x*num_features], x, len);
	}

	printf(".done.\n");
	
	num_vectors=get_num_vectors() ;
	num_features=get_num_features() ;

	return feature_matrix;
}
