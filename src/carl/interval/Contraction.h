/* 
 * File:   Contraction.h
 * Author: stefan
 *
 * Created on August 30, 2013, 4:55 PM
 */

#pragma once
#include "Interval.h"
#include "../core/Sign.h"
#include <carl/core/MultivariateHorner.h>
#include "IntervalEvaluation.h"
#include <algorithm>

#define CONTRACTION_DEBUG
#define USE_HORNER

namespace carl {
    
    template<typename Polynomial>
    class VarSolutionFormula
    {
        private:
            /// The variable, for which to solve.
            Variable mVar;
            /// Stores n, if the nth root has to be taken of mNumerator/mDenominator
            unsigned mRoot;
            /// Stores the numerator
            Polynomial mNumerator;
            /// Stores the denominator, which is one, if mDenominator == nullptr
            std::shared_ptr<const typename Polynomial::MonomType> mDenominator;
            
        public:
            VarSolutionFormula() = delete;
            
            /**
             * Constructs the solution formula for the given variable x in the equation p = 0, where p is the given polynomial.
             * The polynomial p must have one of the following forms: 
             *      1.) ax+h, with a being a rational number and h a linear polynomial not containing x and not having a constant part
             *      2.) x^i*m-y, with i being a positive integer, m being a monomial not containing x and y being a variable different from x
             * @param p The polynomial containing the given variable to construct a solution formula for.
             * @param x The variable to construct a solution formula for.
             */

            VarSolutionFormula(const Polynomial& p, Variable::Arg x):
                mVar(x),
                mRoot(1),
                mNumerator(),
                mDenominator(nullptr)
            {
        
                #ifdef CONTRACTION_DEBUG
                std::cout << __func__ << ": [Polynome]: " << p << " [#Terms]: " << p.nrTerms() << std::endl;     
                #endif


                assert(p.has(x));
                assert(!p.hasConstantTerm());
                assert(p.isLinear() || (p.nrTerms() == 2 && (carl::isOne(p.begin()->coeff()) || carl::isOne(p.rbegin()->coeff()))
                                        && ((p.begin()->has(x) && !p.rbegin()->has(x)) 
                                        || (p.rbegin()->has(x) && !p.begin()->has(x)))));
                
                // Construct the solution formula for x in p = 0

                #ifdef CONTRACTION_DEBUG
                std::cout << __func__ << ": Propagating... " << std::endl;     
                #endif

                // Case 1.):
                if (p.isLinear())
                {
                    #ifdef CONTRACTION_DEBUG
                    std::cout << __func__ << ": Case 1. (linear)... " << std::endl;     
                    #endif
                    for (const auto& t: p) {
                        assert(t.monomial() != nullptr);
                        if (t.has(x)) {
                            
                            mNumerator = p / t.coeff();
                            mNumerator -= x;
                            mNumerator *= (-1);
                            
                            #ifdef CONTRACTION_DEBUG
                            std::cout << __func__ << ": Setting mNumerator:" << mNumerator << std::endl;     
                            #endif

                            return;
                        }
                    }                    
                }

                // Case 2.)
                else
                {   
                    #ifdef CONTRACTION_DEBUG
                    std::cout << __func__ << ": Case 2. (non-linear)... " << std::endl;     
                    #endif

                    assert(p.nrTerms() == 2);
                    typename Polynomial::TermsType::const_iterator yIter;
                    typename Polynomial::TermsType::const_iterator xIter;
                    if (p.begin()->has(x))
                    {
                        xIter = p.begin();   
                        yIter = xIter;
                        ++yIter;
                    } 
                    else
                    {
                        yIter = p.begin();   
                        xIter = yIter;
                        ++xIter;
                    }

                    assert(xIter->monomial() != nullptr);
                    assert(!xIter->isLinear() || xIter->coeff() == (-1));
                    //std::cout << "[Case 2]" << std::endl;
                    if (xIter->isLinear()) 
                    {
                        mNumerator = Polynomial ( *yIter );
                    }
                    else
                    {
                        mRoot = xIter->monomial()->exponentOfVariable(x);
                        #ifdef CONTRACTION_DEBUG
                        std::cout << __func__ << ": Setting mRoot:" << mRoot << std::endl;     
                        #endif
              
                        mDenominator = xIter->monomial()->dropVariable(x);
                        #ifdef CONTRACTION_DEBUG
                        std::cout << __func__ << ": Setting mDenominator:" << mDenominator << std::endl;     
                        #endif
                        mNumerator = -Polynomial ( *yIter );
                    }
                    #ifdef CONTRACTION_DEBUG
                    std::cout << __func__ << ": Setting mNumerator:" << mNumerator << std::endl;     
                    #endif                         
                }
            }
            
