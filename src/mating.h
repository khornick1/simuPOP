/***************************************************************************
 *   Copyright (C) 2004 by Bo Peng                                         *
 *   bpeng@rice.edu
 *                                                                         *
 *   $LastChangedDate$
 *   $Rev$                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _MATING_H
#define _MATING_H
/**
\file
\brief head file of class mating and its subclasses
*/
#include "utility.h"
// for trajectory simulation functions
#include "misc.h"
#include "simuPOP_cfg.h"
#include "population.h"
#include "operator.h"

#include <string>
#include <algorithm>
using std::max_element;
using std::string;

#include <stack>
using std::stack;

namespace simuPOP
{
	/// the default method to generate offspring from parents
	///
	/// This part is separated from the mating schemes, because mating schemes
	/// usually only differ by the way parents are choosing.
	///
	/// input: parents,
	/// output: offsprings
	class offspringGenerator
	{
		public:
			/// constructor, save information from pop and ops to speed up
			/// the calls to generateOffspring
			offspringGenerator(const population& pop, vector<Operator *>& ops);

			/// generate numOff offspring, or until reach offEnd
			/// this is because offBegin+numOff may go beyond subpopulation boundary.
			/// return the ending iterator
			void generateOffspring(population& pop, individual* dad, individual* mom, UINT numOff,
				population::IndIterator& offBegin);

			/// generate numOff offspring, or until reach offEnd
			/// this is because offBegin+numOff may go beyond subpopulation boundary.
			/// return the ending iterator
			void copyOffspring(population& pop, individual* par, UINT numOff,
				population::IndIterator& offBegin);

		private:
			bool formOffspringGenotype();

		public:
			// use bernullitrisls with p=0.5 for free recombination
			BernulliTrials m_bt;

			/// cache during-mating operators
			/// we do not cache pop since it may be changed during mating.
			vector<Operator *>& m_ops;

			/// see if who will generate offspring genotype
			bool m_formOffGenotype;

			/// sex chromosome handling
			bool m_hasSexChrom;

			/// cache ploidy
			bool m_ploidy;

			// cache chromBegin, chromEnd for better performance.
			vectoru m_chIdx;
	};

	/**
	The mating classes describe various mating scheme --- a required parameter
	of simulator.

	*/

	class mating
	{
		public:

			/// numOffspring: constant, numOffspringFunc: call each time before mating
#define MATE_NumOffspring           1
			/// call numOffspringFunc each time during mating.
#define MATE_NumOffspringEachFamily 2
			/// numOffspring and numOffsrpingsFunc call each time before mating is
			/// the p for a geometric distribution
#define MATE_GeometricDistribution   3
#define MATE_PoissonDistribution     4
#define MATE_BinomialDistribution    5
			/// uniform between numOffspring and maxNumOffspring
#define MATE_UniformDistribution     6

		public:
			/// check if the mating type is compatible with population structure
			/**  possible things to check:
			 -  need certain types of individual (age, sex etc)
			 -  need resizeable population...
			*/
			virtual bool isCompatible(const population& pop) const
			{
				return true;
			}

			/// constructor
			mating(double numOffspring=1.0,
				PyObject* numOffspringFunc=NULL,
				UINT maxNumOffspring = 0,
				UINT mode=MATE_NumOffspring,
				vectorlu newSubPopSize=vectorlu(),
				string newSubPopSizeExpr="",
				PyObject* newSubPopSizeFunc=NULL);

			mating(const mating& rhs)
				: m_numOffspring(rhs.m_numOffspring),
				m_numOffspringFunc(rhs.m_numOffspringFunc),
				m_maxNumOffspring(rhs.m_maxNumOffspring),
				m_mode(rhs.m_mode), m_firstOffspring(true),
				m_subPopSize(rhs.m_subPopSize),
				m_subPopSizeExpr(rhs.m_subPopSizeExpr),
				m_subPopSizeFunc(rhs.m_subPopSizeFunc)
			{
				if( m_subPopSizeFunc != NULL)
					Py_INCREF(m_subPopSizeFunc);
				if( m_numOffspringFunc != NULL)
					Py_INCREF(m_numOffspringFunc);
			}

