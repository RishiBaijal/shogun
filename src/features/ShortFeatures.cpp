#include "features/ShortFeatures.h"
#include "features/CharFeatures.h"

CShortFeatures::CShortFeatures(LONG size) : CSimpleFeatures<SHORT>(size)
{
}

CShortFeatures::CShortFeatures(const CShortFeatures & orig) : CSimpleFeatures<SHORT>(orig)
{
}

CShortFeatures::CShortFeatures(CHAR* fname) : CSimpleFeatures<SHORT>(fname)
{
}

bool CShortFeatures::obtain_from_char_features(CCharFeatures* cf, INT start, INT order)
{
	assert(cf);

	num_vectors=cf->get_num_vectors();
	num_features=cf->get_num_features();

	INT len=num_vectors*num_features;
	delete[] feature_matrix;
	feature_matrix=new SHORT[len];
	assert(feature_matrix);

	INT num_cf_feat;
	INT num_cf_vec;

	CHAR* fm=cf->get_feature_matrix(num_cf_feat, num_cf_vec);

	assert(num_cf_vec==num_vectors);
	assert(num_cf_feat==num_features);

	INT max_val=0;
	for (INT i=0; i<len; i++)
	{
		feature_matrix[i]=(SHORT) cf->remap(fm[i]);
		max_val=CMath::max((INT) feature_matrix[i],max_val);
	}

	for (INT line=0; line<num_vectors; line++)
		translate_from_single_order(&feature_matrix[line*num_features], num_features, start, order, max_val);

	return true;
}


void CShortFeatures::translate_from_single_order(SHORT* obs, INT sequence_length, INT start, INT order, INT max_val)
{
	INT i,j;
	SHORT value=0;

	for (i=sequence_length-1; i>= ((int) order)-1; i--)	//convert interval of size T
	{
		value=0;
		for (j=i; j>=i-((int) order)+1; j--)
			value= (value >> max_val) | (obs[j] << (max_val * (order-1)));
		
		obs[i]=value;
	}

	for (i=order-2;i>=0;i--)
	{
		value=0;
		for (j=i; j>=i-order+1; j--)
		{
			value= (value >> max_val);
			if (j>=0)
				value|=obs[j] << (max_val * (order-1));
		}
		obs[i]=value;
	}

	for (i=start; i<sequence_length; i++)	
		obs[i-start]=obs[i];
}

CFeatures* CShortFeatures::duplicate() const
{
	return new CShortFeatures(*this);
}

bool CShortFeatures::load(CHAR* fname)
{
	return false;
}

bool CShortFeatures::save(CHAR* fname)
{
	return false;
}