            void addRoot( const Interval<double>& _interv, const Interval<double>& _varInterval, std::vector<Interval<double>>& _result ) const
            {
                Interval<double> tmp = _interv.root((int) mRoot);
                if( mRoot % 2 == 0 )
                {
                    Interval<double> rootA, rootB;
                    if( tmp.unite( -tmp, rootA, rootB ) )
                    {   
                        if (mVar.getType() == VariableType::VT_INT) {
                            rootA = rootA.integralPart();
                        }
                        rootA.intersect_assign(_varInterval);
                        if( !rootA.isEmpty() )
                            _result.push_back(std::move(rootA));
                        if (mVar.getType() == VariableType::VT_INT) {
                            rootB = rootB.integralPart();
                        }
                        rootB.intersect_assign(_varInterval);
                        if( !rootB.isEmpty() )
                            _result.push_back(std::move(rootB));
                    }
                    else
                    {
                        if (mVar.getType() == VariableType::VT_INT) {
                            rootA = rootA.integralPart();
                        }
                        rootA.intersect_assign(_varInterval);
                        if( !rootA.isEmpty() )
                            _result.push_back(std::move(rootA));
                    }
                }
                else
                {
                    if (mVar.getType() == VariableType::VT_INT) {
                        tmp = tmp.integralPart();
                    }
                    _result.push_back( std::move(tmp) );
                }
            }
            
            /**
             * Evaluates this solution formula for the given mapping of the variables occurring in the solution formula to double intervals.
             * @param intervals The mapping of the variables occurring in the solution formula to double intervals
             * @param resA The first interval of the result.
             * @param resB The second interval of the result.
             * @return true, if the second interval is not empty. (the first interval must then be also nonempty)
             */
            std::vector<Interval<double>> evaluate(const Interval<double>::evalintervalmap& intervals) const
            {
                // evaluate monomial
                std::vector<Interval<double>> result;
                assert( intervals.find(mVar) != intervals.end() );
                const Interval<double>& varInterval = intervals.at(mVar);
                Interval<double> numerator = IntervalEvaluation::evaluate(mNumerator, intervals);
                if (mDenominator == nullptr)
                {
                    addRoot( numerator, varInterval, result );
                    return result;
                }
                Interval<double> denominator = IntervalEvaluation::evaluate(*mDenominator, intervals);        
                Interval<double> result1, result2;
                // divide:
                bool split = numerator.div_ext(denominator, result1, result2);
                // extract root:
                assert(mRoot <= std::numeric_limits<int>::max());
                if( split )
                {
                    // TODO: division returns two intervals, which might be united to one interval
                    Interval<double> tmpA, tmpB;
                    if( result1.unite( result2, tmpA, tmpB ) )
                    {
                        addRoot( tmpA, varInterval, result );
                        addRoot( tmpB, varInterval, result );
                    }
                    else
                    {
                        addRoot( tmpA, varInterval, result );
                    }
                }
                else
                {
                    addRoot( result1, varInterval, result );
                }
                return result;
            }
    };

    template <template<typename> class Operator, typename Polynomial>
    class Contraction : private Operator<Polynomial> {

    private:
        const Polynomial mConstraint; // Todo: Should be a reference.
        std::map<Variable, Polynomial> mDerivatives;
        std::map<Variable, VarSolutionFormula<Polynomial>> mVarSolutionFormulas;
    public:

        Contraction(const Polynomial& constraint) : Operator<Polynomial>(),
        mConstraint(constraint),
        mDerivatives(),
        mVarSolutionFormulas() {}