			/// destructor
			virtual ~mating()
			{
				if( m_subPopSizeFunc != NULL)
					Py_DECREF(m_subPopSizeFunc);
				if( m_numOffspringFunc != NULL)
					Py_DECREF(m_numOffspringFunc);
			}

			/// clone() const. Generate a copy of itself and return a pointer
			/**
			This function is important because Python automatically
			release an object after it is used.

			For example:
			\code
			  foo( mate = mating() )
			  // in C++ implementation, foo keeps a pointer to mating()
			  // mating * p = mate;
			  foo.p->fun()
			\endcode
			will fail since mating() is released after the first line
			being executed.

			With the help of clone() const, the C++ implementation can avoid this problem by
			\code
			foo( mating* mate = &mating() )
			{
			mating * p = mate.clone() const;
			}
			\endcode
			*/
			virtual mating* clone() const
			{
				return new mating(*this);
			}

			/// return name of the mating type
			/// used primarily in logging.
			virtual string __repr__()
			{
				return "<simuPOP::generic mating scheme>";
			}

			virtual void submitScratch(population& pop, population& scratch)
			{
			}

			/// mate: This is not supposed to be called for base mating class.
			/**
			\param pop population
			\param scratch scratch population
			\param ops during mating operators
			\return return false when mating fail.
			*/
			virtual bool mate( population& pop, population& scratch, vector<Operator* >& ops, bool submit)
			{
				throw SystemError("You are not supposed to call base mating scheme.");
			}

			bool fixedFamilySize();

			ULONG numOffspring(int gen );

			void resetNumOffspring()
			{
				m_firstOffspring =  true;
			}

		public:

			/// dealing with pop/subPop size change, copy of structure etc.
			void prepareScratchPop(population& pop, population& scratch);

		protected:

			/// number of offspring each mate
			double m_numOffspring;

			/// number of offspring func
			PyObject* m_numOffspringFunc;

			///
			UINT m_maxNumOffspring;

			/// whether or not call m_numOffspringFunc each time
			UINT m_mode;

			///
			bool m_firstOffspring;

			/// new subpopulation size. mostly used to 'keep' subPopsize
			/// after migration.
			vectorlu m_subPopSize;

			/// expression to evaluate subPopSize
			/// population size can change as a result of this
			/// e.g. "%popSize*1.3" whereas %popSize is predefined
			Expression m_subPopSizeExpr;

			/// the function version of the parameter.
			/// the python function should take one parameter (gen) and
			/// return a vector of subpop size.
			PyObject* m_subPopSizeFunc;
	};

	/**
	   No mating. No subpopulation change.
	   During mating operator will be applied, but
	   the return values are not checked.
	*/

	class noMating: public mating
	{
		public:

			/// constructor, no new subPopsize parameter
			noMating()
				:mating(1, NULL, 0, MATE_NumOffspring,
				vectorlu(),"",NULL)
			{

			}

			/// destructor
			~noMating(){}

			/// clone() const. The same as mating::clone() const.
			/** \sa mating::clone() const
			 */
			virtual mating* clone() const
			{
				return new noMating(*this);
			}

			/// return name of the mating type
			virtual string __repr__()
			{
				return "<simuPOP::no mating>";
			}

			virtual void submitScratch(population& pop, population& scratch)
			{
			}

			/**
			 \brief do the mating. --- no mating :-)

			 All individuals will be passed to during mating operators but
			 no one will die (ignore during mating failing signal).
			*/
			virtual bool mate( population& pop, population& scratch, vector<Operator *>& ops, bool submit);
	};