        bool operator()(const Interval<double>::evalintervalmap& intervals, Variable::Arg variable, Interval<double>& resA, Interval<double>& resB, bool useNiceCenter = false, bool withPropagation = false) {
            typename std::map<Variable, Polynomial>::const_iterator it = mDerivatives.find(variable);
            if( it == mDerivatives.end() )
            {
                it = mDerivatives.emplace(variable, mConstraint.derivative(variable)).first;
            }

            #ifdef CONTRACTION_DEBUG
            std::cout << __func__ << ": contraction of " << variable << " with " << intervals << " in " << mConstraint << std::endl;
            #endif

            bool splitOccurredInContraction = Operator<Polynomial>::contract(intervals, variable, mConstraint, (*it).second, resA, resB, useNiceCenter);

            #ifdef CONTRACTION_DEBUG
            std::cout << "  after contraction: " << resA;
            if( splitOccurredInContraction ) std::cout << " and " << resB;
            std::cout << std::endl;                            
            #endif

            if( withPropagation )
            {
                typename std::map<Variable, VarSolutionFormula<Polynomial>>::const_iterator itB = mVarSolutionFormulas.find(variable);
                if( itB == mVarSolutionFormulas.end() )
                {
                    itB = mVarSolutionFormulas.emplace(variable, std::move(VarSolutionFormula<Polynomial>(mConstraint,variable))).first;
                }
                if (withPropagation) {

                

                    // calculate result of propagation
                    std::vector<Interval<double>> resultPropagation = itB->second.evaluate( intervals );

                    if( resultPropagation.empty() )
                    {
                        resA = Interval<double>::emptyInterval();
                        resB = Interval<double>::emptyInterval();
                        return false;
                    }
                    // intersect with result of contraction
                    std::vector<Interval<double>> resultingIntervals;
                    if( splitOccurredInContraction )
                    {   
                        Interval<double> tmp;
                        for( const auto& i : resultPropagation )
                        {
                            tmp = i.intersect( resA );
                            if( !tmp.isEmpty() )
                                resultingIntervals.push_back(tmp);
                            tmp = i.intersect( resB );
                            if( !tmp.isEmpty() )
                                resultingIntervals.push_back(tmp);
                        }
                    }

                    else 
                    {
                        Interval<double> tmp;
                        for( const auto& i : resultPropagation )
                        {
                            tmp = i.intersect( resA );
                            if( !tmp.isEmpty() )
                                resultingIntervals.push_back(tmp);
                        }
                    }
                    if( resultingIntervals.empty() )
                    {
                        resA = Interval<double>::emptyInterval();
                        resB = Interval<double>::emptyInterval();
                        return false;
                    }
                    if( resultingIntervals.size() == 1 )
                    {
                        resA = resultingIntervals[0];
                        resB = Interval<double>::emptyInterval();
                        return false;
                    }
                    if( resultingIntervals.size() == 2 )
                    {
                        if( resultingIntervals[0] < resultingIntervals[1] )
                        {
                            resA = resultingIntervals[0];
                            resB = resultingIntervals[1];
                        }
                        else
                        {
                            assert(resultingIntervals[1] < resultingIntervals[0]);
                            resA = resultingIntervals[1];
                            resB = resultingIntervals[0];
                        }
                        return true;
                    }
                    else
                    {
                        std::sort( resultPropagation.begin(), resultPropagation.end(), 
                                  [](const Interval<double>& i,const Interval<double> j) 
                                  { if(i<j){return true;} else { assert(j<i); return false; } }
                                 );
                        auto intervalBeforeBiggestGap = resultPropagation.begin();
                        auto iter = resultPropagation.begin();
                        ++iter;
                        double bestDistance = intervalBeforeBiggestGap->distance(*iter);
                        for( ; iter != resultPropagation.end(); ++iter )
                        {
                            auto jter = iter;
                            ++jter;
                            if( jter == resultPropagation.end() )
                            {
                                break;
                            }
                            else
                            {
                                double distance = iter->distance(*jter);
                                if( bestDistance < distance )
                                {
                                    bestDistance = distance;
                                    intervalBeforeBiggestGap = iter;
                                }
                            }
                        }
                        resA = *intervalBeforeBiggestGap;
                        for( iter = resultPropagation.begin(); iter != intervalBeforeBiggestGap; ++iter )
                        {
                            resA = resA.convexHull( *iter );
                        }
                        ++iter;
                        resB = *iter;
                        for( ; iter != resultPropagation.end(); ++iter )
                        {
                            resB = resB.convexHull( *iter );
                        }
                        return true;
                    }
                }
            }
            return splitOccurredInContraction;
        }
    };

    template<typename Polynomial>
    class SimpleNewton {
    private:
        std::map<Polynomial, MultivariateHorner<Polynomial, GREEDY_Is>> mHornerSchemes;
    public:
        