	/**
	  binomial random selection

	  No sex. Choose one individual from last generation.

	  1. numOffspring protocol is honored
	  2. population size changes are allowed
	  3. selection is possible.

	  So this works just like a sexless random mating.
	  If ploidy is one, this is chromosomal mating.
	*/

	class binomialSelection: public mating
	{
		public:

			/// constructor
			binomialSelection(double numOffspring=1.,
				PyObject* numOffspringFunc=NULL,
				UINT maxNumOffspring=0,
				UINT mode=MATE_NumOffspring,
				vectorlu newSubPopSize=vectorlu(),
				string newSubPopSizeExpr="",
				PyObject* newSubPopSizeFunc=NULL)
				:mating(numOffspring,
				numOffspringFunc, maxNumOffspring, mode,
				newSubPopSize, newSubPopSizeExpr,
				newSubPopSizeFunc),
				m_sampler(rng())
				{}

			/// destructor
			~binomialSelection(){}

			/// clone() const. The same as mating::clone() const.
			/** \sa mating::clone() const
			 */
			virtual mating* clone() const
			{
				return new binomialSelection(*this);
			}

			/// return name of the mating type
			virtual string __repr__()
			{
				return "<simuPOP::binomial random selection>";
			}

			virtual void submitScratch(population& pop, population& scratch)
			{
				pop.setBoolVar("selection", false);
				// use scratch population,
				pop.pushAndDiscard(scratch);
				DBG_DO(DBG_MATING, pop.setIntVectorVar("famSizes", m_famSize));
			}

			/**
			 \brief do the mating.
			 \param pop population
			 \param scratch scratch population, will be used in this mating scheme.
			 \param ops during mating operators
			 \return return false when mating fails.
			*/
			virtual bool mate( population& pop, population& scratch, vector<Operator *>& ops, bool submit);

		protected:
			/// accumulative fitness
			Weightedsampler m_sampler;

#ifndef OPTIMIZED
			///
			vectori m_famSize;
#endif
	};

	/**
	basic sexual random mating.

	Within each subpopulation, choose male and female randomly
	randmly get one copy of chromosome from father/mother.

	require: sexed individual; ploidy == 2

	apply during mating operators and put into the next generation.

	if ignoreParentsSex is set, parents will be chosen regardless of sex.

	Otherwise, male and female will be collected and be chosen randomly.

	If there is no male or female in a subpopulation,
	if m_UseSameSexIfUniSex is true, an warning will be generated and same
	sex mating (?) will be used
	otherwise, randomMating will return false.

	if there is no during mating operator to copy alleles, a direct copy
	will be used.
	*/

	class randomMating : public mating
	{
		public:

			/// create a random mating scheme
			/**
			\param numOffspring, number of offspring or p in some modes
			\param numOffspringFunc, a python function that determine
			 number of offspring or p depending on mode
			\param maxNumOffspring used when mode=MATE_BinomialDistribution
			\param mode one of  MATE_NumOffspring , MATE_NumOffspringEachFamily,
			 MATE_GeometricDistribution, MATE_PoissonDistribution, MATE_BinomialDistribution
			\param newSubPopSize an array of subpopulation sizes, should have the same
			number of subpopulations as current population
			\param newSubPopSizeExpr an expression that will be evaluated as an array of subpop sizes
			\param newSubPopSizeFunc an function that have parameter gen and oldSize (current subpop
			size).
			\param contWhenUniSex continue when there is only one sex in the population, default to true

			*/
			randomMating( double numOffspring=1.,
				PyObject* numOffspringFunc=NULL,
				UINT maxNumOffspring=0,
				UINT mode=MATE_NumOffspring,
				vectorlu newSubPopSize=vectorlu(),
				PyObject* newSubPopSizeFunc=NULL,
				string newSubPopSizeExpr="",
				bool contWhenUniSex=true)
				:mating(numOffspring,
				numOffspringFunc, maxNumOffspring, mode,
				newSubPopSize, newSubPopSizeExpr, newSubPopSizeFunc),
				m_contWhenUniSex(contWhenUniSex),
				m_maleIndex(0), m_femaleIndex(0),
				m_maleFitness(0), m_femaleFitness(0),
				m_malesampler(rng()), m_femalesampler(rng())
			{
			}

			/// destructor
			~randomMating(){}

			/// clone() const. Generate a copy of itself and return pointer
			/// this is to make sure the object is persistent and
			/// will not be freed by python.
			virtual mating* clone() const
			{
				return new randomMating(*this);
			}

			virtual bool isCompatible(const population& pop) const
			{
				// test if individual has sex
				// if not, will yield compile time error.
				pop.indBegin()->sex();

#ifndef OPTIMIZED
				if( pop.ploidy() != 2 )
					cout << "Warning: This mating type only works with diploid population." << endl;
#endif

				return true;
			}

			/// return name of the mating type
			virtual string __repr__()
			{
				return "<simuPOP::sexual random mating>";
			}

			virtual void submitScratch(population& pop, population& scratch)
			{
				pop.setBoolVar("selection", false);
				// use scratch population,
				pop.pushAndDiscard(scratch);
				DBG_DO(DBG_MATING, pop.setIntVectorVar("famSizes", m_famSize));
			}

			/// do the mating. parameters see mating::mate .
			/**
			Within each subpopulation, choose male and female randomly
			randmly get one copy of chromosome from father/mother.

			require: sexed individual; ploidy == 2

			apply during mating operators and put into the next generation.

			Otherwise, male and female will be collected and be chosen randomly.

			- If there is no male or female in a subpopulation,
			- if m_contWhenUniSex is true, an warning will be generated and same
			sex mating (?) will be used
			- otherwise, randomMating will return false.

			*/
			virtual bool mate( population& pop, population& scratch, vector<Operator *>& ops, bool submit);

		protected:

			/// if no other sex exist in a subpopulation,
			/// same sex mating will occur if m_contWhenUniSex is set.
			/// otherwise, an exception will be thrown.
			bool m_contWhenUniSex;

			/// internal index to female/males.
			vectorlu m_maleIndex, m_femaleIndex;

			vectorf m_maleFitness, m_femaleFitness;

			// weighted sampler
			Weightedsampler m_malesampler, m_femalesampler;

#ifndef OPTIMIZED
			///
			vectori m_famSize;
#endif
	};


	/// CPPONLY
	void countAlleles(population& pop, int subpop, const vectori& loci, const vectori& alleles,
		vectorlu& numAllele);

	/**
	  controlled mating
	*/
	class controlledMating : public mating
	{
		public:

			/// Controlled mating,
			/// control allele frequency at a locus.
			/**
			\param mating a mating scheme.
			\param loci  loci at which allele frequency is controlled. Note that
			  controlling the allele frequencies at several loci may take a long
			  time.
			\param alleles alleles to control at each loci. Should have the
			  same length as loci
			\param freqFunc frequency boundaries. If the length of the return
			  value equals the size of loci, the range for loci will be
			  [value0, value0+range], [value1, value1+range] etc.
			  If the length of the return value is 2 times size of loci, it will
			be interpreted as [low1, high1, low2, high2 ...]
			*/
			controlledMating(
				mating& matingScheme,
				vectori loci,
				vectori alleles,
				PyObject* freqFunc,
				double range = 0.01
				)
				: m_loci(loci), m_alleles(alleles),
				m_freqFunc(freqFunc),
				m_range(range)
			{
				DBG_FAILIF( m_loci.empty(), ValueError, "Have to specify a locus (or a loci) to control");

				DBG_FAILIF( m_alleles.empty(), ValueError, "Have to specify allele at each locus");

				DBG_FAILIF( m_loci.size() != m_alleles.size(), ValueError, "Should specify allele for each locus");

				if(m_freqFunc == NULL || !PyCallable_Check(m_freqFunc))
					throw ValueError("Please specify a valid frequency function");
				else
					Py_INCREF(m_freqFunc);

				m_matingScheme = matingScheme.clone();
			}