        bool contract(const Interval<double>::evalintervalmap& intervals, Variable::Arg variable, const Polynomial& constraint, const Polynomial& derivative, Interval<double>& resA, Interval<double>& resB, bool useNiceCenter = false) 
        {
            bool splitOccurred = false;
            
            double center = useNiceCenter ? intervals.at(variable).sample() : intervals.at(variable).center();
            if( center == std::numeric_limits<double>::infinity() || center == -std::numeric_limits<double>::infinity() )
            {
                resA = intervals.at(variable);
                return false;
            }
            Interval<double> centerInterval = Interval<double>(center);
            
			#ifdef CONTRACTION_DEBUG
            std::cout << "variable = " << variable << std::endl;
            std::cout << "constraint = " << constraint << std::endl;
            std::cout << "derivative = " << derivative << std::endl;
			std::cout << __func__ << ": centerInterval: " << centerInterval << std::endl;
			#endif
			
            // Create map for replacement of variables by intervals and replacement of center by point interval
            typename Interval<double>::evalintervalmap substitutedIntervalMap = intervals;
            substitutedIntervalMap[variable] = centerInterval;

            Interval<double> numerator (0);
            Interval<double> denominator(0);
            
            #ifdef USE_HORNER
                std::cout << "Hier ist der Ping!" << std::endl;
                typename  std::map<Polynomial, MultivariateHorner<Polynomial, GREEDY_Is>>::const_iterator it_constraint = mHornerSchemes.find(constraint);
                if( it_constraint == mHornerSchemes.end() )
                {
                    Polynomial constraint_tmp (constraint);
                    MultivariateHorner<Polynomial, GREEDY_Is> constraint_asHornerScheme (std::move( constraint_tmp ));
                    it_constraint = mHornerSchemes.emplace(constraint, constraint_asHornerScheme).first;
                }

                typename  std::map<Polynomial, MultivariateHorner<Polynomial, GREEDY_Is>>::const_iterator it_denominator = mHornerSchemes.find(derivative);
                if( it_denominator == mHornerSchemes.end() )
                {
                    Polynomial derivative_tmp (derivative);
                    MultivariateHorner<Polynomial, GREEDY_Is> derivative_asHornerScheme (std::move( derivative_tmp ));
                    it_denominator = mHornerSchemes.emplace(constraint, derivative_asHornerScheme).first;
                }
            
                numerator = evaluate((*it_constraint).second, substitutedIntervalMap);
                denominator = evaluate((*it_denominator).second, intervals);

            #endif 
            
            #ifndef USE_HORNER
            // Create Newton Operator
            numerator = IntervalEvaluation::evaluate(constraint, substitutedIntervalMap);
            denominator = IntervalEvaluation::evaluate(derivative, intervals);
            #endif

            Interval<double> result1, result2;
			
			#ifdef CONTRACTION_DEBUG
			std::cout << __func__ << ": numerator: " << numerator << ", denominator: " << denominator << std::endl;
			#endif
            
            bool split = numerator.div_ext(denominator, result1, result2);
            if (split) {
				#ifdef CONTRACTION_DEBUG
				std::cout << __func__ << ": caused split: " << result1 << " and " << result2 << std::endl;
				#endif
                splitOccurred = true;
                if(result1 >= result2) {
                    resA = intervals.at(variable).intersect(centerInterval.sub(result1));
                    resB = intervals.at(variable).intersect(centerInterval.sub(result2));
					if (variable.getType() == VariableType::VT_INT) {
						resA = resA.integralPart();
						resB = resB.integralPart();
					}
					#ifdef CONTRACTION_DEBUG
					std::cout << __func__ << ": result after intersection: " << resA << " and " << resB << std::endl;
					#endif
                    if( resB.isEmpty() )
                    {
                        splitOccurred = false;
                    }
                    else if( resA.isEmpty() ) // resB is not empty at this state
                    {
                        resA = resB;
                        resB = Interval<double>::emptyInterval();
                        splitOccurred = false;
                    }
                }
                else
                {
                    resA = intervals.at(variable).intersect(centerInterval.sub(result2));
                    resB = intervals.at(variable).intersect(centerInterval.sub(result1));
					if (variable.getType() == VariableType::VT_INT) {
						resA = resA.integralPart();
						resB = resB.integralPart();
					}
					#ifdef CONTRACTION_DEBUG
					std::cout << __func__ << ": result after intersection: " << resA << " and " << resB << std::endl;
					#endif
                    if( resB.isEmpty() )
                    {
                        splitOccurred = false;
                    }
                    else if( resA.isEmpty() ) // resB is not empty at this state
                    {
                        resA = resB;
                        resB = Interval<double>::emptyInterval();
                        splitOccurred = false;
                    }
                }
            } else {
				#ifdef CONTRACTION_DEBUG
				std::cout << __func__ << ": result: " << result1 << std::endl;
				#endif
                resA = intervals.at(variable).intersect(centerInterval.sub(result1));
				if (variable.getType() == VariableType::VT_INT) {
					resA = resA.integralPart();
				}
				#ifdef CONTRACTION_DEBUG
				std::cout << __func__ << ": result after intersection: " << resA << std::endl;
				#endif
            }
            return splitOccurred;
        }
    };

    typedef Contraction<SimpleNewton, Polynomial> SimpleNewtonContraction;

}