			/// CPPONLY
			controlledMating(const controlledMating& rhs)
				: m_loci(rhs.m_loci),
				m_alleles(rhs.m_alleles),
				m_freqFunc(rhs.m_freqFunc),
				m_range(rhs.m_range)
			{
				Py_INCREF(m_freqFunc);
				m_matingScheme = rhs.m_matingScheme->clone();
			}

			/// destructor
			~controlledMating()
			{
				if( m_freqFunc != NULL)
					Py_DECREF(m_freqFunc);
				delete m_matingScheme;
			}

			/// clone() const. Generate a copy of itself and return pointer
			/// this is to make sure the object is persistent and
			/// will not be freed by python.
			virtual mating* clone() const
			{
				return new controlledMating(*this);
			}

			virtual bool isCompatible(const population& pop) const
			{
				return m_matingScheme->isCompatible(pop);
			}

			/// return name of the mating type
			virtual string __repr__()
			{
				return "<simuPOP::controlled mating>";
			}

			virtual bool mate( population& pop, population& scratch, vector<Operator *>& ops, bool submit);

		private:

			/// mating scheme
			mating* m_matingScheme;

			/// loci at which mating is controlled.
			vectori m_loci;

			/// allele to be controlled at each locus
			vectori m_alleles;

			/// function that return an array of frquency range
			PyObject * m_freqFunc;

			/// range, used when m_freqFunc returns a vector of the same length as m_loci
			double m_range;
	};

	/// CPPONLY
	void getExpectedAlleles(population& pop, vectorf& expFreq, const vectori& loci, const vectori& alleles,
		vectoru& expAlleles);

	/**
	  binomial random selection

	  No sex. Choose one individual from last generation.

	  1. numOffspring protocol is honored
	  2. population size changes are allowed
	  3. selection is possible.

	  So this works just like a sexless random mating.
	  If ploidy is one, this is chromosomal mating.
	*/

	class controlledBinomialSelection: public binomialSelection
	{
		public:

			/// constructor
			controlledBinomialSelection(
				vectori loci,
				vectori alleles,
				PyObject* freqFunc,
				double numOffspring=1.,
				PyObject* numOffspringFunc=NULL,
				UINT maxNumOffspring=0,
				UINT mode=MATE_NumOffspring,
				vectorlu newSubPopSize=vectorlu(),
				string newSubPopSizeExpr="",
				PyObject* newSubPopSizeFunc=NULL)
				:binomialSelection(numOffspring,
				numOffspringFunc, maxNumOffspring, mode,
				newSubPopSize, newSubPopSizeExpr,
				newSubPopSizeFunc),
				m_loci(loci),
				m_alleles(alleles),
				m_freqFunc(freqFunc)
			{
				if( m_loci.empty() || m_loci.size() != m_alleles.size() )
					throw ValueError("Please specify loci and corresponding alleles");

				if(m_freqFunc == NULL || !PyCallable_Check(m_freqFunc))
					throw ValueError("Please specify a valid frequency function");
				else
					Py_INCREF(m_freqFunc);
			}

			/// CPPONLY
			controlledBinomialSelection(const controlledBinomialSelection& rhs)
				: binomialSelection(rhs),
				m_loci(rhs.m_loci),
				m_alleles(rhs.m_alleles),
				m_freqFunc(rhs.m_freqFunc),
				m_stack()
			{
				Py_INCREF(m_freqFunc);
			}

			/// destructor
			~controlledBinomialSelection()
			{
				if( m_freqFunc != NULL)
					Py_DECREF(m_freqFunc);
			}

			/// clone() const. The same as mating::clone() const.
			/** \sa mating::clone() const
			 */
			virtual mating* clone() const
			{
				return new controlledBinomialSelection(*this);
			}

			/// return name of the mating type
			virtual string __repr__()
			{
				return "<simuPOP::binomial random selection>";
			}

			virtual void submitScratch(population& pop, population& scratch)
			{
				pop.setBoolVar("selection", false);
				// use scratch population,
				pop.pushAndDiscard(scratch);
				DBG_DO(DBG_MATING, pop.setIntVectorVar("famSizes", m_famSize));
			}

			/**
			 \brief do the mating.
			 \param pop population
			 \param scratch scratch population, will be used in this mating scheme.
			 \param ops during mating operators
			 \return return false when mating fails.
			*/
			virtual bool mate( population& pop, population& scratch, vector<Operator *>& ops, bool submit);

		private:
			/// locus at which mating is controlled.
			vectori m_loci;

			/// allele to be controlled at each locus
			vectori m_alleles;

			/// function that return an array of frquency range
			PyObject * m_freqFunc;

			///
			stack<population::IndIterator> m_stack;
	};

	/**
	basic sexual random mating.

	Within each subpopulation, choose male and female randomly
	randmly get one copy of chromosome from father/mother.

	require: sexed individual; ploidy == 2

	apply during mating operators and put into the next generation.

	if ignoreParentsSex is set, parents will be chosen regardless of sex.

	Otherwise, male and female will be collected and be chosen randomly.

	If there is no male or female in a subpopulation,
	if m_UseSameSexIfUniSex is true, an warning will be generated and same
	sex mating (?) will be used
	otherwise, randomMating will return false.

	if there is no during mating operator to copy alleles, a direct copy
	will be used.
	*/

	class controlledRandomMating : public randomMating
	{
		public:

			/// create a random mating scheme
			/**
			\param numOffspring, number of offspring or p in some modes
			\param numOffspringFunc, a python function that determine
			 number of offspring or p depending on mode
			\param maxNumOffspring used when mode=MATE_BinomialDistribution
			\param mode one of  MATE_NumOffspring , MATE_NumOffspringEachFamily,
			 MATE_GeometricDistribution, MATE_PoissonDistribution, MATE_BinomialDistribution
			\param newSubPopSize an array of subpopulation sizes, should have the same
			number of subpopulations as current population
			\param newSubPopSizeExpr an expression that will be evaluated as an array of subpop sizes
			\param newSubPopSizeFunc an function that have parameter gen and oldSize (current subpop
			size).
			\param contWhenUniSex continue when there is only one sex in the population, default to true
			*/
			controlledRandomMating(
				vectori loci,
				vectori alleles,
				PyObject* freqFunc,
				int acceptScheme=0,
				double numOffspring=1.,
				PyObject* numOffspringFunc=NULL,
				UINT maxNumOffspring=0,
				UINT mode=MATE_NumOffspring,
				vectorlu newSubPopSize=vectorlu(),
				PyObject* newSubPopSizeFunc=NULL,
				string newSubPopSizeExpr="",
				bool contWhenUniSex=true)
				:randomMating(numOffspring,
				numOffspringFunc, maxNumOffspring, mode,
				newSubPopSize,
				newSubPopSizeFunc,
				newSubPopSizeExpr,
				contWhenUniSex),
				m_loci(loci),
				m_alleles(alleles),
				m_freqFunc(freqFunc),
				m_stack()
			{
				if(m_freqFunc == NULL || !PyCallable_Check(m_freqFunc))
					throw ValueError("Please specify a valid frequency function");
				else
					Py_INCREF(m_freqFunc);
			}

			/// CPPONLY
			controlledRandomMating(const controlledRandomMating& rhs)
				: randomMating(rhs),
				m_loci(rhs.m_loci),
				m_alleles(rhs.m_alleles),
				m_freqFunc(rhs.m_freqFunc),
				m_stack()
			{
				Py_INCREF(m_freqFunc);
			}

			/// destructor
			~controlledRandomMating()
			{
				if( m_freqFunc != NULL)
					Py_DECREF(m_freqFunc);
			}

			/// clone() const. Generate a copy of itself and return pointer
			/// this is to make sure the object is persistent and
			/// will not be freed by python.
			virtual mating* clone() const
			{
				return new controlledRandomMating(*this);
			}

			virtual bool isCompatible(const population& pop) const
			{
				// test if individual has sex
				// if not, will yield compile time error.
				pop.indBegin()->sex();

#ifndef OPTIMIZED
				if( pop.ploidy() != 2 )
					cout << "Warning: This mating type only works with diploid population." << endl;
#endif

				return true;
			}

			/// return name of the mating type
			virtual string __repr__()
			{
				return "<simuPOP::sexual random mating>";
			}

			virtual void submitScratch(population& pop, population& scratch)
			{
				pop.setBoolVar("selection", false);
				// use scratch population,
				pop.pushAndDiscard(scratch);
				DBG_DO(DBG_MATING, pop.setIntVectorVar("famSizes", m_famSize));
			}

			/// do the mating. parameters see mating::mate .
			/**
			Within each subpopulation, choose male and female randomly
			randmly get one copy of chromosome from father/mother.

			require: sexed individual; ploidy == 2

			apply during mating operators and put into the next generation.

			Otherwise, male and female will be collected and be chosen randomly.

			- If there is no male or female in a subpopulation,
			- if m_contWhenUniSex is true, an warning will be generated and same
			sex mating (?) will be used
			- otherwise, controlledRandomMating will return false.

			*/
			virtual bool mate( population& pop, population& scratch, vector<Operator *>& ops, bool submit);

		private:

			/// locus at which mating is controlled.
			vectori m_loci;

			/// allele to be controlled at each locus
			vectori m_alleles;

			/// function that return an array of frquency range
			PyObject * m_freqFunc;

			///
			stack<population::IndIterator> m_stack;
	};

	/**
	   python mating.
	   Parental and offspring generation, along with during mating
	   operators, are passed to a python function. All mating
	   are done there, and the resulting population be returned.

	   This process will be slow and should be used mainly for
	   prototyping or demonstration purposes.
	*/

	class pyMating: public mating
	{
		public:

			/// constructor, no new subPopsize parameter
			pyMating(PyObject* func=NULL,
				vectorlu newSubPopSize=vectorlu(),
				string newSubPopSizeExpr="",
				PyObject* newSubPopSizeFunc=NULL)
				:mating(1.0, NULL, 0, MATE_NumOffspring,
				newSubPopSize, newSubPopSizeExpr,
				newSubPopSizeFunc)
			{
				if( !PyCallable_Check(func))
					throw ValueError("Passed variable is not a callable python function.");

				Py_XINCREF(func);
				m_mateFunc = func;
			}

			/// destructor
			~pyMating()
			{
				Py_XDECREF(m_mateFunc);
			}

			/// clone() const. The same as mating::clone() const.
			/** \sa mating::clone() const
			 */
			virtual mating* clone() const
			{
				return new pyMating(*this);
			}

			/// CPPONLY
			pyMating(const pyMating& rhs):
			mating(rhs), m_mateFunc(rhs.m_mateFunc)
			{
				if(m_mateFunc != NULL )
					Py_INCREF(m_mateFunc);
			}

			/// return name of the mating type
			virtual string __repr__()
			{
				return "<simuPOP::pyMating>";
			}

			/**
			 \brief do the mating. --- no mating :-)

			 All individuals will be passed to during mating operators but
			 no one will die (ignore during mating failing signal).
			*/
			virtual bool mate(population& pop, population& scratch, vector<Operator *>& ops, bool submit);

		private:
			PyObject* m_mateFunc;

	};

}
#endif
